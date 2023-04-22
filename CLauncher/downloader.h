#define CURL_STATICLIB

#ifndef _H_DOWNLOADER
#define _H_DOWNLOADER

#include "curl/curl.h"
#include "strutils.h"

struct raw_data_t
{
    char* data;
    size_t size;
    uint64_t hcl;
};

typedef struct raw_data_t* RawData;
typedef struct raw_data_t RDATA;

typedef void (*dlinit)();
typedef void (*dlcomplete)();
typedef int (*progress)(void*, double, double, double, double);
typedef void (*dlerror)(const char*);

typedef struct
{
    dlinit init;
    dlcomplete completed;
    progress dlprogress;
    dlerror error;
    void* progressData;
} DownloadCallback;

typedef struct
{
    DownloadCallback callback;
    RDATA rdata;
} PerformGroup;

struct raw_data_t;


RDATA GetData(const char* url, DownloadCallback callback);
RDATA GetDataS(const char* url, DownloadCallback callback, uint64_t size);
RDATA GetDataHeaders(const char* url, DownloadCallback callback, struct curl_slist* headers);

HeapString GetDataAsString(RDATA data);

int SaveBinToFile(const char* path, RDATA data);

int DirectDowload(const char* url, const char* path);

void ClearData(RDATA data);

RDATA PostRequest(const char* url, struct curl_slist* headers, const char* postdata);



#endif // _H_DOWNLOADER