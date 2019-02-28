#pragma once

#include <string>
#include <vector>

typedef int(*FunctionReturnType)(int);

class CLibraryLoader
{
public:
	CLibraryLoader(const std::string& fullPath);
	~CLibraryLoader();

	std::vector<std::wstring> ListExports();
	FunctionReturnType LoadFunction(const std::wstring& functionName);
private:
	size_t RvaToOffset(DWORD rva, PIMAGE_SECTION_HEADER sections, size_t NumberOfSections);

	std::string path;
	std::string name;
	std::vector<std::wstring> exportedFunctions;
};

