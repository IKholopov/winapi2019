#pragma once

#include <windows.h>
#include <wchar.h>
#include <vector>
#include <cwchar>
#include <tchar.h>
#include <string>
#include <queue>

#define BUFFERSIZE 1024 * 10

enum TType {
	FilterWords, FilterLetters, Close
};

struct CTaskOperation {
	CTaskOperation() = default;
	CTaskOperation(TType type, HANDLE fileToEdit, HANDLE argumentsFile) : 
		Type(type), 
		FileToEdit(fileToEdit), 
		ArgumentsFile(argumentsFile) {}
	TType Type;
	HANDLE FileToEdit;
	HANDLE ArgumentsFile;
};

class CManager;

struct HelperData {
	HelperData() = default;
	HelperData(CManager* m, size_t n) : manager(m), number(n) {}
	CManager* manager;
	size_t number;
};

class CManager
{
public:
	explicit CManager(int numberOfWorkers);
	void ScheduleTask(const CTaskOperation& operation);
	void WaitAll();
	void Close();

	~CManager();

private:
	size_t workersNumber;
	std::vector<PROCESS_INFORMATION> workers;
	std::vector<HANDLE> pipes;
	std::vector<HANDLE> events;
	std::vector<HANDLE> helpers;
	std::vector<HelperData> data;

	std::queue<CTaskOperation> jobQueue;
	CONDITION_VARIABLE queueNotEmpty;
	CONDITION_VARIABLE queueEmpty;
	boolean closed = false;
	CRITICAL_SECTION queueSection;

	friend DWORD WINAPI HelperThread(LPVOID lpParam);
};

