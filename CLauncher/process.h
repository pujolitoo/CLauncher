#pragma once
#ifndef _H_PROCESS_
#define _H_PROCESS_
#include "Core.h"

unsigned long GetProcessID(const char* name, int* morethanone);

EXTERNC void MessageBoxLastError();

int GetExecutableName(char* dest, size_t bufferSize);

#endif
