#include "allocator.h"
#include<stdio.h>
#include <Windows.h>
#include<stdlib.h>
#include <MinHook.h>

__malloclikefunc morg = NULL;
MALLOCTRACKER hTracker = { 0 };

void* xmalloc(size_t size)
{
	void* allocated = malloc(size);
	if (!allocated)
	{
		printf("ERROR ALLOCATING %zu BYTES!", size);
		return NULL;
	}
	return allocated;
}

void* xrealloc(void* block, size_t newsize)
{
	void* allocated = realloc(block, newsize);
	if (!allocated)
	{
		printf("ERROR REALLOCATING %zu BYTES!", newsize);
		return NULL;
	}
	return allocated;
}

void xfree(void* block)
{
	if (!block)
	{
		puts("Couldn't free memory block. Ignoring.\n");
		return;
	}
	free(block);
}

void* __cdecl __trampoline_malloc_(size_t size)
{
	void* result = NULL;
	void* tempptr = NULL;
	if (morg)
	{
		result = morg(size);
		if (!result)
		{
			//Error handling
		}
		hTracker.nPtrs++;
		tempptr = (void*)hTracker.hPtrs;
		hTracker.hPtrs = (uintptr_t*)realloc(hTracker.hPtrs, hTracker.nPtrs * sizeof(uintptr_t));
		if (!hTracker.hPtrs)
		{
			//Error handling
		}
		free(tempptr);
		hTracker.hPtrs[hTracker.nPtrs - 1] = (uintptr_t)result;
	}
	return result;
}

int TrackMalloc()
{
	if (MH_Initialize() != MH_OK)
	{
		return 1;
	}

	if (MH_CreateHook(&malloc, &__trampoline_malloc_, (LPVOID*)&morg))
	{
		return 1;
	}

	if (MH_EnableHook(&malloc) != MH_OK)
	{
		return 1;
	}

	return 0;
}

void StopTracking()
{
	MH_DisableHook(&malloc);
	MH_Uninitialize();
}

MALLOCTRACKER GetTrackedData()
{
	return hTracker;
}

void FreeTrackedCache()
{
	hTracker.nPtrs = 0;
	if (hTracker.hPtrs)
	{
		free(hTracker.hPtrs);
	}
}