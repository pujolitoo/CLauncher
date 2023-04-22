#ifndef _H_ARRUTIL
#define _H_ARRUTIL
#include <stdint.h>
#include "strutils.h"


struct HPArray_t {
	struct HEAPSTRING* arr;
	size_t totalsize;
	size_t stringtotalsize;
	int count;
};

typedef struct HPArray_t HPArray;

struct HPArray_t createHPArray();

void addToHPArray(struct HPArray_t* arr, const char* string);
struct HEAPSTRING GetStringSep(const HPArray arr, const char separator);
struct HEAPSTRING GetString(const HPArray arr);

void cleanHPArray(struct HPArray_t* arr);


#endif