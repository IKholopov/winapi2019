#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <algorithm> 
#include <cctype>
#include <locale>

#include "Manager.h"

DWORD pId;

static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z';
	}));
}

static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z';
	}).base(), s.end());
}

static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

std::string GetFilteredText(const std::vector<std::string>& words, const std::unordered_set<std::string>& dict) {
	std::string result = "";
	for (auto word : words) {
		trim(word);
		if (dict.find(word) == dict.end()) {
			result += " " + word;
		}
	}
	return result.empty() ? "" : result.substr(1);
}

std::string ReadText(HANDLE handle) {
	DWORD read;
	std::string textBuffer(BUFFERSIZE, ' ');
	// critical interprocess section?
	DWORD dwPtr = SetFilePointer(
		handle,
		0,
		nullptr,
		FILE_BEGIN);
	bool success = ReadFile(handle, (LPVOID)textBuffer.data(), BUFFERSIZE - 1, &read, NULL);
	if (!success) {
		_tprintf(TEXT("Could not open text. GLE=%d, pid=%d, handle=%d\n"), GetLastError(), pId, handle);
		return "";
	}
	dwPtr = SetFilePointer(handle,
		0,
		nullptr,
		FILE_BEGIN);
	textBuffer.resize(read);
	return textBuffer;
}

void RewriteFile(const std::string& newText, HANDLE handle) {
	DWORD dwPtr = SetFilePointer(handle,
		0,
		nullptr,
		FILE_BEGIN);
	DWORD read;
	bool success = WriteFile(handle, newText.data(), newText.length(), &read, NULL);
	if (!success) {
		_tprintf(TEXT("Could not write to text. GLE=%d, pid=%d, handle=%d\n"), GetLastError(), pId, handle);
		return;
	}
	SetEndOfFile(handle);
}

void DoFilterWords(HANDLE text, HANDLE dict) {
	std::string textBuffer = ReadText(text);
	std::string dictBuffer = ReadText(dict);
	std::vector<std::string> words;
	std::unordered_set<std::string> dictionary;

	std::string delim = " ";
	size_t start = 0;
	auto end = textBuffer.find(delim);
	while (end != std::string::npos) {
		words.emplace_back(textBuffer.substr(start, end - start));
		start = end + delim.length();
		end = textBuffer.find(delim, start);
	}
	words.emplace_back(textBuffer.substr(start, end));

	start = 0;
	end = dictBuffer.find(delim);
	while (end != std::string::npos) {
		dictionary.emplace(dictBuffer.substr(start, end - start));
		start = end + delim.length();
		end = dictBuffer.find(delim, start);
	}
	dictionary.emplace(dictBuffer.substr(start, end));
	std::string newText = GetFilteredText(words, dictionary);
	RewriteFile(newText, text);
}

std::string GetFilteredByThresholdText(const std::string& text, const std::unordered_map<char, size_t>& freq, size_t threshold) {
	std::string result = "";
	std::string delim = " ";
	size_t start = 0;
	auto end = text.find(delim);
	while (true) {
		std::string word = text.substr(start, end - start);
		bool accept = true;
		for (auto& ch : word) {
			if (freq.at(ch) < threshold) {
				accept = false;
				break;
			}
		}
		if (accept) {
			result += " " + word;
		}
		if (end == std::string::npos) {
			break;
		}
		start = end + delim.length();
		end = text.find(delim, start);
	}
	return result.empty() ? "" : result.substr(1);
}

void DoFilterLetters(HANDLE textHandle, HANDLE thresholdHandle) {
	std::string text = ReadText(textHandle);
	std::string thresholdText = ReadText(thresholdHandle);
	size_t threshold = std::stoi(thresholdText);

	std::unordered_map<char, size_t> freq;
	for (auto& ch : text) {
		if (freq.find(ch) == freq.end()) {
			freq[ch] = 1;
		}
		else {
			freq[ch]++;
		}
	}
	freq[' '] = 0;
	std::string newText = GetFilteredByThresholdText(text, freq, threshold);
	RewriteFile(newText, textHandle);
}

int main() {
	HANDLE hPipe;
	LPCWSTR lpvMessage = TEXT("Default message from client.");
	CTaskOperation op;
	BOOL fSuccess = FALSE;
	DWORD cbRead, dwMode;
	pId = GetCurrentProcessId();
	std::wstring name = L"\\\\.\\pipe\\mynamedpipe" + std::to_wstring(pId);
	std::wstring eventName = L"Global\\FreeWorker" + std::to_wstring(pId);
	_tprintf(TEXT("pid: %d, Start\n"), pId);

	while (true) {
		hPipe = CreateFile(
			name.c_str(), 
			GENERIC_READ | GENERIC_WRITE,
			0,             
			nullptr,          
			OPEN_EXISTING,
			0,            
			nullptr);         
		if (hPipe != INVALID_HANDLE_VALUE) {
			break;
		}
		if (GetLastError() != ERROR_PIPE_BUSY) {
			_tprintf(TEXT("Could not open pipe. GLE=%d, pid=%d\n"), GetLastError(), pId);\
			return -1;
		}
		
		if (!WaitNamedPipe(name.c_str(), 20000)) {
			printf("Could not open pipe: 20 second wait timed out.");
			return -1;
		}
	}

	dwMode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
		hPipe,   
		&dwMode,
		nullptr,     
		nullptr);    

	if (!fSuccess) {
		_tprintf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError());
		return -1;
	}

	SECURITY_DESCRIPTOR sd = { 0 };
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE);
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;

	HANDLE event = OpenEvent(EVENT_MODIFY_STATE, FALSE, eventName.c_str());
	if (event == nullptr) {
		_tprintf(TEXT("CreateEvent failed. GLE=%d\n"), GetLastError());
		return -1;
	}
	else {
		_tprintf(TEXT("OpenEvent successed: %d\n"), event);
	}
	
	do {
		SetEvent(event);

		fSuccess = ReadFile(
			hPipe, 
			&op,   
			sizeof(op),  
			&cbRead, 
			nullptr);   

		if (!fSuccess && GetLastError() != ERROR_MORE_DATA) {
			_tprintf(TEXT("ReadFile from pipe failed. GLE=%d\n"), GetLastError());
			return -1;
		}

		_tprintf(TEXT("pid: %d, Type: \"%d\"\n"), pId, op.Type);
		switch (op.Type) {
		case Close:
			SetEvent(event);
			printf("\nWorker: exited\n");
			return 0;
		case FilterWords:
			DoFilterWords(op.FileToEdit, op.ArgumentsFile);
			break;
		case FilterLetters:
			DoFilterLetters(op.FileToEdit, op.ArgumentsFile);
			break;
		}
		CloseHandle(op.ArgumentsFile);
		CloseHandle(op.FileToEdit);
	} while (true);  

	return 0;
}