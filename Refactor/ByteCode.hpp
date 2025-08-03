#include "GeneralDefines.hpp"
#include "Console.hpp"


/**
* @brief A class to read a file and convert its contents into a vector of bytes.
*/
class FileReader
{
private:
	std::unique_ptr<std::ifstream> fileStream;
	std::string fileName;
	std::vector<uint8_t> fileData;

public:

	explicit FileReader( __in const std::string& iFileName ):fileData()
	{
		this->fileStream = std::make_unique<std::ifstream>( iFileName, std::ios::binary);
		if (!this->fileStream->is_open()) [[unlikely]]
		{
			HandleError( std::runtime_error("FileReader: Failed to open file") );
		} else
		{
			this->fileName.resize( iFileName.size() + 1 );
			std::exchange( this->fileName, iFileName );
		}

		std::string content(
			( std::istreambuf_iterator<char>( *this->fileStream.get() ) ),
			( std::istreambuf_iterator<char>() )
		);

		std::istringstream hexStream( content );

		uint16_t tempInt;
		while ( hexStream >> std::hex >> tempInt )
		{
			this->fileData.push_back( static_cast<uint8_t>( tempInt ) );
		}

		if ( this->fileData.empty() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "FileReader: File is empty or could not be read" ) );
		}

		this->fileData.shrink_to_fit(); 

		this->fileStream->close();
		if ( this->fileStream->fail() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "FileReader: Failed to close file stream" ) );
		} else if ( this->fileData.empty() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "FileReader: No data read from file" ) );
		}
	}

	~FileReader()
	{
		if ( fileStream && fileStream->is_open() ) [[unlikely]]
		{
			fileStream->close();
		}
	}

	inline auto GetData() -> std::span<uint8_t>
	{
		return std::span<uint8_t>( this->fileData.data(), this->fileData.size());
	}

	inline auto GetFileName() const -> std::string_view
	{
		return this->fileName;
	}
};


/**
* @brief A class to write a vector of bytes to a file in hexadecimal format.
*/
class FileWriter
{
private:
	std::unique_ptr<std::ofstream> fileStream;
	std::string fileName;

public:

	template<typename T>
		requires ( std::is_same_v<T, std::vector<uint8_t>> || std::is_same_v<T, std::span<uint8_t>> || std::ranges::range<T> )
	explicit FileWriter( __in const std::string& iFileName, __in const T data ): fileName( iFileName )
	{
		if ( iFileName.empty() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "FileWriter: File name cannot be empty" ) );
		} else
		{
			this->fileName.resize( iFileName.size() + 1 );
			std::exchange( this->fileName, iFileName );
		}
		
		this->fileStream = std::make_unique<std::ofstream>( iFileName, std::ios::binary );
		if ( !this->fileStream->is_open() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "FileWriter: Failed to open file for writing" ) );
		}

		std::string hexString;
		static size_t byteCnt = 0;
		std::ranges::for_each( data, [&]( const auto& byte )
		{
			hexString += std::format( "{:02X} ", byte );
			if ( ++byteCnt % 16 == 0 )
			{
				hexString += "\n";
			}
		} );

		this->fileStream->seekp( 0, std::ios::beg );

		this->fileStream->write( hexString.data(), hexString.size() );
		if ( this->fileStream->fail() ) [[unlikely]]
		{
			HandleError( std::runtime_error( "FileWriter: Failed to write data to file" ) );
		}
		this->fileStream->close();
	}

	~FileWriter()
	{
		if ( fileStream && fileStream->is_open() ) [[unlikely]]
		{
			fileStream->close();
		}
	}

	inline auto GetFileName() const -> std::string_view
	{
		return this->fileName;
	}

};
