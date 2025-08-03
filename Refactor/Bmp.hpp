#include "GeneralDefines.hpp"
#include "Console.hpp"


/**
* @brief Structure to hold BMP file name, headers and data.
*/
struct BmpFile
{
	std::string fileName;
	BITMAPFILEHEADER fileHeader;
	BITMAPV5HEADER dibHeader;
	std::vector<uint8_t> data;

	explicit BmpFile( __in const std::string& iFileName ): fileHeader{}, dibHeader{}, data{}
	{
		this->fileName.resize( iFileName.size() + 1 );
		std::exchange( this->fileName, iFileName );
		if ( this->fileName.empty() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "BmpFile: File name cannot be empty." ) );
		}
	}
	~BmpFile() = default;
};



/**
* @brief Class to handle BMP file implantation and extraction.
*/
class BmpImplantation
{
private:

	
	
	static constexpr uint8_t BMP_24 = 0x18; // 24 bits per pixel
	static constexpr uint8_t BMP_32 = 0x20; // 32 bits per pixel
	static constexpr uint32_t BMP_SIG = 0x4D42; // 'BM'
	
	BmpFile bmpFile;
	std::unique_ptr<std::ifstream> readBmpFile = nullptr;
	std::unique_ptr<std::ofstream> writeBmpFile = nullptr;

	inline std::string_view GetFileName() const
	{
		return std::string_view( this->bmpFile.fileName );
	}

	
	inline size_t GetDataOffset() const
	{
		return this->bmpFile.fileHeader.bfOffBits;
	}

	inline size_t GetDataSize() const
	{
		return static_cast<size_t>( this->bmpFile.fileHeader.bfSize ) - this->bmpFile.fileHeader.bfOffBits;
	}

	inline size_t GetWidth() const
	{
		return this->bmpFile.dibHeader.bV5Width;
	}

	inline size_t GetHeight() const 
	{
		return this->bmpFile.dibHeader.bV5Height;
	}

	inline size_t GetStride() const
	{
		return static_cast<size_t>( this->bmpFile.dibHeader.bV5Width ) * ( this->bmpFile.dibHeader.bV5BitCount / CHAR_BIT );
	}

	inline size_t GetBitsPerPixel() const
	{
		return this->bmpFile.dibHeader.bV5BitCount;
	}

	inline bool IsValidBmp() const
	{
		return this->bmpFile.fileHeader.bfType == BMP_SIG &&
		( this->bmpFile.dibHeader.bV5Compression == BI_RGB || this->bmpFile.dibHeader.bV5Compression == BI_BITFIELDS ) &&
		( this->bmpFile.dibHeader.bV5BitCount == BMP_24 || this->bmpFile.dibHeader.bV5BitCount == BMP_32);
	}

	inline std::span<uint8_t> GetData()&
	{
		if ( this->bmpFile.data.empty() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "BMP data is not loaded." ) );
		}
		return std::span<uint8_t>( this->bmpFile.data.data(), this->bmpFile.data.size() );
	}

	inline void VerifyBmp() const
	{
		if ( !IsValidBmp() )
		{
			HandleError( std::runtime_error( "Invalid BMP file format." ) );
		}
	}


	/**
	* @brief Reads the BMP file headers and verifies the BMP format.
	*/
	inline void ReadFileHeaders()
	{
		this->readBmpFile = std::make_unique<std::ifstream>( this->bmpFile.fileName.c_str(), std::ios::binary );
		if ( !readBmpFile->is_open() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "Failed to open BMP file for reading." ) );
		}

		this->readBmpFile->seekg( 0, std::ios::beg );

		this->readBmpFile->read( std::bit_cast< char* >( &this->bmpFile.fileHeader ), sizeof( BITMAPFILEHEADER ) );
		if ( this->readBmpFile->fail() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "Failed to read BMP file header." ) );
		}

		this->readBmpFile->read( std::bit_cast< char* >( &this->bmpFile.dibHeader ), sizeof( BITMAPV5HEADER ) );
		if ( this->readBmpFile->fail() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "Failed to read BMP DIB header." ) );
		}
	}

	/**
	* @brief Reads the BMP data from the file into the bmpFile structure.
	* This is explicitly the pixel data of the BMP file.
	*/
	inline void ReadBmpData()
	{
		this->bmpFile.data.resize( GetDataSize() );
		if ( this->bmpFile.data.capacity() != GetDataSize() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "Failed to allocate memory for BMP data." ) );
		}
		
		this->readBmpFile->seekg( GetDataOffset(), std::ios::beg );
		
		this->readBmpFile->read( std::bit_cast< char* >( this->bmpFile.data.data() ), GetDataSize() );
		if ( this->readBmpFile->gcount() != GetDataSize() || this->readBmpFile->fail() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "Failed to read BMP data." ) );
		}
	}


	inline void LoadBmp()
	{
		ReadFileHeaders();
		VerifyBmp();
		ReadBmpData();
		this->readBmpFile->close();
	}

	/**
	* @brief Reads the implant data from the BMP file.
	* @return A vector of bytes containing the implant data.
	*/
	std::vector<uint8_t> ReadImplant()
	{
		if ( this->bmpFile.data.empty() )
		{
			LoadBmp();
		}

		auto imageData = GetData();
		if ( imageData.empty() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "BMP data is empty." ) );
		}

		size_t lengthOfData = 0x0; 
		for ( size_t i = 0; i < sizeof( size_t ) - 0x01; ++i )
		{
			lengthOfData |= static_cast<size_t>( imageData[ i ] << ( i << 0x03 ) );
		}
		if ( lengthOfData == 0x0 ) [[unlikely]]
		{
			HandleError( std::runtime_error( "No implant data found in BMP file." ) );
		}

		std::vector<uint8_t> implantData;
		implantData.reserve( ( lengthOfData >> 0x02 ) + 0x01 ); 
		if ( implantData.capacity() < ( lengthOfData >> 0x02 ) + 0x01 ) [[unlikely]]
		{
			HandleError( std::runtime_error( "Failed to allocate memory for implant data." ) );
		}

		BitBreakDown breakDown;

		for ( size_t i = sizeof( size_t ); i < lengthOfData + sizeof( size_t ); i += 0x04 )
		{
			auto chunk = imageData.subspan( i, 0x04 );
			implantData.push_back( breakDown.GetBytes( chunk ) );
		}

		if ( implantData.empty() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "No implant data found before the exit signature." ) );
		}
		
		return implantData; 
	}


	/**
	* @brief Writes the implant data to the BMP file.
	*/
	void WriteImplant( const std::span<uint8_t>& implantData )
	{

		if ( this->bmpFile.data.empty() ) [[unlikely]]
		{
			LoadBmp();
		}
		
		if ( implantData.size() > ( GetDataSize() >> 0x02 ) ) [[unlikely]]
		{
			HandleError( std::runtime_error( "Implant data is too large for the BMP file." ) );
		}

		if ( implantData.empty() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "Implant data is empty." ) );
		}

		auto imageData = GetData();
		if ( imageData.empty() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "BMP data is empty." ) );
		}

		const size_t lengthOfData = implantData.size() << 0x02;
		for ( size_t i = 0; i < sizeof( size_t ) - 0x01; ++i )
		{
			imageData[ i ] = ( lengthOfData >> ( i << 0x03 ) ) & UINT8_MAX;
		}

		BitBreakDown breakDown;

		for ( size_t i = 0; i < implantData.size(); ++i )
		{
			auto chunk = imageData.subspan( ( i << 0x02 ) + sizeof( size_t ), 0x04 );
			breakDown.SetBytes( implantData[ i ], chunk );
		}
	}

	/**
	* @brief Writes the BMP data back to the file.
	*/
	void WriteBmpData()
	{
		if ( this->bmpFile.data.empty() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "BMP data is not loaded.") );
		}
		
		this->writeBmpFile = std::make_unique<std::ofstream>( this->bmpFile.fileName.c_str(), std::ios::binary );
		if ( !writeBmpFile->is_open() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "Failed to open BMP file for writing." ) );
		}

		this->writeBmpFile->seekp( 0, std::ios::beg );
		
		this->writeBmpFile->write( std::bit_cast< const char* >( &this->bmpFile.fileHeader ), sizeof( BITMAPFILEHEADER ) );
		if ( this->writeBmpFile->fail() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "Failed to write BMP file header." ) );
		}
		
		this->writeBmpFile->write( std::bit_cast< const char* >( &this->bmpFile.dibHeader ),  sizeof( BITMAPV5HEADER ) );
		if ( this->writeBmpFile->fail() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "Failed to write BMP DIB header." ) );
		}

		this->writeBmpFile->seekp( GetDataOffset(), std::ios::beg );
		
		this->writeBmpFile->write( std::bit_cast< const char* >( this->bmpFile.data.data() ), GetDataSize() );
		if ( this->writeBmpFile->fail() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "Failed to write BMP data." ) );
		}
		
		this->writeBmpFile->close();
	}

public:

	explicit BmpImplantation( __in const std::string& iFileName ): bmpFile( iFileName )
	{
		this->LoadBmp();
	}

	~BmpImplantation()
	{
		if ( this->readBmpFile && this->readBmpFile->is_open() ) [[unlikely]]
		{
			this->readBmpFile->close();
		}

		if ( this->writeBmpFile && this->writeBmpFile->is_open() ) [[unlikely]]
		{
			this->writeBmpFile->close();
		}
	}

	/**
	* @brief public method to read implant data from the BMP file.
	*/
	inline std::vector<uint8_t> Read()
	{
		return ReadImplant();
	};

	/**
	* @brief public method to write implant data to the BMP file.
	*/
	inline void Write( const std::span<uint8_t>& implantData )
	{
		WriteImplant( implantData );
		WriteBmpData();
	};
};
