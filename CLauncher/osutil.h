#pragma once

#if defined(_WIN32)
#include <ShlObj.h>
#include <processthreadsapi.h>
#define RECDIRS(x) SHCreateDirectoryExA(NULL, x, NULL)
#define EXECPROC(/*STARTCMD*/x, /*STARTUPINFO*/y, /*PROCINFO*/z) CreateProcessA(NULL, x, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, y, z)
#define OSNAME "windows"
#elif defined(__linux__)
#define OSNAME "linux"
#elif defined(__OSX__)
#define OSNAME "osx"
#endif