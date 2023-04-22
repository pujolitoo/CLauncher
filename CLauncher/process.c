#include "process.h"
#include <string.h>
#include "winimport.h"
#include "strutils.h"

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