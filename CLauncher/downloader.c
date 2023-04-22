
#include "downloader.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "allocator.h"

#define HEADCLENGTH "Content-Length: "

#pragma comment(lib, "Normaliz.lib" )
#pragma comment(lib, "wldap32.lib" )
#pragma comment(lib, "crypt32.lib" )
#pragma comment(lib, "Ws2_32.lib")


static size_t header_callback(char* buffer, size_t size,
    size_t nitems, void* userdata)
{
    uint64_t* out = (uint64_t*)userdata;

    if (strstr(buffer, HEADCLENGTH) != NULL)
    {
        char* strint = buffer + strlen(HEADCLENGTH);
        *out = (uint64_t)strtoull(strint, NULL, 10);
    }
    return nitems * size;
}

static size_t directFile(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    size_t total = size * nmemb;

    FILE* file = (FILE*)userdata;
    fwrite(ptr, size, nmemb, file);

    return total;
}

static size_t writeFunction(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t total = size*nmemb;

    PerformGroup* clback = (PerformGroup*)userdata;

    //If data lenght is not specified, reallocate RDATA memory block for each block recieved.
    if (clback->rdata.hcl == 0)
    {
        clback->rdata.data = xrealloc(clback->rdata.data, clback->rdata.size + total);
        if (!clback->rdata.data)
        {
            free(clback->rdata.data);
            clback->rdata.data = NULL;
            clback->callback.error("Error reallocating memory space.");
        }
    }

    //If data lenght could be resolved, allocate with the provided size.
    else if (clback->rdata.hcl != 0 && clback->rdata.size == 0)
    {
        // Fails on allocating 10GB, no contiguous memory block available. Maybe allocating in partial blocks could fix the problem.
        clback->rdata.data = xmalloc(clback->rdata.hcl);
        if (!clback->rdata.data)
        {
            free(clback->rdata.data);
            clback->callback.error("Error allocating memory space.");
        }
    }

    //Append recieved data to RDATA buffer.
    if (clback->rdata.data)
    {
        memcpy(&(clback->rdata.data[clback->rdata.size]), ptr, total);
        clback->rdata.size += total;
    }

    //Download completed.
    if (clback->rdata.hcl != 0 && clback->rdata.size == clback->rdata.hcl)
    {
        /*printf("Download Completed.\n");
        if (clback->callback.completed != NULL)
        {
            clback->callback.completed();
        }*/
    }
    
    return total;
}

static void curlPerform(CURL* handle, DownloadCallback callback)
{
    CURLcode result = curl_easy_perform(handle);
    if (result != CURLE_OK)
    {
        const char* errorstring = curl_easy_strerror(result);
        if (callback.error != NULL)
        {
            callback.error(errorstring);
        }
    }
    else
    {
        if (callback.completed != NULL)
        {
            callback.completed();
        }
    }
}

static CURL *InitCurl(const char* url)
{
    CURL* handle;
    handle = curl_easy_init();
    if(!handle) return NULL;
    curl_easy_setopt(handle, CURLOPT_URL, url);
    return handle;
}

static void DestroyCurl(CURL* handle)
{
    if (handle)
    {
        curl_easy_cleanup(handle);
    }
}

static RDATA GetDataRaw(const char* url, DownloadCallback callback, uint64_t size, struct curl_slist* headers)
{
    struct raw_data_t response = { 0 };

    PerformGroup perf;
    perf.callback = callback;
    perf.rdata = response;



    CURL* handle = InitCurl(url);
    if (!handle) callback.error("Couldn't initialize curl.");
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&perf);
    curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, callback.dlprogress);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    if (headers)
    {
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
    }
    curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, callback.progressData);
    if (callback.dlprogress != NULL) curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0);

    if (size != 0)
    {
        perf.rdata.hcl = size;
    }
    else
    {
        perf.rdata.hcl = getContentLenght(url);
    }

    if (callback.init != NULL)
    {
        callback.init();
    }

    curlPerform(handle, callback);

    DestroyCurl(handle);

    return perf.rdata;
}

RDATA GetDataS(const char* url, DownloadCallback callback, uint64_t size)
{
    return GetDataRaw(url, callback, size, 0);
}

RDATA GetData(const char* url, DownloadCallback callback)
{
    return GetDataRaw(url, callback, 0, 0);
}

RDATA GetDataHeaders(const char* url, DownloadCallback callback, struct curl_slist* headers)
{
    return GetDataRaw(url, callback, 0, headers);
}

HeapString GetDataAsString(const RDATA data)
{
    uint32_t size = strlen(data.data);
    char* buffer = malloc(size);
    if (!buffer)
    {
        printf("ERROR.");
    }
    memcpy(buffer, data.data, size);
    buffer[data.size] = 0;
    HeapString datastr = createString(buffer);
    datastr.size = data.size;
    free(buffer);
    return datastr;
}

int SaveBinToFile(const char* path, const RDATA data)
{
    FILE* file;
    fopen_s(&file, path, "wb");
    if (!file)
    {
        return 1;
    }
    fwrite(data.data, 1, data.size, file);
    fclose(file);
    return 0;
}

int DirectDowload(const char* url, const char* path)
{
    FILE* file;
    fopen_s(&file, path, "wb");
    if (!file)
    {
        return -1;
    }
    CURL* handle = InitCurl(url);
    if (!handle)
    {
        return -1;
    }
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)file);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, directFile);

    CURLcode result = curl_easy_perform(handle);
    if (result != CURLE_OK)
    {
        return -1;
    }
    fclose(file);
    return 0;
}

void ClearData(RDATA data)
{
    if (data.data != NULL)
    {
        free(data.data);
    }
    data.size = 0;
}

static uint32_t getContentLenght(const char* url)
{
    uint64_t size = 0;
    DownloadCallback cl = { 0 };

    CURL* handle = InitCurl(url);

    curl_easy_setopt(handle, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, (void*) &size);

    curlPerform(handle, cl);

    DestroyCurl(handle);

    return size;
}

RDATA PostRequest(const char* url, struct curl_slist* headers, const char* postdata)
{
    DownloadCallback cl = { 0 };
    struct raw_data_t response = { 0 };

    PerformGroup perf;
    perf.callback = cl;
    perf.rdata = response;

    CURL* hCurl = InitCurl(url);
    curl_easy_setopt(hCurl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(hCurl, CURLOPT_POSTFIELDS, postdata);
    curl_easy_setopt(hCurl, CURLOPT_POST, 1L);
    curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, (void*)&perf);

    CURLcode result = curl_easy_perform(hCurl);

    DestroyCurl(hCurl);

    return perf.rdata;
}