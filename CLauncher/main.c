#include<stdio.h>
#include<stdlib.h>
#include<math.h>

#include "downloader.h"
#include "launcher.h"
#include "cJSON.h"
#include "strutils.h"
#include "arrutil.h"
#include "process.h"
#include "wpage.h"
#include "imgui/example.h"

#pragma warning(disable : 4996)

LPSTR* WINAPI CommandLineToArgvA_wine(LPSTR lpCmdline, int* numargs)
{
    DWORD argc;
    LPSTR* argv;
    LPSTR s;
    LPSTR d;
    LPSTR cmdline;
    int qcount, bcount;

    if (!numargs || *lpCmdline == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* --- First count the arguments */
    argc = 1;
    s = lpCmdline;
    /* The first argument, the executable path, follows special rules */
    if (*s == '"')
    {
        /* The executable path ends at the next quote, no matter what */
        s++;
        while (*s)
            if (*s++ == '"')
                break;
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*s && *s != ' ' && *s != '\t')
            s++;
    }
    /* skip to the first argument, if any */
    while (*s == ' ' || *s == '\t')
        s++;
    if (*s)
        argc++;

    /* Analyze the remaining arguments */
    qcount = bcount = 0;
    while (*s)
    {
        if ((*s == ' ' || *s == '\t') && qcount == 0)
        {
            /* skip to the next argument and count it if any */
            while (*s == ' ' || *s == '\t')
                s++;
            if (*s)
                argc++;
            bcount = 0;
        }
        else if (*s == '\\')
        {
            /* '\', count them */
            bcount++;
            s++;
        }
        else if (*s == '"')
        {
            /* '"' */
            if ((bcount & 1) == 0)
                qcount++; /* unescaped '"' */
            s++;
            bcount = 0;
            /* consecutive quotes, see comment in copying code below */
            while (*s == '"')
            {
                qcount++;
                s++;
            }
            qcount = qcount % 3;
            if (qcount == 2)
                qcount = 0;
        }
        else
        {
            /* a regular character */
            bcount = 0;
            s++;
        }
    }

    /* Allocate in a single lump, the string array, and the strings that go
     * with it. This way the caller can make a single LocalFree() call to free
     * both, as per MSDN.
     */
    argv = LocalAlloc(LMEM_FIXED, (argc + 1) * sizeof(LPSTR) + (strlen(lpCmdline) + 1) * sizeof(char));
    if (!argv)
        return NULL;
    cmdline = (LPSTR)(argv + argc + 1);
    strcpy(cmdline,lpCmdline);

    /* --- Then split and copy the arguments */
    argv[0] = d = cmdline;
    argc = 1;
    /* The first argument, the executable path, follows special rules */
    if (*d == '"')
    {
        /* The executable path ends at the next quote, no matter what */
        s = d + 1;
        while (*s)
        {
            if (*s == '"')
            {
                s++;
                break;
            }
            *d++ = *s++;
        }
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*d && *d != ' ' && *d != '\t')
            d++;
        s = d;
        if (*s)
            s++;
    }
    /* close the executable path */
    *d++ = 0;
    /* skip to the first argument and initialize it if any */
    while (*s == ' ' || *s == '\t')
        s++;
    if (!*s)
    {
        /* There are no parameters so we are all done */
        argv[argc] = NULL;
        *numargs = argc;
        return argv;
    }

    /* Split and copy the remaining arguments */
    argv[argc++] = d;
    qcount = bcount = 0;
    while (*s)
    {
        if ((*s == ' ' || *s == '\t') && qcount == 0)
        {
            /* close the argument */
            *d++ = 0;
            bcount = 0;

            /* skip to the next one and initialize it if any */
            do {
                s++;
            } while (*s == ' ' || *s == '\t');
            if (*s)
                argv[argc++] = d;
        }
        else if (*s == '\\')
        {
            *d++ = *s++;
            bcount++;
        }
        else if (*s == '"')
        {
            if ((bcount & 1) == 0)
            {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a quote which we erase.
                 */
                d -= bcount / 2;
                qcount++;
            }
            else
            {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                d = d - bcount / 2 - 1;
                *d++ = '"';
            }
            s++;
            bcount = 0;
            /* Now count the number of consecutive quotes. Note that qcount
             * already takes into account the opening quote if any, as well as
             * the quote that lead us here.
             */
            while (*s == '"')
            {
                if (++qcount == 3)
                {
                    *d++ = '"';
                    qcount = 0;
                }
                s++;
            }
            if (qcount == 2)
                qcount = 0;
        }
        else
        {
            /* a regular character */
            *d++ = *s++;
            bcount = 0;
        }
    }
    *d = '\0';
    argv[argc] = NULL;
    *numargs = argc;

    return argv;
}

#define MEMLIMIT 8589934592L

#define PROGRESSLENGHT 50

int bypassMemLimit = 0;

HeapString g_BasePath = { 0 };


#if defined(_WIN32)
#include <windows.h>
#include <process.h>

#ifdef UNICODE
#define CommandLineToArgv(cmdLine, argc) CommandLineToArgvW(cmdLine, argc)
#else
#define CommandLineToArgv(cmdLine, argc) CommandLineToArgvA_wine(cmdLine, argc)
#endif

unsigned long long getTotalSystemMemory()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
}
#else
#include <unistd.h>

unsigned long long getTotalSystemMemory()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}
#endif

typedef struct {
    double dltotal;
    double dlnow;
    int lenght;
}ProgressInfo;

ProgressInfo pinfo = {0};
uintptr_t progressThread;

void progressbarRender(void* param)
{
    ProgressInfo* pinfo = (ProgressInfo*)param;
    bool ended = false;

    while (!ended)
    {
        while (pinfo->dlnow != pinfo->dltotal)
        {
            //printf("%lf / %lf\n", pinfo->dlnow, pinfo->dltotal);
            float percentatge = (pinfo->dlnow / pinfo->dltotal) * 100;
            int filled = (int)((pinfo->dlnow / pinfo->dltotal) * pinfo->lenght);
            int spaces = (int)(((pinfo->dltotal - pinfo->dlnow) / pinfo->dltotal) * pinfo->lenght);

            printf("[");
            for (int i = 0; i < filled; i++)
            {
                printf("=");
            }
            printf(">");
            for (int i = 0; i < spaces; i++)
            {
                printf(" ");
            }
            printf("] %0.0f", roundf(percentatge)); putchar('%');
            fputc('\r', stdout);
            fflush(stdout);
            if (pinfo->dlnow == pinfo->dltotal)
            {
                ended = true;
            }
        }
    }
}

HeapString GetBasePath(char* fpath)
{
    HeapString path = pathFromFileC(fpath);
    return path;
}


int progress_callback(void* clientp,
    double dltotal,
    double dlnow,
    double ultotal,
    double ulnow)
{

    if (dltotal <= 0.0)
    {
        return 0;
    }

    ProgressInfo* info = (ProgressInfo*)clientp;
    info->dlnow = dlnow;
    info->dltotal = dltotal;
    
    return 0;
}

void errorcl(const char* errstr)
{
    printf("Error: %s.\n", errstr);
}

void completed()
{
    WaitForSingleObject((HANDLE)progressThread, INFINITE);
    printf("\nDownload Completed.\n");
}

void init()
{
    progressThread = _beginthread(progressbarRender, 0, &pinfo);
}

void Parse(LPCSTR** argv, int argc)
{
    for (int i = 0; i < argc; i++)
    {
        LPSTR* parameter = argv[i];
        if (!strcmp("--bypass-memlimit", parameter))
        {
            bypassMemLimit = 1;
        }
    }
}

HeapString* GetEXEBasePath()
{
    return &g_BasePath;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{

    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    curl_global_init(CURL_GLOBAL_ALL);

    int alreadyRunning = 0;

    int argc = 0;
    LPSTR** argv = CommandLineToArgvA_wine(GetCommandLineA(), &argc);

    Parse(argv, argc);

    g_BasePath = GetBasePath(argv[0]);

    char exeName[MAX_PATH] = { 0 };
    GetExecutableName(exeName, MAX_PATH);

    //Check if the current process is already running
    GetProcessID(exeName, &alreadyRunning);
    if (alreadyRunning)
    {
        MessageBoxW(NULL, L"This process is already running.", L"Launcher Error", MB_ICONERROR);
        return 0;
    }

    unsigned long long phmem = getTotalSystemMemory();

    printf("SYSMEMORY: %llu bytes.\n", phmem);
    if (phmem < MEMLIMIT && !bypassMemLimit)
    {
        MessageBoxW(NULL, L"Not enough memory!", L"Launcher Error", MB_ICONERROR);
        printf("ERROR: Not enough memory!\n");
        return 0;
    }

    //----------------------------------------------------------------------------
    // Login process. Sets profile symbol, it can be retrieved with GetProfile()
    //----------------------------------------------------------------------------

    OpenWindow();

    TCHAR tempDir[_MAX_PATH];
    GetTempPath(_MAX_PATH, tempDir);

    wprintf(L"TEMP PATH: %s\n", tempDir);

    CLEANSTRING(g_BasePath);
    curl_global_cleanup();
    return EXIT_SUCCESS;
}