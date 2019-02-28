#include <windows.h>
#include <ImageHlp.h>
#include <assert.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <functional>

#include "LibraryLoader.h"

struct Defer {
	Defer(std::function<void(void)> func) {
		func_ = func;
	}
	~Defer() {
		func_();
	}
	std::function<void(void)> func_;
};

CLibraryLoader::CLibraryLoader(const std::string& fullPath)
{
	int pos = fullPath.find_last_of("/");
	path = fullPath.substr(0, pos + 1);
	name = fullPath.substr(pos + 1);
}


CLibraryLoader::~CLibraryLoader()
{
}

size_t CLibraryLoader::RvaToOffset(DWORD rva, PIMAGE_SECTION_HEADER sections, size_t NumberOfSections) {
	for (size_t i = 0; i < NumberOfSections; ++i) {
		if (rva >= sections[i].VirtualAddress) {
			return rva + sections[i].PointerToRawData - sections[i].VirtualAddress;
		}
	}
	return -1;
}

std::vector<std::wstring> CLibraryLoader::ListExports()
{
	PLOADED_IMAGE image = ImageLoad(name.c_str(), path.c_str());
	Defer imageDefer([&]() { ImageUnload(image); });
	unsigned long exportSize;
	PIMAGE_EXPORT_DIRECTORY exports = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(ImageDirectoryEntryToData(image->MappedAddress, false, 
		IMAGE_DIRECTORY_ENTRY_EXPORT, &exportSize));

	PDWORD addressOfNames = reinterpret_cast<PDWORD>(ImageRvaToVa(image->FileHeader, image->MappedAddress, exports->AddressOfNames, nullptr));
	for (size_t i = 0; i < exports->NumberOfNames; i++) {
		char* funcName = reinterpret_cast<char*>(ImageRvaToVa(image->FileHeader, image->MappedAddress, addressOfNames[i], nullptr));

		// Get number of wchars to allocate
		int charNumber = MultiByteToWideChar(CP_ACP, 0, funcName, -1, nullptr, 0);
		std::vector<WCHAR> preparedName(charNumber);
		MultiByteToWideChar(CP_ACP, 0, funcName, -1, preparedName.data(), charNumber);
		exportedFunctions.push_back(std::wstring(preparedName.data()));
	}
	return exportedFunctions;
}

FunctionReturnType CLibraryLoader::LoadFunction(const std::wstring& functionName)
{
	std::string full = path + name;
	HANDLE hFile = CreateFile(
		full.c_str(),
		GENERIC_EXECUTE | GENERIC_READ,
		FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	Defer fileDefer([&]() { CloseHandle(hFile); });

	if (hFile == INVALID_HANDLE_VALUE) {
		DWORD dw = GetLastError();
		return nullptr;
	}

	HANDLE mapHandle = CreateFileMapping(
		hFile,
		0,
		PAGE_EXECUTE_WRITECOPY,
		0,
		0,
		name.c_str());
	Defer mapDefer([&]() { CloseHandle(mapHandle); });

	if (mapHandle == nullptr) {
		return nullptr;
	}

	auto view = reinterpret_cast<PBYTE>(MapViewOfFile(
		mapHandle,
		FILE_MAP_EXECUTE | FILE_MAP_COPY,
		0,
		0,
		0
	));

	PLOADED_IMAGE image = ImageLoad(name.c_str(), path.c_str());
	Defer imageDefer([&]() { ImageUnload(image); });

	PIMAGE_SECTION_HEADER sections = image->Sections;
	ULONG sectionsNumber = image->NumberOfSections;
	unsigned long exportSize;
	PIMAGE_EXPORT_DIRECTORY exports = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(ImageDirectoryEntryToData(image->MappedAddress, false,
		IMAGE_DIRECTORY_ENTRY_EXPORT, &exportSize));

	size_t* addressOfNames = reinterpret_cast<size_t*>(ImageRvaToVa(image->FileHeader, image->MappedAddress, exports->AddressOfNames, nullptr));
	size_t* addressOfFuncs = reinterpret_cast<size_t*>(ImageRvaToVa(image->FileHeader, image->MappedAddress, exports->AddressOfFunctions, nullptr));
	short* addressOfOrdinals = reinterpret_cast<short*>(ImageRvaToVa(image->FileHeader, image->MappedAddress, exports->AddressOfNameOrdinals, nullptr));
	for (size_t i = 0; i < exportedFunctions.size(); ++i) {
		if (functionName == exportedFunctions[i]) {
			DWORD ord = addressOfOrdinals[i];
			DWORD funcRVA = addressOfFuncs[ord];
			int offset = RvaToOffset(funcRVA, sections, sectionsNumber);
			return reinterpret_cast<FunctionReturnType>(view + offset);
		}
	}
	return nullptr;
}