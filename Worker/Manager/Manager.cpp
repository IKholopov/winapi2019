#include "Manager.h"

DWORD WINAPI HelperThread(LPVOID lpParam);

CManager::CManager(int numberOfWorkers) :
	workersNumber(numberOfWorkers),
	workers(numberOfWorkers),
	pipes(numberOfWorkers),
	helpers(numberOfWorkers),
	events(numberOfWorkers),
	data(numberOfWorkers)
{
	InitializeConditionVariable(&queueNotEmpty);
	InitializeConditionVariable(&queueEmpty);
	InitializeCriticalSection(&queueSection);

	SECURITY_ATTRIBUTES attrs;
	attrs.bInheritHandle = true;
	attrs.lpSecurityDescriptor = nullptr;
	attrs.nLength = sizeof(SECURITY_ATTRIBUTES);

	for (size_t i = 0; i < workersNumber; ++i) {
		std::wstring workerDir(TEXT("C:\\Users\\maryf\\source\\repos\\Worker\\Debug\\MyWorker.exe"));
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		boolean success = CreateProcess(
			&workerDir[0],
			nullptr,     
			&attrs,          
			&attrs,         
			TRUE,         
			NORMAL_PRIORITY_CLASS,             
			nullptr,       
			nullptr,         
			&si,  
			&workers[i]);  
		if (!success) {
			printf("CreateProcess failed (%d).\n", GetLastError());
			return;
		}

		printf("child pid: %d\n", workers[i].dwProcessId);

		SECURITY_DESCRIPTOR sd = { 0 };
		::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		::SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE);
		SECURITY_ATTRIBUTES sa = { 0 };
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = &sd;

		std::wstring eventName = L"Global\\FreeWorker" + std::to_wstring(workers[i].dwProcessId);
		events[i] = CreateEvent(
			&sa,              
			TRUE,               
			FALSE,           
			eventName.c_str() 
		);
		if (events[i] == nullptr)
		{
			printf("CreateEvent failed (%d)\n", GetLastError());
			return;
		}
		else {
			printf("Parent: CreateEvent sucessed (%d)\n", events[i]);
		}

		std::wstring name = L"\\\\.\\pipe\\mynamedpipe" + std::to_wstring(workers[i].dwProcessId);
		pipes[i] = CreateNamedPipe(
			name.c_str(),            
			PIPE_ACCESS_DUPLEX,     
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,               
			PIPE_UNLIMITED_INSTANCES, 
			BUFFERSIZE,                 
			BUFFERSIZE,                
			0,                        
			nullptr);                   
		if (pipes[i] == INVALID_HANDLE_VALUE)
		{
			printf("CreateNamedPipe failed, GLE=%d.\n", GetLastError());
			return;
		}

		bool fConnected = ConnectNamedPipe(pipes[i], nullptr) ?
			TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (!fConnected) {
			printf("Cannot connect to pipe, GLE=%d.\n", GetLastError());
			return;
		}

		DWORD dwThreadId = 0;
		data[i] = HelperData(this, i);
		helpers[i] = CreateThread(
			nullptr,                  
			0,                     
			HelperThread,      
			&data[i],         
			0,                     
			&dwThreadId);  
		if (helpers[i] == nullptr)
		{
			printf("CreateThread failed (%d)\n", GetLastError());
			return;
		}
	}
}

void CManager::ScheduleTask(const CTaskOperation & operation)
{
	EnterCriticalSection(&queueSection);
	jobQueue.push(operation);
	LeaveCriticalSection(&queueSection);

	WakeConditionVariable(&queueNotEmpty);
}

void CManager::WaitAll()
{
	EnterCriticalSection(&queueSection);
	while (!jobQueue.empty()) {
		SleepConditionVariableCS(&queueEmpty, &queueSection, INFINITE);
	}
	LeaveCriticalSection(&queueSection);
	WaitForMultipleObjects(workersNumber, events.data(), TRUE, INFINITE);
}

void CManager::Close()
{
	WaitAll();

	EnterCriticalSection(&queueSection);
	closed = true;
	LeaveCriticalSection(&queueSection);

	WakeAllConditionVariable(&queueNotEmpty);

	CTaskOperation close;
	close.Type = TType::Close;
	for (auto& pipe : pipes) {
		DWORD cbWritten;
		WriteFile(
			pipe,      
			(LPVOID)&close,   
			sizeof(close),
			&cbWritten,  
			nullptr);       
	}
	WaitForMultipleObjects(workersNumber, helpers.data(), TRUE, INFINITE);
}

CManager::~CManager()
{
	Close();
	for (auto& pipe : pipes) {
		CloseHandle(pipe);
	}
	for (auto& worker : workers) {
		CloseHandle(worker.hProcess);
	}
}

DWORD WINAPI HelperThread(LPVOID lpParam)
{
	auto data = reinterpret_cast<HelperData*>(lpParam);
	while (true) {
		DWORD dwEvent = WaitForSingleObject(
			data->manager->events[data->number],
			INFINITE);

		if (dwEvent != WAIT_OBJECT_0) {
			printf("Thread Helper: wait error, GLE=%d.\n", GetLastError());
			return -1;
		}

		EnterCriticalSection(&data->manager->queueSection);

		while (!data->manager->closed && data->manager->jobQueue.empty()) {
			SleepConditionVariableCS(&data->manager->queueNotEmpty, &data->manager->queueSection, INFINITE);
		}

		if (data->manager->closed) {
			LeaveCriticalSection(&data->manager->queueSection);
			printf("Helper: exited\n");
			return 0;
		}

		ResetEvent(data->manager->events[data->number]);
		CTaskOperation operation = data->manager->jobQueue.front();
		data->manager->jobQueue.pop();
		bool isEmpty = data->manager->jobQueue.empty();

		LeaveCriticalSection(&data->manager->queueSection);

		if (isEmpty) {
			WakeConditionVariable(&data->manager->queueEmpty);
		}

		bool dup = DuplicateHandle(
			GetCurrentProcess(),
			operation.FileToEdit,
			data->manager->workers[data->number].hProcess,
			&operation.FileToEdit,
			0,
			TRUE,
			DUPLICATE_SAME_ACCESS);
		if (!dup) {
			printf("dup failed, %d\n", GetLastError());
		}
		dup = DuplicateHandle(
			GetCurrentProcess(),
			operation.ArgumentsFile,
			data->manager->workers[data->number].hProcess,
			&operation.ArgumentsFile,
			0,
			TRUE,
			DUPLICATE_SAME_ACCESS);
		if (!dup) {
			printf("dup failed, %d\n", GetLastError());
		}
		DWORD cbWritten;
		bool fSuccess = WriteFile(
			data->manager->pipes[data->number],      
			(LPVOID)&operation,   
			sizeof(operation), 
			&cbWritten,  
			nullptr);       
		if (!fSuccess) {
			printf("Cannot write to file in ScheduleTask, GLE=%d.\n", GetLastError());
			return -1;
		}

	}
	return 0;
}