#include "arrutil.h"
#include "allocator.h"

struct HPArray_t createHPArray()
{
	struct HPArray_t arr;
	arr.count = 0;
	arr.totalsize = 0;
	arr.arr = NULL;
	arr.stringtotalsize = 0;
	return arr;
}

void addToHPArray(struct HPArray_t* arr, const char* string)
{
	HeapString hp = createString(string);
	size_t newsize = arr->totalsize + sizeof(hp);
	arr->arr = realloc(arr->arr, newsize);
	if (!arr->arr)
	{
		printf("ERROR\n");
	}
	arr->arr[arr->count] = hp;
	arr->count++;
	arr->totalsize = newsize;
	arr->stringtotalsize += hp.size;
}

HeapString GetStringSep(const HPArray arr, const char separator)
{
	char* buffer = xmalloc(arr.stringtotalsize);
	uint32_t currsize = 0;
	for (int i = 0; i < arr.count; i++)
	{
		HeapString str = arr.arr[i];
		memcpy(&(buffer[currsize]), str.string, str.size);
		currsize += str.size;
		if ((i + 1) != arr.count)
		{
			buffer[currsize-1] = separator;
		}
	}
	HeapString out = createString(buffer);
	free(buffer);
	return out;
}

HeapString GetString(const HPArray arr)
{
	char* buffer = xmalloc((arr.stringtotalsize - arr.count)+1);
	uint32_t currsize = 0;
	for (int i = 0; i < arr.count; i++)
	{
		HeapString str = arr.arr[i];
		memcpy(&(buffer[currsize]), str.string, str.lenght);
		currsize += str.lenght;
	}
	buffer[currsize] = 0;
	HeapString out = createString(buffer);
	free(buffer);

	return out;
}

void cleanHPArray(struct HPArray_t* arr)
{
	if (arr != NULL && arr->arr != NULL)
	{
		for (int i = 0; i < arr->count; i++)
		{
			if(arr->arr[i].string)
				free(arr->arr[i].string);
		}
	}
}