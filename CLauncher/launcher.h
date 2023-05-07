#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "Core.h"

#include "strutils.h"
#include "authenticate.h"

#define LAUNCH_SUCCESFUL 0

typedef void(*llog)(const char* const message);
typedef void(*lstate)(const char* const message);
typedef void(*lcompleted)();
typedef void(*lprogress)(double value, double max);

typedef struct
{
	llog clLog;
	lcompleted clCompleted;
	lprogress clProgress;
	lstate clState;
} LauncherCallback;

typedef struct
{
	HeapString argument;
	HeapString value;
}MCArgument;

struct launcher_info_t
{
	const char* versionID;
	const wchar_t tempDir[_MAX_PATH];
	const MinecraftProfile* profile;
	const char* javapath;
	uint32_t memalloc;
	uint32_t port;
	const char* svIP;
};

EXTERNC int InitMinecraftInstance(const char* const path);

EXTERNC void ClearMinecraftInstance();

typedef struct launcher_info_t LauncherInfo;

EXTERNC void SetCallback(const LauncherCallback callback);

EXTERNC HeapString GetCommand(const struct launcher_info_t info);

EXTERNC int InstallForge(void* fVersion);

EXTERNC int InstallVersion(void* param);