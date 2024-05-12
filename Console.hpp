#ifndef FRAMEWORK_HPP
#define FRAMEWORK_HPP

#pragma once
#include <Windows.h>
#include <iostream>


#define EPRINT( s ) dye.EPrint( s );
#define ERPRINT( s ) dye.ERPrint( s );
#define SPRINT( s ) dye.SPrint( s );
#define DPRINT( s , d ) dye.DPrint( s , d );
#define CLOSE( s , d , e ) dye.Close( s , d, e )

class DYE
{
private:
	HANDLE hConsole;
	DWORD oldConsoleMode;
public:
	::DYE() : hConsole(GetStdHandle(STD_OUTPUT_HANDLE)), oldConsoleMode(0)
	{
		GetConsoleMode(hConsole, &oldConsoleMode);
		SetConsoleMode(hConsole, ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
	}

	~DYE()
	{
		SetConsoleMode(hConsole, oldConsoleMode);
		CloseHandle(hConsole);
	}

	const char* red = "\x1B[31m";
	const char* green = "\x1B[32m";
	const char* reset = "\x1B[0m";
	const char* yellow = "\x1B[33m";
	const char* purple = "\x1B[35m";


	void EPrint(const char* input)
	{
		std::cout << red << "[!]" << reset << input << "\n";
	}

	void ERPrint(const char* input)
	{
		std::cout << red << "[!]" << reset << input << "\r";
	}

	void SPrint(const char* input)
	{
		std::cout << green << "[+]" << reset << input << "\n";
	}

	template<typename T>
	void DPrint(const char* input, T data)
	{
		std::cout << green << "[-]" << reset << input << " : " << data << "\n";
	}

	template<typename T>
	void Close(const char* input, T data, bool error)
	{
		if (!error)
		std::cout << green << "[+]" << input << " : " << data << reset << "\r";

		if (error)
			std::cout << red << "[!]" << input << " : " << data << reset << "\r";
	}
};

inline DYE dye;

#endif //!FRAMEWORK_HPP