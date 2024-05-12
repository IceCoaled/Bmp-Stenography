#include "PeLoader.hpp"
#include "Console.hpp"

#include <iostream>
#include <Windows.h>


static void MemClean(std::vector<MEMALLOC> allocedMem)
{
	for (auto& it : allocedMem)
	{
		VirtualFree(it.mem, it.szMem, MEM_RELEASE);
	}
}



void PeLoader::MapPe(__in const std::string& fileName)
{	
	SPRINT("Fetching Handle To File")
	
	auto fileInfo = std::make_unique<FILE_STANDARD_INFO>();
	auto alloc1 = std::make_unique<MEMALLOC>();
	auto alloc2 = std::make_unique<MEMALLOC>();
	std::vector<MEMALLOC> Cleanup;
	void* allocMem = nullptr;
	void* mappedPe = nullptr;
	std::size_t szAlloc = 0;
	std::size_t length = 0;
	DWORD dwBytesRead = 0;
	auto dos_Header = PIMAGE_DOS_HEADER{ nullptr };
	auto nt_Header = PIMAGE_NT_HEADERS64{ nullptr };
	auto sectionHeader = PIMAGE_SECTION_HEADER{ nullptr };



	HANDLE hFile = CreateFileA(fileName.c_str(), FILE_GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (!hFile)
	{
		EPRINT("Error While Opening File")
		return;
	}

	SPRINT("Fetching File Size");

	if (!GetFileInformationByHandleEx(hFile, FileStandardInfo, fileInfo.get(), sizeof(FILE_STANDARD_INFO)))
	{
		EPRINT("Error While Fetching Filer Information")
		return;
	}

	szAlloc = fileInfo.get()->AllocationSize.QuadPart;
	length = szAlloc;
	
	allocMem = VirtualAlloc(nullptr, szAlloc, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!allocMem)
	{
		EPRINT("Error While Allocation Memory")
		return;
	}


	alloc1.get()->mem = allocMem;
	alloc1.get()->szMem = szAlloc;
	Cleanup.push_back(*alloc1.get());

	SPRINT("Reading File");

	
	if (!ReadFile(hFile, allocMem, length, &dwBytesRead, nullptr))
	{
		EPRINT("Error While Reading File, Cleaning Allocated Memory")
		goto cleanup;
	}

	dos_Header = (PIMAGE_DOS_HEADER)((PBYTE)allocMem);
	if (dos_Header->e_magic != IMAGE_DOS_SIGNATURE)
	{
		EPRINT("Mapped Pe Failed Dos Signature, Cleaning Allocated Memory")
		goto cleanup;
	}

	nt_Header = (PIMAGE_NT_HEADERS64)(((PBYTE)allocMem) + dos_Header->e_lfanew);
	if (nt_Header->Signature != IMAGE_NT_SIGNATURE)
	{
		EPRINT("Mapped Pe Failed Nt Signature, Cleaning Allocated Memory")
		goto cleanup;
	}

	SPRINT("Mapping Pe To Memory");

	length = nt_Header->OptionalHeader.SizeOfImage;

	mappedPe = VirtualAlloc(nullptr, length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!mappedPe)
	{
		EPRINT("Error While Allocation Memory, Cleaning Allocated Memory")
		goto cleanup;
	}

	
	alloc2.get()->mem = allocMem;
	alloc2.get()->szMem = nt_Header->OptionalHeader.SizeOfImage;
	Cleanup.push_back(*alloc2.get());

	std::memcpy(mappedPe, allocMem, nt_Header->OptionalHeader.SizeOfHeaders);


	sectionHeader = IMAGE_FIRST_SECTION(nt_Header);
	if (!sectionHeader)
	{
		EPRINT("Error While Fetching Section Header, Cleaning Allocated Memory")
		goto cleanup;
	}

	SPRINT("Copying Headers")

	for (auto i = 0; i < nt_Header->FileHeader.NumberOfSections; i++)
	{
		std::memcpy(C_PTR((U_MAX(mappedPe) + sectionHeader[i].VirtualAddress)),
			C_PTR((U_MAX(allocMem) + sectionHeader[i].PointerToRawData)),
			sectionHeader[i].SizeOfRawData);
	}

	SPRINT("Moving Pe Into Buffer")

	PeLoader::peBuffer.reserve(nt_Header->OptionalHeader.SizeOfImage);
	PeLoader::peBuffer.resize(nt_Header->OptionalHeader.SizeOfImage);
	std::memmove(peBuffer.data(), mappedPe, nt_Header->OptionalHeader.SizeOfImage);

	SPRINT("Success, Cleaning Allocated Memory")
	
cleanup:
	MemClean(Cleanup);
}