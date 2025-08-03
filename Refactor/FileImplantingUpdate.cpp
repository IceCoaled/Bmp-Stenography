#include <utility>

#include <utility>

#include "Bmp.hpp"
#include "Wave.hpp"
#include "ByteCode.hpp"
#include "Console.hpp"


// Need to forward declare so we
// can call main from other functions
int main( int argC, char** argV );


enum class FileIO: uint8_t
{
	IMPLANT,
	EXTRACT,
	BYTECODER,
	BYTECODEW,
	BMP,
	WAVE,
	INVALID,
};
using enum FileIO;

namespace MainGlobals
{
	static bool firstRun = true;
	static int8_t userSelection = -1;
	static int8_t userInput = -1;
	static FileIO cmdLineSelect = INVALID;

	static constexpr uint8_t IMPLANT_BMP = 0x01;	//
	static constexpr uint8_t EXTRACT_BMP = 0x02;	// Not putting these in a enum as it just
	static constexpr uint8_t IMPLANT_WAVE = 0x03;	// Adds unnecessary complexity to get
	static constexpr uint8_t EXTRACT_WAVE = 0x04;	// underlying type from user input

	static std::string byteCodeRFile{};
	static std::string implantFile{};
	static std::string byteCodeWFile{};

	static std::unique_ptr<FileReader> byteReader = nullptr;
	static std::unique_ptr<FileWriter> byteWriter = nullptr;
	static std::unique_ptr<BmpImplantation> bmpImplant = nullptr;
	static std::unique_ptr<WaveImplantation> waveImplant = nullptr;

	static std::vector<uint8_t> implantData{};


	void ResetGlobals()
	{
		userSelection = -1;
		userInput = -1;
		byteCodeRFile.clear();
		byteCodeWFile.clear();
		implantFile.clear();
		byteReader.reset();
		byteWriter.reset();
		bmpImplant.reset();
		waveImplant.reset();
		implantData.clear();
	}
};//!MainGlobals

using namespace MainGlobals;


[[noreturn]] void UnhandledException()
{
	std::string errorMessage = "An unhandled exception occurred. Please report this issue to the developer.";
	Console::Error( errorMessage );
	std::this_thread::sleep_for( 5s );
	Console::ResetConsole();
	HandleError( std::runtime_error( errorMessage ) );
}



inline void PrintHelp()
{
	Console::Menu( "Usage: FileImplantingUpdate [i/e/h] <file path> <file path>" );
	Console::Menu( "Usage: FileImplantingUpdate [i] <bytecode file> <implant file>" );
	Console::Menu( "Usage: FileImplantingUpdate [e] <implanted file> <bytecode file>" );
	Console::Menu( "Usage: FileImplantingUpdate [h]" );
	Console::Menu( "i - Implant data into BMP or WAVE file" );
	Console::Menu( "e - Extract data from BMP or WAVE file" );
	Console::Menu( "h - Show this help message" );
	Console::Menu( "bytecode file - The bytecode file to read from or write to" );
	Console::Menu( "implant file - The BMP or WAVE file to implant data into" );
	Console::Menu( "implanted file - The BMP or WAVE file to extract data from" );
	std::cout << std::endl;
	Console::Menu( "Press any key to exit..." );
	system( "pause" );
}



inline auto PrintMenu() -> void
{
	Console::Menu( "IceCoaled Steganography Tool" );
	Console::Menu( "Please select an option:" );
	Console::Menu( "1) Implant BMP" );
	Console::Menu( "2) Extract BMP" );
	Console::Menu( "3) Implant WAVE" );
	Console::Menu( "4) Extract WAVE" );
	std::cout << std::endl;
}



template<FileIO ioType>
	requires ( ioType == IMPLANT || ioType == BYTECODEW )
void SuccessReset()
{
	std::string message;
	if constexpr ( ioType == IMPLANT )
	{
		message = "Implantation completed successfully";
	} else if constexpr ( ioType == BYTECODEW )
	{
		message = "Bytecode written successfully";
	}

	Console::Success( message );
	Console::Menu( "Going back to main menu..." );
	std::this_thread::sleep_for( 5s );
	ResetGlobals();
	firstRun = false;
	Console::ClearConsole();
	std::ignore = main(0, nullptr );
}


[[noreturn]] inline void ErrorClose( const char* msg )
{
	Console::Error( msg );
	Console::Menu( "Exiting application..." );
	std::this_thread::sleep_for( 2s );
	ResetGlobals();
	_exit( EXIT_FAILURE );
}




inline void GetUserInput()
{
	while ( true )
	{
		if ( _kbhit() )
		{
			userInput = _getch();
			if ( userInput >= '1' && userInput <= '4' )
			{
				userSelection = userInput - '0';
				break;
			} else
			{
				Console::Warning( "Invalid input, please try again" );
				std::this_thread::sleep_for( 1s );
				Console::DeleteLastLine();
			}
		}
	}
}


template<FileIO ioType>
requires ( ioType == IMPLANT || ioType == EXTRACT ||
ioType == BYTECODER || ioType == BYTECODEW )
void GetFile( __in std::string& str )
{
	std::string message;
	if constexpr ( ioType == BYTECODER )
	{
		message = "Please select the bytecode file to read from";
	} else if constexpr ( ioType == BYTECODEW )
	{
		message = "Please select the bytecode file to write to";
	} else if constexpr ( ioType == IMPLANT )
	{
		message = "Please select the Bmp or Wave file to implant data";
	} else if constexpr ( ioType == EXTRACT )
	{
		message = "Please select the Bmp or Wave file to extract data from";
	}

	constexpr auto filter = ( ioType == BYTECODER || ioType == BYTECODEW ) ?
		"Text\0 * .txt\0 Bin\0 * .bin\0 File\0 * .file\0 Dat\0 * .dat\0\0" :
		"Bmp\0 * .bmp\0 Wave\0 * .wav\0\0";

	OPENFILENAMEA ofn{ 0 };
	ofn.lStructSize = sizeof( OPENFILENAMEA );
	ofn.hwndOwner = GetConsoleWindow();
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = &str[ 0 ];
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = message.c_str();
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if ( !GetOpenFileNameA( &ofn ) )
	{
		ErrorClose( "Failed to get file, exiting" );
	} else
	{	
		if constexpr ( ioType == BYTECODER || ioType == BYTECODEW )
		{
			Console::Success( "Bytecode file selected", ofn.lpstrFile );

		} else if constexpr ( ioType == IMPLANT )
		{
			Console::Success( "Implant file selected", ofn.lpstrFile );
		} else if constexpr ( ioType == EXTRACT )
		{
			Console::Success( "Extract file selected", ofn.lpstrFile );
		} 
	}
}



template<FileIO ioType>
requires ( ioType == IMPLANT || 
ioType == EXTRACT || ioType == BYTECODER || 
ioType == BYTECODEW )
bool AskToPrint()
{
	std::string message;
	if constexpr( ioType == IMPLANT )
	{
		message = "Do you want to print the implant data? (Y/N)";
	} else if constexpr ( ioType == EXTRACT )
	{
		message = "Do you want to print the extracted data? (Y/N)";
	} else if constexpr ( ioType == BYTECODER || ioType == BYTECODEW )
	{
		message = "Do you want to print the bytecode data? (Y/N)";
	} 
	
	Console::Menu( message );
	while ( true )
	{
		if ( _kbhit() )
		{
			auto input = _getch();
			if ( input == 'Y' || input == 'y' )
			{
				return true;
			} else if ( input == 'N' || input == 'n' )
			{
				return false;
			} else
			{
				Console::Warning( "Invalid input, please try again" );
				std::this_thread::sleep_for( 1s );
				Console::DeleteLastLine();
				Console::Menu( message );
			}
		}
	}
}

template<FileIO ioType>
requires ( ioType == IMPLANT || ioType == EXTRACT || ioType == BYTECODER || ioType == BYTECODEW )
inline FileIO VerifyFileExtension( std::string fileName )
{
	static constexpr auto txtExt = "txt";
	static constexpr auto binExt = "bin";
	static constexpr auto fileExt = "file";
	static constexpr auto datExt = "dat";
	static constexpr auto bmpExt = "bmp";
	static constexpr auto wavExt = "wav";
	
	// Convert fileName to lowercase for case-insensitive comparison
	std::transform( fileName.begin(), fileName.end(), fileName.begin(), ::tolower );

	if constexpr( ioType == BYTECODER || ioType == BYTECODEW )
	{
		if ( fileName.ends_with( txtExt ) || fileName.ends_with( binExt ) ||
			 fileName.ends_with( fileExt ) || fileName.ends_with( datExt ) )
		{
			return ioType;
		}
	} else if constexpr ( ioType == IMPLANT || ioType == EXTRACT )
	{
		if ( fileName.ends_with( bmpExt ) )
		{
			return BMP;
		} else if ( fileName.ends_with( wavExt ) )
		{
			return WAVE;
		} else
		{
			return INVALID;
		}
		
	}
}


int HandleCommandLine( char** argV )
{
	
	userInput = argV[ 1 ][ 1 ];

	if ( userInput != 'i' && userInput != 'e' && userInput != 'I' && userInput != 'E' )
	{
		Console::Error( "Invalid first argument, must be 'i' for implant or 'e' for extract" );
		HandleError( std::runtime_error( "Invalid first argument" ) );
		std::this_thread::sleep_for( 2s );
		return EXIT_FAILURE;
	}

	if ( userInput == 'i' || userInput == 'I' )
	{
		byteCodeRFile = argV[ 2 ];
		implantFile = argV[ 3 ];

		if ( VerifyFileExtension<BYTECODER>( byteCodeRFile ) != BYTECODER )
		{
			Console::Error( "Invalid bytecode file extension, must be .txt, .bin, .file or .dat" );
			HandleError( std::runtime_error( "Invalid bytecode file extension" ) );
			std::this_thread::sleep_for( 2s );
			return EXIT_FAILURE;
		}

		if ( cmdLineSelect = VerifyFileExtension<IMPLANT>( implantFile ); cmdLineSelect == INVALID )
		{
			Console::Error( "Invalid implant file extension, must be .bmp or .wav" );
			HandleError( std::runtime_error( "Invalid implant file extension" ) );
			std::this_thread::sleep_for( 2s );
			return EXIT_FAILURE;
		}

		byteReader = std::make_unique<FileReader>( byteCodeRFile );

		if ( cmdLineSelect == BMP )
		{
			bmpImplant = std::make_unique<BmpImplantation>( implantFile );
			bmpImplant->Write( byteReader->GetData() );
		} else if ( cmdLineSelect == WAVE )
		{
			waveImplant = std::make_unique<WaveImplantation>( implantFile );
			waveImplant->Write( byteReader->GetData() );
		}

		Console::Success( "Implantation completed successfully" );
		Console::Menu( "Closing App in 5 seconds..." );
		std::this_thread::sleep_for( 5s );
		return EXIT_SUCCESS;
	} else if ( userInput == 'e' || userInput == 'E' )
	{
		implantFile = argV[ 2 ];
		byteCodeWFile = argV[ 3 ];

		if ( cmdLineSelect = VerifyFileExtension<EXTRACT>( implantFile ); cmdLineSelect == INVALID )
		{
			Console::Error( "Invalid implant file extension, must be .bmp or .wav" );
			HandleError( std::runtime_error( "Invalid implant file extension" ) );
			std::this_thread::sleep_for( 2s );
			return EXIT_FAILURE;
		}

		if ( VerifyFileExtension<BYTECODEW>( byteCodeWFile ) != BYTECODEW )
		{
			Console::Error( "Invalid bytecode file extension, must be .txt, .bin, .file or .dat" );
			HandleError( std::runtime_error( "Invalid bytecode file extension" ) );
			std::this_thread::sleep_for( 2s );
			return EXIT_FAILURE;
		}

		if ( cmdLineSelect == BMP )
		{
			bmpImplant = std::make_unique<BmpImplantation>( implantFile );
			implantData = bmpImplant->Read();
		} else if ( cmdLineSelect == WAVE )
		{
			waveImplant = std::make_unique<WaveImplantation>( implantFile );
			implantData = waveImplant->Read();
		}

		byteWriter = std::make_unique<FileWriter>( byteCodeWFile, implantData );
		Console::Success( "Extraction completed successfully" );
		Console::Menu( "Closing App in 5 seconds..." );
		std::this_thread::sleep_for( 5s );
		return EXIT_SUCCESS;
	}
}




int main( int argC, char** argV )
{
	if ( firstRun )
	{
		std::set_terminate( UnhandledException );
		LogInfo::GetCurrentPath();
		Console::SetupConsole();
	}

	byteCodeRFile.resize( MAX_PATH );
	byteCodeWFile.resize( MAX_PATH );
	implantFile.resize( MAX_PATH );

	
	if ( argC == 0x02 && ( argV[ 1 ][ 1 ] == 'h' || argV[ 1 ][ 1 ] == 'H' ) )
	{
		PrintHelp();
		return EXIT_SUCCESS;
	} else if ( argC > 0x03 )
	{
		return HandleCommandLine( argV );
	}
	
	
	PrintMenu();

	GetUserInput();

	if ( userSelection == IMPLANT_BMP || userSelection == IMPLANT_WAVE )
	{
		GetFile<BYTECODER>( byteCodeRFile );
		byteReader = std::make_unique<FileReader>( byteCodeRFile );

		if ( byteReader->GetData().empty() )
		{
			ErrorClose( "Bytecode file is empty, exiting" );
		} else
		{
			Console::Success( "Bytecode file loaded successfully" );
			if ( AskToPrint<BYTECODER>() )
			{
				PrintBuffer( byteReader->GetData() );
			}
		}

		GetFile<IMPLANT>( implantFile );

		if ( userSelection == IMPLANT_BMP )
		{
			bmpImplant = std::make_unique<BmpImplantation>( implantFile );
			bmpImplant->Write( byteReader->GetData() );
		} else if ( userSelection == IMPLANT_WAVE )
		{
			waveImplant = std::make_unique<WaveImplantation>( implantFile );
			waveImplant->Write( byteReader->GetData() );
		} else
		{
			ErrorClose( "Invalid selection, exiting" );
		}

		SuccessReset<IMPLANT>();
	} else if ( userSelection == EXTRACT_BMP || userSelection == EXTRACT_WAVE )
	{
		GetFile<EXTRACT>( implantFile );

		if ( userSelection == EXTRACT_BMP )
		{
			bmpImplant = std::make_unique<BmpImplantation>( implantFile );
			implantData = bmpImplant->Read();
		} else if ( userSelection == EXTRACT_WAVE )
		{
			waveImplant = std::make_unique<WaveImplantation>( implantFile );
			implantData = waveImplant->Read();
		} else
		{
			ErrorClose( "Invalid selection, exiting" );
		}

		if ( implantData.empty() )
		{
			ErrorClose( "No implant data found, exiting" );
		} else
		{
			Console::Success( "Implant data extracted successfully" );
			if ( AskToPrint<EXTRACT>() )
			{
				PrintBuffer( implantData );
			}
		}

		GetFile<BYTECODEW>( byteCodeWFile );
		byteWriter = std::make_unique<FileWriter>( byteCodeWFile, implantData );
		SuccessReset<BYTECODEW>();
	} else
	{
		ErrorClose( "Invalid selection, exiting" );
	}

	ResetGlobals();
	Console::ResetConsole();

	return 0;
}



