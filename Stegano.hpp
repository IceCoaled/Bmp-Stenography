#pragma once
#include <Windows.h>
#include <memory>
#include <fstream>
#include <string>
#include <vector>


constexpr std::uint8_t EXIT_SIG[] = { 0x01, 0x0C, 0x0E, 0x0C, 0x00, 0x0A, 0x01, 0x0E, 0x0D , 0x05, 0x01, 0x06 };
constexpr auto BIT_24 = 0x18;
#define C_PTR( x ) ((void*) x)
#define U_MAX( x ) ((std::uintmax_t) x)


#define BM_SIGNATURE 0x4D42
#define COPYTOIMAGE 0x0000002554
#define COPYFROMIMAGE 0x0000003121


class Stegano
{
private:
	std::string fileName;
	std::unique_ptr<BITMAPFILEHEADER> fileHeader;
	std::unique_ptr<BITMAPV5HEADER> DIBHeader;
	std::unique_ptr<std::ifstream> readBmpFile;
	std::unique_ptr<std::ofstream> writeBmpFile;
	std::vector<std::uint8_t> imageBuffer;
	std::size_t szImage = 0;
	std::size_t bitMapOffset = 0;
	std::size_t szBitMap = 0;
	std::size_t rowStride = 0;
	std::size_t padding = 0;
	long imageWidth = 0;
	long imageHeight = 0;
	bool imageIsPadded = false;
	std::uint32_t readSwitch = 0;

private:
	bool CopyHeaders() const;
	bool DataChecks();
	__forceinline std::size_t StrideAlign(__in std::size_t alignStride) const;
	bool ReadNonPaddedBitMap();
	bool ReadPaddedBitMap();
	bool ReadData(__in std::vector<std::uint8_t>& externalBuffer) const;
	bool WriteData(__in std::vector<std::uint8_t>& externalBuffer, __in std::size_t szExternalBuffer);
	bool WriteNonPaddedBitMap();
	bool WritePaddedBitMap();

public:
	::Stegano(const std::string& file)
	{
		Stegano::readBmpFile = std::make_unique<std::ifstream>(file, std::ios::binary | std::ios::beg);
		Stegano::fileHeader = std::make_unique<BITMAPFILEHEADER>();
		Stegano::DIBHeader = std::make_unique<BITMAPV5HEADER>();
		
		Stegano::fileName.reserve(MAX_PATH);
		strcpy_s(Stegano::fileName.data(), MAX_PATH, file.data());
	}

	bool ImplantHiddenData(__in std::vector<std::uint8_t>& externalBuffer, __in std::size_t szExternalBuffer);
	bool ExtractHiddenData(__in std::vector<std::uint8_t>& externalBuffer);

};



using BMPSWITCH = enum _BMPSWITCH
{
	Padding,
	NonPadding,
};


