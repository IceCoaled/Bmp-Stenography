#include "Stegano.hpp"
#include "Console.hpp"

#include <iostream>
#include <future>
using namespace std::chrono_literals;


bool Stegano::ExtractHiddenData(__in std::vector<std::uint8_t>& externalBuffer)
{
	
	if (!Stegano::readBmpFile.get()->is_open())
	{
		EPRINT("Error While Opening File")
		return false;
	}
	
	if (!Stegano::CopyHeaders()) return false;

	if (!Stegano::DataChecks()) return false;

	SPRINT("Reading BitMap")
	
	switch (Stegano::readSwitch)
	{
	case NonPadding:
		if (!Stegano::ReadNonPaddedBitMap()) return false;
		SPRINT("Success, BitMap Loaded In Buffer")
		
		SPRINT("Reading Hidden Data")
		if (!Stegano::ReadData(externalBuffer)) return false;
		break;
	case Padding:
		if (!Stegano::ReadPaddedBitMap()) return false;
		SPRINT("Success, BitMap Loaded In Buffer")
		
		SPRINT("Reading Hidden Data");
		if (!Stegano::ReadData(externalBuffer)) return false;
		break;
	default:
		break;
	}
		
	SPRINT("Success, Data Copied To External Buffer")
	return true;
}



bool Stegano::ImplantHiddenData(__in std::vector<std::uint8_t>& externalBuffer, __in std::size_t szExternalBuffer)
{

	if (!Stegano::readBmpFile.get()->is_open())
	{
		EPRINT("Error While Opening File")
		return false;
	}
	
	if (!Stegano::CopyHeaders()) return false;

	if (!Stegano::DataChecks()) return false;

	SPRINT("Reading BitMap")

	switch (Stegano::readSwitch)
	{
	case NonPadding:
		if (!Stegano::ReadNonPaddedBitMap()) return false;
		SPRINT("Success, BitMap Loaded In Buffer")

		SPRINT("Writing Hidden Data")
		if (!Stegano::WriteData(externalBuffer, szExternalBuffer)) return false;

		if (!WriteNonPaddedBitMap()) return false;
		break;
	case Padding:
		if (!Stegano::ReadPaddedBitMap()) return false;
		SPRINT("Success, BitMap Loaded In Buffer");

		SPRINT("Writing Hidden Data");
		if (!Stegano::WriteData(externalBuffer, szExternalBuffer)) return false;

		if (!WritePaddedBitMap()) return false;
		break;
	default:
		break;
	}

	SPRINT("Success, Updated Image");
	return true;
}





bool Stegano::CopyHeaders() const
{
	
	Stegano::readBmpFile.get()->read((PCHAR)Stegano::fileHeader.get(), sizeof(BITMAPFILEHEADER));
	if (!Stegano::readBmpFile.get()->fail())
	{
		SPRINT("Success, Copied Bmp File Header");
	}
	else
	{
		EPRINT("Failed To Copy File Header");
		return false;
	}

	Stegano::readBmpFile.get()->read((PCHAR)Stegano::DIBHeader.get(), sizeof(BITMAPV5HEADER));
	if (!Stegano::readBmpFile.get()->fail())
	{
		SPRINT("Success, Copied Bmp DIB Header");
	}
	else
	{
		EPRINT("Failed To Copy DIB Header")
		return false;
	}
	
	return true; 
}



bool Stegano::DataChecks()
{
	if (Stegano::fileHeader.get()->bfType != BM_SIGNATURE)
	{
		EPRINT("Image Failed BM Signature Check, File Is Unsupported")
		return false;
	}
	
	if (Stegano::DIBHeader.get()->bV5Compression != BI_RGB)
	{
		EPRINT("Image Has Been Compressed, File Is Unsupported")
		return false;
	}


	if (Stegano::DIBHeader.get()->bV5BitCount != BIT_24)
	{
		EPRINT("Image Isnt 32Bit Or 24Bit, File Is Unsupported")
		return false;
	}
	

	Stegano::imageWidth = Stegano::DIBHeader.get()->bV5Width;
	Stegano::imageHeight = Stegano::DIBHeader.get()->bV5Height;
	Stegano::szImage = Stegano::fileHeader.get()->bfSize;
	Stegano::bitMapOffset = Stegano::fileHeader.get()->bfOffBits;
	

	DPRINT("Image Pixel Width", Stegano::imageWidth);
	DPRINT("Image Pixel Height", Stegano::imageHeight);
	DPRINT("Size Of Image", Stegano::szImage)
	DPRINT("Offset To BitMap", Stegano::bitMapOffset)


	if (Stegano::imageWidth % 4 == 0)
	{
		SPRINT("BitMap Isnt Padded")
		Stegano::imageIsPadded = false;
	}
	else
	{
		Stegano::rowStride = (Stegano::imageWidth * BIT_24) / 0x08;
		std::size_t newStride = Stegano::StrideAlign(0x04);
		Stegano::padding = newStride - Stegano::rowStride;
		
		DPRINT("BitMap Is Padded With", Stegano::padding)
		Stegano::imageIsPadded = true;
	}


	Stegano::szBitMap = (Stegano::imageWidth * Stegano::imageHeight * BIT_24 / 0x08);
	DPRINT("Size Of BitMap", Stegano::szBitMap)

	SPRINT("Resizing ImageBuffer")
	Stegano::imageBuffer.reserve(Stegano::szBitMap);
	Stegano::imageBuffer.resize(Stegano::szBitMap);
	

	if (Stegano::imageIsPadded == false) Stegano::readSwitch = NonPadding;
	if (Stegano::imageIsPadded == true) Stegano::readSwitch = Padding;
	

	return true;
}




bool Stegano::ReadNonPaddedBitMap()
{

	Stegano::readBmpFile.get()->seekg(Stegano::bitMapOffset, std::ios::beg);
	if (Stegano::readBmpFile.get()->bad())
	{
		EPRINT("Error While Moving Read Pointer")
		return false;
	}
	

	Stegano::readBmpFile.get()->read((PCHAR)Stegano::imageBuffer.data(), Stegano::szBitMap);
		
	if (Stegano::readBmpFile.get()->fail())
	{
		EPRINT("Error While Reading BitMap")
		return false;
	}
		
	return true;
}




bool Stegano::ReadPaddedBitMap()
{
	Stegano::readBmpFile.get()->seekg(Stegano::bitMapOffset, std::ios::beg);
	if (Stegano::writeBmpFile.get()->bad())
	{
		EPRINT("Error While Moving Read Pointer")
		return false;
	}

	std::vector<std::uint8_t> paddingBytes(Stegano::padding);

	for (std::size_t row = 0; row < Stegano::imageHeight; row++)
	{
			
		Stegano::readBmpFile.get()->read((PCHAR)(Stegano::imageBuffer.data() + Stegano::rowStride * row), Stegano::rowStride);
		if (Stegano::readBmpFile.get()->fail())
		{
			EPRINT("Error While Reading BitMap")
			return false;
		}

		Stegano::readBmpFile.get()->read((PCHAR)paddingBytes.data(), paddingBytes.size());
		if (Stegano::readBmpFile.get()->fail())
		{
			EPRINT("Error While Reading BitMap Padding")
			return false;
		}
	}
	
	return true;
}


///static function for multi threading
static bool SigFinder(__in const std::uint8_t* imageBufferMem, __in std::size_t szImageBufferMem, __out std::size_t* result)
{
	for (std::size_t it = 0; it < szImageBufferMem; it++)
	{
		if (imageBufferMem[it] == EXIT_SIG[0] &&
			std::memcmp(&imageBufferMem[it], EXIT_SIG, sizeof(EXIT_SIG)) == 0)
		{
		
			*result = it - 1;
			return true;
			
		}
	}
	return false;
}
/// 


bool Stegano::ReadData(__in std::vector<std::uint8_t>& externalBuffer) const
{ 
	std::size_t sigLoc = 0;
	auto future = std::async(std::launch::async, SigFinder, Stegano::imageBuffer.data(), Stegano::imageBuffer.size(), &sigLoc);
	
	std::size_t endOfBuffer = Stegano::imageBuffer.size() - sizeof(EXIT_SIG);
	for (std::size_t it = 0; it < endOfBuffer; it++)
	{
		
		if (endOfBuffer != sigLoc)
		{
			std::this_thread::sleep_for(50ns);
			if (future._Is_ready())
			{
				if (!future._Get_value())
				{
					EPRINT("Error While Finding Exit Signature")
					return false;
				}

				endOfBuffer = sigLoc;
			}
		}

		externalBuffer.push_back(Stegano::imageBuffer[it]);
	}

	return true;
}



bool Stegano::WriteData(__in std::vector<std::uint8_t>& externalBuffer, __in std::size_t szExternalBuffer)
{
	if ((szExternalBuffer + sizeof(EXIT_SIG)) > Stegano::imageBuffer.size())
	{
		EPRINT("Data Will Not Fit In Image")
		return false;
	}
	
	
	std::size_t it = 0;
	for (; it < szExternalBuffer; it++)
	{	
		Stegano::imageBuffer[it] = externalBuffer[it];
	}

	it++;

	for (std::size_t sig = 0; sig < sizeof(EXIT_SIG); sig++, it++)
	{
		Stegano::imageBuffer[it] = EXIT_SIG[sig];
	}

	return true;
}



bool Stegano::WriteNonPaddedBitMap()
{
	SPRINT("Closing Read Stream")
	Stegano::readBmpFile.get()->close();
	
	SPRINT("Opening Write Stream")
	Stegano::writeBmpFile = std::make_unique<std::ofstream>(Stegano::fileName, std::ios::binary | std::ios::beg);
	if (!*Stegano::writeBmpFile.get())
	{
		EPRINT("Error While Opening File")
		return false;
	}

	SPRINT("Writing Headers")
	Stegano::writeBmpFile.get()->write((LPCSTR)Stegano::fileHeader.get(), sizeof(BITMAPFILEHEADER));
	if (Stegano::writeBmpFile.get()->fail())
	{
		EPRINT("Error While Writing File Header, Closing Write Stream")
		Stegano::writeBmpFile.get()->close();
		return false;
	}


	Stegano::writeBmpFile.get()->write((LPCSTR)Stegano::DIBHeader.get(), sizeof(BITMAPV5HEADER));
	if (Stegano::writeBmpFile.get()->fail())
	{
		EPRINT("Error While Writing DIB Header, Closing Write Stream")
		Stegano::writeBmpFile.get()->close();
		return false;
	}
	
	SPRINT("Writing New BitMap")
	Stegano::writeBmpFile.get()->seekp(Stegano::bitMapOffset, std::ios::beg);
	if (Stegano::writeBmpFile.get()->bad())
	{
		EPRINT("Error While Moving Write Pointer, Closing Write Stream")
		Stegano::writeBmpFile.get()->close();
		return false;
	}

	Stegano::writeBmpFile.get()->write((PCHAR)Stegano::imageBuffer.data(), imageBuffer.size());
	if (Stegano::writeBmpFile.get()->fail())
	{
		EPRINT("Error While Writing BitMap, Closing Write Stream")
		Stegano::writeBmpFile.get()->close();
		return false;
	}

	SPRINT("Success, Wrote BitMap To File, Closing Write Stream")
	Stegano::writeBmpFile.get()->close();
	return true;
}



bool Stegano::WritePaddedBitMap()
{
	SPRINT("Closing Read Stream")
	Stegano::readBmpFile.get()->close();

	SPRINT("Opening Write Stream")
	Stegano::writeBmpFile = std::make_unique<std::ofstream>(Stegano::fileName);

	SPRINT("Writing Headers")
	Stegano::writeBmpFile.get()->write((LPCSTR)Stegano::fileHeader.get(), sizeof(BITMAPFILEHEADER));
	if (Stegano::writeBmpFile.get()->fail())
	{
		EPRINT("Error While Writing File Header, Closing Write Stream")
		Stegano::writeBmpFile.get()->close();
		return false;
	}
	Stegano::writeBmpFile.get()->write((LPCSTR)Stegano::DIBHeader.get(), sizeof(BITMAPV5HEADER));
	if (Stegano::writeBmpFile.get()->fail())
	{
		EPRINT("Error While Writing DIB Header, Closing Write Stream")
		Stegano::writeBmpFile.get()->close();
		return false;
	}

	SPRINT("Writing New BitMap")
	Stegano::writeBmpFile.get()->seekp(Stegano::bitMapOffset);
	if (Stegano::writeBmpFile.get()->bad())
	{
		EPRINT("Error While Moving Write Pointer, Closing Write Stream")
		Stegano::writeBmpFile.get()->close();
		return false;
	}


	std::vector<std::uint8_t> paddingBytes(Stegano::padding);

	for (std::size_t row = 0; row < Stegano::imageHeight; row++)
	{

		Stegano::writeBmpFile.get()->write((LPCSTR)(Stegano::imageBuffer.data() + Stegano::rowStride * row), Stegano::rowStride);

		if (!std::basic_ofstream<char>::failbit)
		{
			EPRINT("Error While Writing BitMap, Closing Write Stream")
			Stegano::writeBmpFile.get()->close();
			return false;
		}


		Stegano::writeBmpFile.get()->write((LPCSTR)paddingBytes.data(), paddingBytes.size());
		if (!std::basic_ofstream<char>::failbit)
		{
			EPRINT("Error While Writing BitMap Padding, Closing Write Stream")
			Stegano::writeBmpFile.get()->close();
			return false;
		}
	}
	
	SPRINT("Success, Wrote BitMap To File, Closing Write Stream");
	Stegano::writeBmpFile.get()->close();
	return true;
}



__forceinline std::size_t Stegano::StrideAlign(__in std::size_t alignStride) const
{
	std::size_t newStride = Stegano::rowStride;
	while (newStride % alignStride != 0)
	{
		newStride++;
	}

	return newStride;
}