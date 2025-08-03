#include "GeneralDefines.hpp"
#include "Console.hpp"

//http://soundfile.sapp.org/doc/WaveFormat/

/*
* These are all the structures needed to
* define a WAVE file.
*/

struct RiffDesc
{
	uint32_t chunkID = 0;        // "RIFF" in ASCII, (0x52494646 big-endian form)
	uint32_t chunkSize = 0;      // Size of the file minus 8 bytes( 4 bytes for chunkID and 4 bytes for chunkSize )
	uint32_t format = 0;         // "WAVE" in ASCII, (0x57415645 big-endian form)
	RiffDesc() = default;
};

struct FormatDesc
{
	uint32_t subchunk1ID = 0;  // "fmt " in ASCII, (0x666d7420 big-endian form)
	uint32_t subchunk1Size = 0; // Size of the fmt chunk (16 for PCM)
	uint16_t formatTag = 0;	// Format tag (1 for PCM, WAVE_FORMAT_EXTENSIBLE for extensible formats)
	uint16_t numChannels = 0; // Number of channels (1 for mono, 2 for stereo)
	uint32_t sampleRate = 0; // Sample rate in Hz
	uint32_t byteRate = 0; // Byte rate (sampleRate * numChannels * bitsPerSample / 8)
	uint16_t blockAlign = 0; // Block align (numChannels * bitsPerSample / 8)
	uint16_t bitsPerSample = 0; // Bits per sample (8, 16, 24, or 32)
	FormatDesc() = default;
};

/**
* @brief Structure for extensible format description in WAVE files.
* @details This is here for any future expansion of the WAVE format.
*/
struct ExtensibleFormatDesc
{
	uint16_t extraParamSize; // Size of the extra parameters (for extensible formats, 0 for PCM)
	union
	{
		uint16_t validBitsPerSample = 0; // Valid bits per sample (for extensible formats)
		uint16_t samplesPerBlock; // Samples per block (for extensible formats)
		uint16_t reserved; // Reserved (for extensible formats)
	} samples; // Samples information for extensible formats;
	uint32_t channelMask = 0; // Channel mask (for extensible formats)
	GUID subFormat = { 0 }; // Sub-format GUID (for extensible formats)	
};

struct DataDesc
{
	uint32_t subchunk2ID = 0;   // "data" in ASCII, (0x64617461 big-endian form)
	uint32_t subchunk2Size = 0; // Size of the data chunk (number of samples * numChannels * bitsPerSample / 8)
	DataDesc() = default;
};


struct WaveHeader
{
	RiffDesc riffInfo{};
	FormatDesc fmtInfo{};
	DataDesc dataInfo{};

	WaveHeader() = default;
};



/**
* @brief Structure to hold the WAVE file information.
*/
struct WaveFile
{
	std::string fileName{};
	WaveHeader header{};
	std::vector<uint8_t> data{};


	explicit WaveFile( __in const std::string& iFileName )
	{
		if ( iFileName.empty() )
		{
			HandleError( std::runtime_error( "WaveFile: File name cannot be empty." ) );
		}
		this->fileName.resize( iFileName.size() + 1 );
		std::exchange( this->fileName, iFileName );
		if ( this->fileName.empty() )
		{
			HandleError( std::runtime_error( "WaveFile: File name cannot be empty." ) );
		}
	}
	~WaveFile() = default;
};


/**
* @brief Class to handle WAVE file implantation and extraction.
*/
class WaveImplantation
{
private:
	// Constants for WAVE file format
	static constexpr uint32_t WAVE_SIG = 0x45564157; // 'WAVE' in ASCII
	static constexpr uint32_t WAVE_FMT = 0x20746D66; // 'fmt ' in ASCII
	static constexpr uint32_t WAVE_DATA = 0x61746164; // 'data' in ASCII
	static constexpr uint32_t WAVE_RIFF = 0x46464952; // 'RIFF' in ASCII

	WaveFile waveFile;
	size_t dataOffset;
	std::unique_ptr<std::ifstream> readWaveFile = nullptr;
	std::unique_ptr<std::ofstream> writeWaveFile = nullptr;
	
	
	inline std::string_view GetFileName() const
	{
		return std::string_view(  this->waveFile.fileName );
	}

	inline size_t GetDataSize() const
	{
		return this->waveFile.header.dataInfo.subchunk2Size;
	}

	inline bool VerifyWave() const
	{
		return ( this->waveFile.header.riffInfo.chunkID != WAVE_RIFF || 
			this->waveFile.header.riffInfo.format != WAVE_SIG ||
			this->waveFile.header.fmtInfo.subchunk1ID != WAVE_FMT );
		
	}

	/**
	* @brief Reads the WAVE file header and initializes the WaveFile structure.
	*/
	void ReadFileHeader()
	{
		this->readWaveFile = std::make_unique<std::ifstream>( this->waveFile.fileName.c_str(), std::ios::binary);
		if ( !readWaveFile->is_open() )
		{
			HandleError( std::runtime_error( "Failed to open WAVE file for reading." ) );
		}

		this->readWaveFile->seekg( 0, std::ios::beg );
		
		this->readWaveFile->read( std::bit_cast< char* >( &this->waveFile.header.riffInfo ), sizeof( RiffDesc ) );
		if (  this->readWaveFile->fail() )
		{
			HandleError( std::runtime_error( "Failed to read WAVE file header." ) );
		}

		this->readWaveFile->seekg( sizeof( RiffDesc ), std::ios::beg );

		this->readWaveFile->read( std::bit_cast< char* >( &this->waveFile.header.fmtInfo ), sizeof( FormatDesc ) );
		if ( this->readWaveFile->fail() )
		{
			HandleError( std::runtime_error( "Failed to read WAVE file format information." ) );
		}

	
		if ( this->VerifyWave() )
		{
			HandleError( std::runtime_error( "Invalid WAVE file format." ) );
		}

		if ( this->waveFile.header.fmtInfo.formatTag != WAVE_FORMAT_PCM )
		{
			HandleError( std::runtime_error( "Unsupported WAVE format. Only PCM format are supported." ) );
		}

		this->readWaveFile->seekg( sizeof( RiffDesc ) + sizeof( FormatDesc ), std::ios::beg );
		
		this->readWaveFile->read( std::bit_cast< char* >( &this->waveFile.header.dataInfo ), sizeof( DataDesc ) );
		if ( this->readWaveFile->fail() )
		{
			HandleError( std::runtime_error( "Failed to read WAVE file data chunk ID." ) );
		}

		if ( this->waveFile.header.dataInfo.subchunk2ID != WAVE_DATA )
		{
			HandleError( std::runtime_error( "Invalid WAVE data chunk ID. Expected 'data'." ) );
		}

		this->readWaveFile->seekg( sizeof( RiffDesc ) + sizeof( FormatDesc ) + sizeof( DataDesc ), std::ios::beg );

		this->dataOffset = this->readWaveFile->tellg();
	}

	/**
	* @brief Reads the WAVE data from the file into the WaveFile structure.
	* This is explicitly the sample data(audio data) of the WAVE file.
	*/
	inline void ReadWaveData()
	{
		this->waveFile.data.resize( GetDataSize() );
		if ( this->waveFile.data.capacity() != GetDataSize() )
		{
			HandleError( std::runtime_error( "Failed to allocate memory for WAVE data." ) );
		}

		this->readWaveFile->seekg( this->dataOffset, std::ios::beg );

		this->readWaveFile->read( std::bit_cast< char* >( this->waveFile.data.data() ), GetDataSize() );
		if ( this->readWaveFile->fail() )
		{
			HandleError( std::runtime_error( "Failed to read WAVE data." ) );
		}
	}

	/**
	* @brief Gets the WAVE data as a span of bytes.
	* 
	* @details This helps increase performance by avoiding unnecessary copies.
	* 
	* @return The WAVE data wrapped in a std::span.
	*/
	inline std::span<uint8_t> GetData()
	{
		if ( this->waveFile.data.empty() )
		{
			HandleError( std::runtime_error( "WAVE data is not loaded." ) );
		}
		return std::span<uint8_t>( this->waveFile.data.data(), this->waveFile.data.size() );
	}
	

	inline void LoadWave()
	{
		ReadFileHeader();
		VerifyWave();
		ReadWaveData();
		this->readWaveFile->close();
	}


	/**
	* @brief Reads the implant data from the WAVE file.
	*/
	std::vector<uint8_t> ReadImplant()
	{
		if ( this->waveFile.data.empty() )
		{
			LoadWave();
		}
		
		auto sampleData = GetData();
		if ( sampleData.empty() )
		{
			HandleError( std::runtime_error( "WAVE data is empty." ) );
		}

		// Get the length of the implant data from the first 8 bytes
		size_t lengthOfData = 0x0;
		for ( size_t i = 0; i < sizeof( size_t ) - 0x01; ++i )
		{
			lengthOfData |= static_cast< size_t >( sampleData[ i ] << ( i << 0x03 ) );
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

		// Read the implant data from the sample data
		for ( size_t i = sizeof( size_t ); i < lengthOfData + sizeof( size_t ); i += 0x04 )
		{
			auto chunk = sampleData.subspan( i, 0x04 );
			implantData.push_back( breakDown.GetBytes( chunk ) );
		}

		if ( implantData.empty() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "No implant data found before the exit signature." ) );
		}

		return implantData;
	}


	/**
	* @brief Writes the implant data into the WAVE file.
	*/
	void WriteImplant( const std::span<uint8_t>& implantData )
	{
		if ( this->waveFile.data.empty() )
		{
			LoadWave();
		}
		
		if ( implantData.size() > ( GetDataSize() >> 2 ) )
		{
			HandleError( std::runtime_error( "Implant data is too large for the WAVE file." ) );
		}

		if ( implantData.empty() )
		{
			HandleError( std::runtime_error( "Implant data is empty." ) );
		}
		
		auto sampleData = GetData();
		
		if ( sampleData.empty() )
		{
			HandleError( std::runtime_error( "WAVE data is empty." ) );
		}

		// Set the first 8 bytes to the length of the implant data
		const size_t lengthOfData = implantData.size() << 0x02;
		for ( size_t i = 0; i < sizeof( size_t ) - 0x01; ++i )
		{
			sampleData[ i ] = ( lengthOfData >> ( i << 0x03 ) ) & UINT8_MAX;
		}

		BitBreakDown breakDown;
		// Write the implant data into the sample data
		for ( size_t i = 0; i < implantData.size(); ++i )
		{
			auto chunk = sampleData.subspan( ( i << 0x02 ) + sizeof( size_t ), 0x04 );
			breakDown.SetBytes( implantData[ i ], chunk );
		}
		
	}

	/**
	* @brief Writes the WAVE data back to the file.
	*/
	void WriteWaveData()
	{
		if ( this->waveFile.data.empty() )
		{
			HandleError( std::runtime_error( "WAVE data is not loaded." ) );
		}

		this->writeWaveFile = std::make_unique<std::ofstream>( this->waveFile.fileName.c_str(), std::ios::binary);
		if ( !writeWaveFile->is_open() )
		{
			HandleError( std::runtime_error( "Failed to open WAVE file for writing." ) );
		}

		this->writeWaveFile->seekp( 0, std::ios::beg );

		this->writeWaveFile->write( std::bit_cast< const char* >( &this->waveFile.header ), sizeof( WaveHeader ) );
		if ( this->writeWaveFile->fail() )
		{
			HandleError( std::runtime_error( "Failed to write WAVE file header." ) );
		}

		this->writeWaveFile->seekp( this->dataOffset, std::ios::beg );
			
		this->writeWaveFile->write( std::bit_cast< const char* >( this->waveFile.data.data() ), this->waveFile.data.size() );
		if ( this->writeWaveFile->fail() )
		{
			HandleError( std::runtime_error( "Failed to write WAVE data." ) );
		}
		
		this->writeWaveFile->close();
	}




public:

	explicit WaveImplantation( __in const std::string& iFileName ): waveFile( iFileName ) 
	{
		this->LoadWave();
	}
	
	~WaveImplantation()
	{
		if ( this->readWaveFile && this->readWaveFile->is_open() )
		{
			this->readWaveFile->close();
		}
	}

	/**
	* @brief public method to read implant data from the WAVE file.
	* @return A vector of bytes containing the implant data.
	*/
	inline std::vector<uint8_t> Read()
	{
		return ReadImplant();
	};

	/**
	* @brief public method to write implant data into the WAVE file.
	*/ 
	inline void Write( const std::span<uint8_t>& implantData )
	{
		WriteImplant( implantData );
		WriteWaveData();
	}

};
