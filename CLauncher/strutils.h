/*
	A little wrapper of c string library
*/


#ifndef _H_STRUTILS
#define _H_STRUTILS

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "arrutil.h"
#include "Core.h"

#define DSLASH_CHAR '\\'
#define SLASH_CHAR '/'

#if defined(_WIN32)
#define NORMALIZE(x) replaceChar(SLASH_CHAR, DSLASH_CHAR, x)
#else
#define NORMALIZE(x) replaceChar(DSLASH_CHAR, SLASH_CHAR, x)
#endif

#define CLEANSTRING(x) cleanString(&x, false)

struct HEAPSTRING
{
	char* string;
	size_t size;
	int lenght;
};

typedef struct HEAPSTRING HeapString;

HeapString createString(const char* string);

char* getString(HeapString handle);

EXTERNC void cleanString(HeapString* handle, bool removeHandle);

void replaceStringFromC(HeapString* dest, const char* source);

void replaceString(HeapString* dest, const HeapString source);

void replaceChar(const char from, const char to, HeapString string);

void moveString(HeapString* dest, const HeapString* source);

void concatFromC(HeapString* dest, const char* source);

void concatString(HeapString* dest, const HeapString source);

HeapString joinStringsC(const char* frst, const char* scnd);

HeapString joinStrings(const HeapString frst, const HeapString scnd);

// Terminate args with null
HeapString join(const char* frst, ...);

HeapString pathFromFileC(const char* filePath);

HeapString pathFromFile(const HeapString filePath);

HeapString getSubString(const char* string, int index0, int index1);

struct HPArray_t split(const HeapString str, const char* tok);

int findCharBackwards(const char* string, const char character);

int countChars(const char* string, const char ch);


#endif // _H_STRUTILS
