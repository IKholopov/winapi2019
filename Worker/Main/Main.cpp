#include "Manager.h"

#include <assert.h>
#include <iostream>
#include <locale>
#include <codecvt>
#include <string>

int main(int argc, char* argv[])
{
	assert(argc == 5);
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring input = converter.from_bytes(argv[1]);
	std::wstring dict = converter.from_bytes(argv[2]);
	int threshold = std::stoi(argv[3]);
	std::wstring output = converter.from_bytes(argv[4]);

	SECURITY_ATTRIBUTES attrs = {};
	attrs.nLength = sizeof(attrs);
	attrs.bInheritHandle = TRUE;
	attrs.lpSecurityDescriptor = 0;

	CManager manager(4);

	HANDLE hInput = CreateFile(
		input.data(),
		GENERIC_READ,
		NULL,
		&attrs,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	DWORD read;
	std::string textBuffer(BUFFERSIZE, ' ');
	bool success = ReadFile(hInput, (LPVOID)textBuffer.data(), BUFFERSIZE - 1, &read, NULL);
	if (!success) {
		_tprintf(TEXT("Could not open input. GLE=%d\n"), GetLastError());
		return -1;
	}
	textBuffer.resize(read);
	size_t partCount = 8;
	size_t part = (textBuffer.length() + partCount - 1) / partCount;
	std::vector<HANDLE> hParts(partCount);

	for (size_t i = 0; i < partCount; ++i) {
		std::wstring name = L"tmp" + std::to_wstring(i) + L".txt";
		hParts[i] = CreateFile(name.data(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			&attrs,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		bool success = WriteFile(hParts[i], textBuffer.data() + i * part, part, &read, NULL);
		if (!success) {
			_tprintf(TEXT("Could not write to threshold file. GLE=%d"), GetLastError());
			return -1;
		}
	}
	std::string buffer(BUFFERSIZE, ' ');
	 
	HANDLE hDict = CreateFile(dict.data(),
		FILE_ALL_ACCESS,
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		&attrs,                  
		OPEN_EXISTING,            
		FILE_ATTRIBUTE_NORMAL, 
		NULL);                 

	HANDLE hThreshold = CreateFile(L"thr.txt",               
		FILE_ALL_ACCESS,        
		FILE_SHARE_READ | FILE_SHARE_WRITE,                     
		&attrs,                  
		CREATE_ALWAYS,            
		FILE_ATTRIBUTE_NORMAL, 
		NULL);                 
	std::string num = std::to_string(threshold);
	success = WriteFile(hThreshold, num.data(), num.length(), &read, NULL);
	if (!success) {
		_tprintf(TEXT("Could not write to thr. GLE=%d"), GetLastError());
		return -1;
	}
	DWORD dwPtr = SetFilePointer(hThreshold,
		NULL,
		NULL,
		FILE_BEGIN);

	std::vector<CTaskOperation> operations;
	bool words = true;
	for (auto& handle : hParts) {
		operations.emplace_back(words ? FilterWords : FilterLetters, handle, words ? hDict : hThreshold);
		words = !words;
	}
	
	for (auto& op : operations) {
		manager.ScheduleTask(op);
	}

	manager.Close();

	std::string partBuffer(part, ' ');
	std::string result;
	for (size_t i = 0; i < partCount; ++i) {
		DWORD dwPtr = SetFilePointer(hParts[i],
			NULL,
			NULL,
			FILE_BEGIN);
		partBuffer.resize(part, ' ');
		bool success = ReadFile(hParts[i], (LPVOID)partBuffer.data(), part, &read, NULL);
		partBuffer.resize(read);
		result += partBuffer;
		if (!success) {
			_tprintf(TEXT("Could read part result. GLE=%d\n"), GetLastError());
			return -1;
		}
	}

	HANDLE hOut = CreateFile(output.data(),               
		GENERIC_WRITE,        
		NULL,                     
		&attrs,                   
		CREATE_ALWAYS,            
		FILE_ATTRIBUTE_NORMAL, 
		NULL);                  
	success = WriteFile(hOut, result.data(), result.length(), &read, NULL);
	if (!success) {
		_tprintf(TEXT("Could not write result. GLE=%d\n"), GetLastError());
		return -1;
	}
	
	for (size_t i = 0; i < partCount; ++i) {
		std::wstring name = L"tmp" + std::to_wstring(i) + L".txt";
		DeleteFile(name.data());
	}
	
	for (auto& handle : hParts) {
		CloseHandle(handle);
	}
	CloseHandle(hInput);
	CloseHandle(hOut);
	CloseHandle(hDict);
	CloseHandle(hThreshold);
}
