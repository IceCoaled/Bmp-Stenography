#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <iostream>


#define C_PTR( x ) ((void*) x)
#define U_MAX( x ) ((std::uintmax_t) x)

class PeLoader
{
private:
	void MapPe(__in const std::string& fileName);

public:
	::PeLoader(__in const std::string& fileName)
	{
		MapPe(fileName);
	}

	std::vector<std::uint8_t> peBuffer;

	std::size_t GetSize() const
	{
		return PeLoader::peBuffer.size();
	}

	void Print() const
	{
		std::cout << "unsigned char Shellcode" << "[] = {";

		for (int i = 0; i < PeLoader::GetSize(); i++)
		{

			if (i % 16 == 0)
			{
				std::cout << "\n\t";
			}
			if (i < PeLoader::GetSize() - 1)
			{
				std::cout << std::format("0x{:X}, ", static_cast<std::uint16_t>(PeLoader::peBuffer[i]));
			}
			else
			{
				std::cout << std::format("0x{:X} ", static_cast<std::uint16_t>(PeLoader::peBuffer[i]));
			}
		}

		std::cout << "};\n\n\n";
	}
};


using MEMALLOC = struct _MEM_ALLOC
{
	void* mem;
	std::size_t szMem;
};