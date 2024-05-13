#include "Stegano.hpp"
#include "TxtReader.hpp"
#include "PeLoader.hpp"
#include "Console.hpp"

#include <iostream>
#include <thread>
#include <conio.h>
#include <format>
using namespace std::chrono_literals;


#define P_LKEY 0x70
#define SPACE_KEY 0x20
#define NUM_ONE 0x31
#define NUM_TWO 0x32
#define NUM_THREE 0x33


static void PrintBuffer(__in std::vector<std::uint8_t> bytes)
{
	
	for (int i = 0; i < bytes.size(); i++)
	{

		if (i % 16 == 0)
		{
			std::cout << "\n\t";
		}
		if (i < bytes.size() - 1)
		{
			std::cout << std::format("0x{:X}, ", static_cast<std::uint16_t>(bytes[i]));
		}
		else
		{
			std::cout << std::format("0x{:X} ", static_cast<std::uint16_t>(bytes[i]));
		}
	}
}






int main(int argc, char** argv)
{
    auto steg = std::unique_ptr<Stegano>();
	
	std::string imageFileName; imageFileName.reserve(MAX_PATH);
	std::string imageFilePath; imageFilePath.reserve(MAX_PATH);

	OPENFILENAMEA ofn{ 0 };
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = GetConsoleWindow();
	ofn.lpstrFilter = ".bmp";
	ofn.lpstrFile = &imageFilePath[0];
	ofn.lpstrFileTitle = &imageFileName[0];
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = "Select a Image";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	
    
    if (argv[1])
    {
		std::string argv1Image = argv[1];
		std::string bmpCheck = argv1Image.substr(argv1Image.find_last_of('.'));
		if (std::strcmp(bmpCheck.c_str(), ".bmp") != 0)
		{
			EPRINT("Invalid Usage: Requires Bmp Image File")
			std::this_thread::sleep_for(3000ms);
			return -1;
		}
		steg = std::make_unique<Stegano>(argv[1]);
		
    }
    else
    {
		SPRINT("Please Select BMP Image")
		std::this_thread::sleep_for(3000ms);
		
		if (!GetOpenFileNameA(&ofn))
		{
			EPRINT("Failed To Open File Dialog")
			return -1;
		}
		steg = std::make_unique<Stegano>(imageFilePath);
    }

	auto input = 0;
	int userSelection = 0;
	
	SPRINT("Please Select To >>Implant<< Or >>Extract<<")
	std::cout << "\n";
	std::cout << dye.yellow << "1) Implant Shellcode\n\n" << dye.reset;
	std::cout << dye.yellow << "2) Implant PE\n\n" << dye.reset;
	std::cout << dye.yellow << "3) Extract\n\n" << dye.reset;

	
	while (input == NULL) 
	{
		input = _getch();

		if (input == NUM_ONE)
		{
			userSelection = 1;
			break;
		}
		else if (input == NUM_TWO)
		{
			userSelection = 2;
			break;
		}
		else if (input == NUM_THREE)
		{
			userSelection = 3;
			break;
		}
		else
		{
			ERPRINT("Invalid Input, Must Be 1 Or 2")
			std::this_thread::sleep_for(1000ms);
			std::cout << "                                                  \r";
			input = NULL;
		}
	}


	std::string fileName; fileName.reserve(MAX_PATH);
	std::string filePath; filePath.reserve(MAX_PATH);

	
	if (userSelection == 1)
	{
		SPRINT("Please Select A Text Docment With Byte Code")
		std::this_thread::sleep_for(3000ms);

		ofn.lpstrFilter = ".txt";
		ofn.lpstrTitle = "Select a Text Document";
		ofn.lpstrFile = &filePath[0];
		ofn.lpstrFileTitle = &fileName[0];

		if (!GetOpenFileNameA(&ofn))
		{
			EPRINT("Failed To Open File Dialog")
			return -1;
		}

		
		auto docsReader = std::make_unique<TxtReader>(filePath);
		if (docsReader.get()->GetShellcodeSize() == 0)
		{
			EPRINT("Error While Reading Shellcode")
			for (auto i = 5; i >= 0; i--)
			{
				CLOSE("Closing App In", i, true);
				std::this_thread::sleep_for(1000ms);
			}
			_exit(0x0);
		}
		DPRINT("Size Of Shellcode", docsReader.get()->GetShellcodeSize())
	

		SPRINT("Press >>P<< To Print Shellcode Or >>SPACE<< To Implant")
		do
		{
			
			if (_getch() == P_LKEY)
			{
				docsReader.get()->PrintShellcode();
				break;
			}
			else if (_getch() == SPACE_KEY) break;
			else if(_getch() != SPACE_KEY || _getch() != P_LKEY)
			{
				ERPRINT("Invalid Input, Must Be P Or SPACE")
				std::this_thread::sleep_for(1000ms);
				std::cout << "                                                  \r";
			}


		} while (true);
		
		if (!steg.get()->ImplantHiddenData(docsReader.get()->shellcode, docsReader.get()->GetShellcodeSize()))
		{
			for (auto i = 5; i >= 0; i--)
			{
				CLOSE("Closing App In", i, true);
				std::this_thread::sleep_for(1000ms);
			}
			_exit(0x0);
		}
		else goto end;
	}
	else if (userSelection == 2)
	{
		SPRINT("Please Select A PE")
		std::this_thread::sleep_for(3000ms);

		ofn.lpstrFilter = "exe files (*.exe)|*.exe| dll files (*.dll)|*.dll | sys files (*.sys)|*.sys";
		ofn.lpstrTitle = "Select a Executable";
		ofn.lpstrFile = &filePath[0];
		ofn.lpstrFileTitle = &fileName[0];

		if (!GetOpenFileNameA(&ofn))
		{
			EPRINT("Failed To Open File Dialog")
			return -1;
		}
		bool succcessCheck = false;
		auto peLoader = std::make_unique<PeLoader>(filePath, &succcessCheck);
		if (!succcessCheck)
		{
			for (auto i = 5; i >= 0; i--)
			{
				CLOSE("Closing App In", i, true);
				std::this_thread::sleep_for(1000ms);
			}
			_exit(0x0);
		}
		DPRINT("Size Of PE", peLoader.get()->GetSize())
	
		
		SPRINT("Press >>P<< To Print PE Data(NOT RECOMMENDED) Or >>SPACE<< To Implant")
		do
		{
			if (_getch() == P_LKEY)
			{
				peLoader.get()->Print();
				break;
			}
			else if (_getch() == SPACE_KEY) break;
			else if (_getch() != SPACE_KEY || _getch() != P_LKEY)
			{
				ERPRINT("Invalid Input, Must Be P Or SPACE")
					std::this_thread::sleep_for(1000ms);
				std::cout << "                                                  \r";
			}

		} while (true);

		if (!steg.get()->ImplantHiddenData(peLoader.get()->peBuffer, peLoader.get()->GetSize()))
		{
			for (auto i = 5; i >= 0; i--)
			{
				CLOSE("Closing App In", i, true);
				std::this_thread::sleep_for(1000ms);
			}
			_exit(0x0);
		}
		else goto end;
	}
	else if (userSelection == 3)
	{
		std::vector<std::uint8_t> extractBuffer;
		
		if (!steg.get()->ExtractHiddenData(extractBuffer))
		{
			for (auto i = 5; i >= 0; i--)
			{
				CLOSE("Closing App In", i, true);
				std::this_thread::sleep_for(1000ms);
			}
			_exit(0x0);
		}

		extractBuffer.shrink_to_fit();
		DPRINT("Size Of Extracted Data", extractBuffer.size())


		SPRINT("Press >>P<< To Print Extracted Data(NOT RECOMMENDED UNLESS ITS SHELLCODE) Or >>SPACE<< To Close APP")
		do
		{
			if (_getch() == P_LKEY)
			{
				PrintBuffer(extractBuffer);
				break;
			}
			else if (_getch() == SPACE_KEY) break;
			else if (_getch() != SPACE_KEY || _getch() != P_LKEY)
			{
				ERPRINT("Invalid Input, Must Be P Or SPACE")
				std::this_thread::sleep_for(1000ms);
				std::cout << "                                                  \r";
			}

		} while (true);
	}

	std::cout << "\n";
	std::cout << "\n";
	std::cout << "\n";

end:
	
	for (auto i = 5; i >= 0; i--)
	{
		CLOSE("Closing App In", i, false);
		std::this_thread::sleep_for(1000ms);
	}
	_exit(0x0);
}