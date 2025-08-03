#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <Windows.h>
#include <print>

/**
* @brief Console utility class for printing messages with colors and formatting.
*/



namespace Console
{
	namespace Backend
	{
		template<class T>
		concept isString = ( std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> );
		
		template<class T>
		concept isStringRef = ( std::is_same_v<T, std::string&> || std::is_same_v<T, std::string_view&> );

		template<class T>
		concept isChar = ( std::is_same_v<T, const char*> || std::is_same_v<T, const char* const> );

		template<class T>
		concept isCharArray = std::is_array_v<T> && std::is_same_v<std::remove_extent_t<T>, const char>;

		template<class T>
		concept TString = ( isString<T> || isStringRef<T> || isChar<T> || isCharArray<T> );

		template<class T>
		concept isStdString = ( TString<T> && ( !isChar<T> || !isCharArray<T> ) );
		
		HANDLE hConsole;
		DWORD dwMode;

		static constexpr const char* RED = "\x1B[31m";
		static constexpr const char* GREEN = "\x1B[32m";
		static constexpr const char* YELLOW = "\x1B[33m";
		static constexpr const char* BLUE = "\x1B[34m";
		static constexpr const char* MAGENTA = "\x1B[35m";
		static constexpr const char* CYAN = "\x1B[36m";
		static constexpr const char* RESET = "\x1B[0m";

		static constexpr const char* SUCCESS = "[+]: ";
		static constexpr const char* WARNING = "[-]: ";
		static constexpr const char* ERR0R = "[!]: ";  // This is on purpose as ERROR is already defined in Windows headers


		// Overload for messages without additional values
		template<TString Str>
		static inline auto Print( const char* type, const char* color, Str msg ) -> void
		{
			std::println( "{}{}{}{}", color, type, msg, RESET );
		}

		template<TString Str, class T>
		static inline auto PrintValue( const char* type, const char* color, Str msg, T&& value ) -> void
		{
			std::println( "{}{}{}: {}{}", color, type, msg, std::forward<T>( value ), RESET );
		}

	}
	
	
	

	/**
	* @brief Sets up the console for colored output and virtual terminal processing.
	*/
	static inline void SetupConsole()
	{
		using namespace Backend;

		hConsole = GetStdHandle( STD_OUTPUT_HANDLE );

		if ( hConsole == INVALID_HANDLE_VALUE )
		{
			throw std::runtime_error( "Failed to get console handle." );
		}

		if ( !GetConsoleMode( hConsole, &dwMode ) )
		{
			throw std::runtime_error( "Failed to get console mode." );
		}

		if ( !SetConsoleMode( hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT ) )
		{
			throw std::runtime_error( "Failed to set console mode." );
		}
	}

	static inline void ResetConsole()
	{
		using namespace Backend;
		if ( hConsole != INVALID_HANDLE_VALUE )
		{
			SetConsoleMode( hConsole, dwMode );
			CloseHandle( hConsole );
		}
	}

	static inline void DeleteLastLine()
	{
		using namespace Backend;
		std::print( "\x1B[1A\x1B[2K" ); 
	}

	static inline auto ClearConsole() -> void
	{
		system( "cls" ); 
	}

	template<Backend::TString Str>
	static inline auto Success( Str& msg ) -> void
	{
		using namespace Backend;
		Print( SUCCESS, GREEN, msg );
	}

	template<Backend::TString Str>
	static inline auto Success( Str& msg, const auto& value ) -> void
	{
		using namespace Backend;
		PrintValue( SUCCESS, GREEN, msg, value );
	}

	template<Backend::TString Str>
	static inline auto Warning( Str& msg ) -> void
	{
		using namespace Backend;
		Print( WARNING, YELLOW, msg );
	}

	template<Backend::TString Str>
	static inline auto Warning( Str& msg, const auto& value ) -> void
	{
		using namespace Backend;
		PrintValue( WARNING, YELLOW, msg, value );
	}

	template<Backend::TString Str>
	static inline auto Error( Str& msg ) -> void
	{
		using namespace Backend;
		Print( ERR0R, RED, msg );
	}

	template<Backend::TString Str>
	static inline auto Error( Str& msg, const auto& value ) -> void
	{
		using namespace Backend;
		PrintValue( ERR0R, RED, msg, value );
	}
	
	template<Backend::TString Str>
	static inline auto Menu( Str& msg ) -> void
	{
		using namespace Backend;
		std::println( "{}{}{}", MAGENTA, msg, RESET );
	}
};


/**
* @brief Log information for log path and file creation.
*/
namespace LogInfo
{
	static std::filesystem::path logFilePath;

	static void GetCurrentPath()
	{
		static std::filesystem::path currentPath = std::filesystem::current_path();
		if ( !std::filesystem::exists( currentPath ) )
		{
			throw std::runtime_error( "Current path does not exist." );
		}
		logFilePath = currentPath += "/errorLog.txt";
		if ( !std::filesystem::exists( logFilePath ) )
		{
			std::ofstream logFile( logFilePath );
			if ( !logFile.is_open() )
			{
				throw std::runtime_error( "Failed to create log file." );
			} else
			{
				logFile.close();
			}
		}
	}

};


/**
* @brief Handles exceptions by logging the error message to a file and printing it to the console.
* Then exits the application with a failure status.
*/
template< class Exp>
requires std::derived_from<std::remove_cvref_t<Exp>, std::exception>
[[noreturn]] static void HandleError( const Exp&& exp )
{
	using namespace LogInfo;
	try
	{
		throw exp;
	} catch ( const Exp& e )
	{
		std::string errorMessage = e.what();
		Console::Error( errorMessage );
		std::ofstream logFile( logFilePath, std::ios::app );
		if ( !logFile.is_open() )
		{
			throw std::runtime_error( "Failed to open log file for writing." );
		} else
		{			
			logFile << std::endl << std::format( "TimeStamp: {:%Y-%m-%d, %H:%M} -\n",
			std::chrono::zoned_time{ std::chrono::current_zone(), std::chrono::system_clock::now() } ) << ": " << errorMessage << std::endl;
		}
		logFile.close();
		_exit( EXIT_FAILURE );
	}
}

#endif // !CONSOLE_HPP

