#pragma once
#include <Windows.h>
#include <memory>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>




class TxtReader
{
private:
	std::unique_ptr<std::ifstream> txtFile;
	
public:

	::TxtReader(__in const std::string& filePath)
	{
		
		TxtReader::txtFile = std::make_unique<std::ifstream>(filePath, std::ios::binary | std::ios::beg);
		
		std::string tempBuffer((std::istreambuf_iterator<char>(*TxtReader::txtFile.get())), (std::istreambuf_iterator<char>()));

		std::istringstream hexStream(tempBuffer);
				
		std::uint16_t tempInt = 0;
		while (hexStream >> std::hex >> tempInt)
		{
			TxtReader::shellcode.push_back(tempInt);
		}
			
		TxtReader::shellcode.shrink_to_fit();
	}

	~TxtReader()
	{
		txtFile.get()->close();
	}

	

	std::vector <std::uint8_t> shellcode;

	std::size_t GetShellcodeSize() const
	{
		return TxtReader::shellcode.size();
	}
	
	void PrintShellcode() const
	{
		std::cout << "unsigned char Shellcode" << "[] = {";

		for (int i = 0; i < TxtReader::GetShellcodeSize(); i++)
		{

			if (i % 16 == 0)
			{
				std::cout << "\n\t";
			}
			if (i < TxtReader::GetShellcodeSize() - 1)
			{
				std::cout << std::format("0x{:X}, ", static_cast<std::uint16_t>(TxtReader::shellcode[i]));
			}
			else
			{
				std::cout << std::format("0x{:X} ", static_cast<std::uint16_t>(TxtReader::shellcode[i]));
			}
		}

		std::cout << "};\n\n\n";
	}

};