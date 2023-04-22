#include "strutils.h"
#include "allocator.h"
#include <stdarg.h>

#define validate_void(ptr) if(!ptr){return;}
#define validate_ret(ptr, ret) if(!ptr){return ret;}

HeapString createString(const char* string)
{
	HeapString out = { 0 };

	if (!string)
	{
		return out;
	}
	
	size_t strsize = strlen(string) + 1; // String lenght + null character

	out.size = strsize;
	out.lenght = strlen(string);
	out.string = (char*)malloc(strsize);

	if (!out.string)
	{
		return;
	}

	strcpy_s(out.string, strsize, string);

	return out;
}


char* getString(HeapString handle)
{
	return handle.string;
}

int getSize(HeapString handle)
{
	return handle.size;
}


//
//	Deletes the string. Given struct becomes pointing to an invalid memory adress.
//
void cleanString(HeapString* handle, bool removeHandle)
{
	char* string = handle->string;

	if (string != NULL || handle->size != 0)
	{
		free(string);
		handle->string = 0;
		handle->size = 0;
	}
	if (removeHandle)
	{
		handle = NULL;
	}

}

int findCharBackwards(const char* string, const char character)
{
	validate_ret(string, -1)
	unsigned int offset = 0;
	unsigned int lenght = strlen(string);
	unsigned int index = 0;

	while (offset < lenght)
	{
		offset++;
		index = lenght - offset;
		if (string[index] == character)
		{
			return index;
		}
		if (offset == lenght)
		{
			return 0;
		}
	}
}

static void replace(HeapString* dest, const HeapString source)
{
	dest->size = strlen(source.string) + 1;
	dest->string = (char*)realloc(dest->string, dest->size);
	if (dest->string != NULL)
	{
		strcpy_s(dest->string, dest->size, source.string);
	}
}

int countChars(const char* string, const char ch)
{
	validate_ret(string, -1)
	int counter = 0;
	for (int i = 0; i < strlen(string); i++)
	{
		if (string[i] == ch)
			counter++;
	}
	return counter;
}


void replaceStringFromC(HeapString* dest, const char* source)
{
	HeapString src = createString(source);
	replace(dest, src);
	cleanString(&src, true);
}

void replaceString(HeapString* dest, const HeapString source)
{
	replace(dest, source);
}

void replaceChar(const char from, const char to, HeapString string)
{
	unsigned int offset = 0;
	while (offset < string.size)
	{
		if (string.string[offset] == from)
		{
			string.string[offset] = to;
		}
		offset++;
	}
}

void moveString(HeapString* dest, const HeapString* source)
{
	cleanString(dest, false);
	dest->size = source->size;
	dest->string = source->string;
	cleanString(source, false);
}

static void concat(HeapString* dest, const HeapString source)
{
	size_t resultSize = (dest->size + source.size)-1;
	strcat_s(dest->string, resultSize, source.string);
	dest->size = resultSize;
}

void concatFromC(HeapString* dest, const char* source)
{
	HeapString src = createString(source);
	concat(dest, src);
	cleanString(&src, true);
}

void concatString(HeapString* dest, const HeapString source)
{
	concat(dest, source);
}

HeapString joinStringsC(const char* frst, const char* scnd)
{
	char out[_MAX_PATH] = {0};

	strcpy_s(out, _MAX_PATH, frst);
	strcat_s(out, _MAX_PATH, scnd);

	return createString(out);

}

HeapString joinStrings(const HeapString frst, const HeapString scnd)
{
	return joinStringsC(frst.string, scnd.string);
}

//Terminate args with null
HeapString join(const char* frst, ...)
{
	va_list args;
	va_start(args, frst);

	size_t lenght = strlen(frst);
	
	char* buffer = xmalloc(lenght);
	memcpy(buffer, frst, lenght);

	char* arg = va_arg(args, char*);

	while (arg != NULL)
	{
		size_t argsize = strlen(arg);

		buffer = xrealloc(buffer, lenght + argsize);
		memcpy(&(buffer[lenght]), arg, argsize);
		lenght += strlen(arg);

		arg = va_arg(args, char*);
	}

	buffer = xrealloc(buffer, lenght + 1);
	buffer[lenght] = '\0';

	HeapString str = createString(buffer);
	free(buffer);

	va_end(args);

	return str;
}


static int findDirSlash(const char* filePath)
{
	int index = findCharBackwards(filePath, SLASH_CHAR);
	int index2 = findCharBackwards(filePath, DSLASH_CHAR);
	return (index < index2) ? index2 : index;
}


HeapString pathFromFileC(const char* filePath)
{
	char out[_MAX_PATH] = {0};

	int index = findDirSlash(filePath);
	if (index != 0)
	{
		strcpy_s(out, _MAX_PATH, filePath);
		out[index + 1] = 0;
	}

	return createString(out);

}

HeapString pathFromFile(const HeapString filePath)
{
	return pathFromFileC(filePath.string);
}

HeapString getSubString(const char* string, int index0, int index1)
{
	size_t size = (size_t)((index1 - index0) + 2);
	char* buffer = xmalloc(size);
	int offset = 0;
	for (int i = 0; i < strlen(string); i++)
	{
		if (index0 <= i && i <= index1)
		{
			buffer[offset] = string[i];
			offset++;
		}
		if (i == index1)
		{
			break;
		}
	}
	buffer[size-1] = 0;
	HeapString out = createString(buffer);
	xfree(buffer);
	return out;
}

struct HPArray_t split(const HeapString str, const char* tok)
{
	struct HPArray_t tokens = {0};
	char* token = NULL;
	char* next_token = NULL;
	unsigned int offset = 0;

	token = strtok_s(str.string, tok, &next_token);
	addToHPArray(&tokens, token);


	while (token != NULL)
	{
		offset++;
		token = strtok_s(NULL, tok, &next_token);
		if (token == NULL)
		{
			break;
		}
		addToHPArray(&tokens, token);
	}

	return tokens;
}
