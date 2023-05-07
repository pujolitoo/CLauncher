#include "process.h"
#include <string.h>
#include <Windows.h>
#include <TlHelp32.h>
#include "strutils.h"

#define BUFSIZE 4096

int GetExecutableName(char* dest, size_t bufferSize)
{
	char buffer[MAX_PATH] = { 0 };
	int copied = GetModuleFileNameA(GetModuleHandleA(NULL), buffer, MAX_PATH);

	if (copied != strlen(buffer))
		return 1;

	int index = findCharBackwards(buffer, '\\');
	char* namePtr = &(buffer[index+1]);
	strcpy_s(dest, MAX_PATH, namePtr);
	return 0;
}

BOOL IsProcessRunning(HANDLE hProcess)
{
	DWORD exitCode;
	return (GetExitCodeProcess(hProcess, &exitCode) == STILL_ACTIVE);
}

DWORD GetProcessID(const char* name, int* morethanone)
{
	HANDLE hSnapshot;
	DWORD pID = 0;
	DWORD pCount = 0;
	*morethanone = 0;

	PROCESSENTRY32 pEntry = {0};
	pEntry.dwSize = sizeof(PROCESSENTRY32);

	hSnapshot = CreateToolhelp32Snapshot(0x00000002, NULL);

	if (!Process32First(hSnapshot, &pEntry))
		return pID;

	do
	{
		if (!strcmp(name, pEntry.szExeFile))
		{
			pID = pEntry.th32ProcessID;
			pCount++;
		}
	} while (Process32Next(hSnapshot, &pEntry));

	if (morethanone && pCount > 1)
		*morethanone = 1;

	CloseHandle(hSnapshot);

	return pID;
}

typedef struct
{
	HANDLE hStdRead;
	HANDLE hStdWrite;
	LPPROCESS_INFORMATION pInfo;
	newline callback;
} StdGroup;

DWORD StdCapture(void* param)
{
	StdGroup* g_Std = (StdGroup*)param;
	char chBuf[BUFSIZE] = {0};
	BOOL bSuccess = FALSE;
	DWORD dwRead = 0;

	for(;;)
	{
		bSuccess = ReadFile(g_Std->hStdRead, chBuf, BUFSIZE, &dwRead, NULL);
		if(!bSuccess || dwRead == 0) break;
		g_Std->callback(chBuf, dwRead);
		ZeroMemory(chBuf, BUFSIZE);
	}

	// Process ended, free memory resources.
	CloseHandle(g_Std->hStdRead);
	CloseHandle(g_Std->hStdWrite);
	CloseHandle(g_Std->pInfo->hProcess);
	CloseHandle(g_Std->pInfo->hThread);

	HeapFree(GetProcessHeap(), 0, g_Std->pInfo);
	HeapFree(GetProcessHeap(), 0, g_Std);

	return 0;
}

int ExecuteProcess(const char* environment, const char* command, void* callback)
{
	BOOL bSuccess = FALSE;
	STARTUPINFO sInfo;
	ZeroMemory(&sInfo, sizeof(STARTUPINFO));
	sInfo.cb = sizeof(STARTUPINFO);

	if(callback)
	{
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;

		StdGroup* group = NULL;
		HANDLE hThread;
		group = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(StdGroup));
		if(group)
		{
			group->pInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PROCESS_INFORMATION));
			group->callback = (newline)callback;
		}

		//STDOUT
		if(!CreatePipe(&group->hStdRead, &group->hStdWrite, &saAttr, 0))
		{
			return 0;
		}

		sInfo.dwFlags = STARTF_USESTDHANDLES;
		sInfo.hStdOutput = group->hStdWrite;
		sInfo.hStdError = group->hStdWrite;

		bSuccess = CreateProcess(NULL, command, NULL, NULL, TRUE, 0, NULL, environment, &sInfo, group->pInfo);
		if(!bSuccess) return 0;
		hThread = CreateThread(&saAttr, 0, (LPTHREAD_START_ROUTINE)StdCapture, group, 0, NULL);
		if(!hThread) return 0;
	}
	else
	{
		PROCESS_INFORMATION pInfo;
		ZeroMemory(&pInfo, sizeof(PROCESS_INFORMATION));
		bSuccess = CreateProcess(NULL, command, NULL, NULL, TRUE, 0, NULL, environment, &sInfo, &pInfo);
		CloseHandle(pInfo.hProcess);
		CloseHandle(pInfo.hThread);
		if(!bSuccess) return 0;
	}

	return 1;
}

void MessageBoxLastError()
{
	char* buffer = NULL;
	DWORD errID = GetLastError();
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errID,
		MAKELANGID(0x00, 0x01),
		(LPSTR)(&buffer),
		NULL, NULL
	);
	MessageBoxA(NULL, buffer, "Error", MB_ICONERROR);
	LocalFree(buffer);
}