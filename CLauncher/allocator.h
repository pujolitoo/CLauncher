#ifndef _H_ALLCOATOR_
#define _H_ALLOCATOR_
#include <stdlib.h>
#include <stdint.h>

typedef void* (*__malloclikefunc)(size_t);

typedef struct
{
	int nPtrs;
	uintptr_t* hPtrs;
} MALLOCTRACKER;

void* xmalloc(size_t size);

void* xrealloc(void* block, size_t newsize);

void xfree(void* block);

int TrackMalloc();

void StopTracking();

MALLOCTRACKER GetTrackedData();

void FreeTrackedCache();

#endif // !_H_ALLOCATOR_