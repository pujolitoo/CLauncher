#pragma once
#ifndef _H_PROCESS_
#define _H_PROCESS_
#include "Core.h"

typedef void(*newline)(const char*, size_t);

unsigned long GetProcessID(const char* name, int* morethanone);

EXTERNC void MessageBoxLastError();

int GetExecutableName(char* dest, size_t bufferSize);

EXTERNC int ExecuteProcess(const char* environment, const char* command, void* callback);

#endif
