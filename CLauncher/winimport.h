#ifndef _H_WINIMP_
#define _H_WINIMP_

#include <stdbool.h>
#include <stdint.h>

#define WINIMPORT __declspec(dllimport)

//------------------------------------------------------------------------------------------------
// Self importing Win32 functions because including Windows.h collides with raylib symbols...
//------------------------------------------------------------------------------------------------

#define MB_OK 0x00000000L
#define MB_ICONINFORMATION  0x00000040L
#define MB_ICONERROR 0x00000010L
#define MB_ICONASTERISK 0x00000040L
#define MB_ICONWARNING 0x00000030L
#define MB_ICONEXCLAMATION 0x00000030L

#define WM_SETICON                      0x0080
#define ICON_SMALL 0
#define ICON_BIG 1

#define MAX_PATH 260

#define INFINITE 0xFFFFFFFF

#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100
#define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000
#define FORMAT_MESSAGE_IGNORE_INSERTS 0x00000200

#define LR_DEFAULTCOLOR     0x00000000
#define LR_MONOCHROME       0x00000001
#define LR_COLOR            0x00000002
#define LR_COPYRETURNORG    0x00000004
#define LR_COPYDELETEORG    0x00000008
#define LR_LOADFROMFILE     0x00000010
#define LR_LOADTRANSPARENT  0x00000020
#define LR_DEFAULTSIZE      0x00000040
#define LR_VGACOLOR         0x00000080
#define LR_LOADMAP3DCOLORS  0x00001000
#define LR_CREATEDIBSECTION 0x00002000
#define LR_COPYFROMRESOURCE 0x00004000
#define LR_SHARED           0x00008000

#define IMAGE_BITMAP        0
#define IMAGE_ICON          1
#define IMAGE_CURSOR        2

#define MAKEINTRESOURCEA(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
#define MAKEINTRESOURCEW(i) ((LPWSTR)((ULONG_PTR)((WORD)(i))))

#define MAKELANGID(p, s)       ((((WORD  )(s)) << 10) | (WORD  )(p))

#define ZeroMemory(dest, size) memset(dest, 0, size);

typedef void* HANDLE;
typedef unsigned long DWORD;
typedef bool BOOL;
typedef long LONG;
typedef unsigned long long ULONG_PTR;
typedef char CHAR;
typedef signed long HRESULT;
typedef unsigned short WORD;
typedef unsigned char* LPBYTE;
typedef char* LPSTR;
typedef unsigned int UINT;
typedef void* HWND;
typedef const char* LPCSTR;
typedef const wchar_t* LPCWSTR;
typedef wchar_t* LPTSTR;
typedef void* HINSTANCE;
typedef void* HMODULE;
typedef const void* LPCVOID;
typedef HANDLE HLOCAL;
typedef __int64 LONG_PTR;
typedef LONG_PTR LRESULT;
typedef LONG_PTR LPARAM;
typedef unsigned __int64 UINT_PTR;
typedef UINT_PTR WPARAM;
typedef HANDLE HICON;
typedef wchar_t* LPWSTR;

#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)

typedef struct _devicemodeA {
    unsigned char  dmDeviceName[32];
    unsigned short dmSpecVersion;
    unsigned short dmDriverVersion;
    unsigned short dmSize;
    unsigned short dmDriverExtra;
    unsigned long dmFields;
    union {
        /* printer only fields */
        struct {
            short dmOrientation;
            short dmPaperSize;
            short dmPaperLength;
            short dmPaperWidth;
            short dmScale;
            short dmCopies;
            short dmDefaultSource;
            short dmPrintQuality;
        };
        /* display only fields */
        struct {
            unsigned long long dmPosition;
            unsigned long  dmDisplayOrientation;
            unsigned long  dmDisplayFixedOutput;
        };
    };
    short dmColor;
    short dmDuplex;
    short dmYResolution;
    short dmTTOption;
    short dmCollate;
    unsigned char   dmFormName[32];
    unsigned short   dmLogPixels;
    unsigned long  dmBitsPerPel;
    unsigned long  dmPelsWidth;
    unsigned long  dmPelsHeight;
    union {
        unsigned long  dmDisplayFlags;
        unsigned long  dmNup;
    };
    unsigned long  dmDisplayFrequency;
    unsigned long  dmICMMethod;
    unsigned long  dmICMIntent;
    unsigned long  dmMediaType;
    unsigned long  dmDitherType;
    unsigned long  dmReserved1;
    unsigned long  dmReserved2;
    unsigned long  dmPanningWidth;
    unsigned long  dmPanningHeight;
} DEVMODEA, * PDEVMODEA, * NPDEVMODEA, * LPDEVMODEA;

typedef struct _SECURITY_ATTRIBUTES {
    unsigned long nLength;
    void* lpSecurityDescriptor;
    bool bInheritHandle;
} SECURITY_ATTRIBUTES, * PSECURITY_ATTRIBUTES, * LPSECURITY_ATTRIBUTES;

typedef struct _STARTUPINFOA {
    DWORD  cb;
    LPSTR  lpReserved;
    LPSTR  lpDesktop;
    LPSTR  lpTitle;
    DWORD  dwX;
    DWORD  dwY;
    DWORD  dwXSize;
    DWORD  dwYSize;
    DWORD  dwXCountChars;
    DWORD  dwYCountChars;
    DWORD  dwFillAttribute;
    DWORD  dwFlags;
    WORD   wShowWindow;
    WORD   cbReserved2;
    LPBYTE lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOA, * LPSTARTUPINFOA;

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD  dwProcessId;
    DWORD  dwThreadId;
} PROCESS_INFORMATION, * PPROCESS_INFORMATION, * LPPROCESS_INFORMATION;

WINIMPORT BOOL EnumDisplaySettingsA(
    char* lpszDeviceName,
    DWORD    iModeNum,
    DEVMODEA* lpDevMode
);
WINIMPORT int GetSystemMetrics(
    int nIndex
);

WINIMPORT int MessageBoxA(
    void*   hWnd,
    const char* lpText,
    const char* lpCaption,
    unsigned int    uType
);

WINIMPORT BOOL EnableWindow(
    void* hwnd,
    BOOL bEnable
);

WINIMPORT int MessageBoxW(
    HWND    hWnd,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT    uType
);

WINIMPORT HANDLE CreateThread(
    LPSECURITY_ATTRIBUTES   lpThreadAttributes,
    size_t                  dwStackSize,
    void*  lpStartAddress,
    void*                  lpParameter,
    unsigned long                  dwCreationFlags,
    unsigned long*          lpThreadId
);

WINIMPORT unsigned long ResumeThread(
    HANDLE hThread
);

WINIMPORT bool TerminateThread(
    HANDLE hThread,
    unsigned long  dwExitCode
);

WINIMPORT DWORD WaitForSingleObject(
    HANDLE hHandle,
    unsigned long  dwMilliseconds
);

WINIMPORT BOOL CloseHandle(
    HANDLE hObject
);

WINIMPORT DWORD GetModuleFileNameA(
    HMODULE hModule,
    LPSTR   lpFilename,
    DWORD   nSize
);

WINIMPORT BOOL DestroyIcon(
    HICON hIcon
);

WINIMPORT HANDLE OpenProcess(
    DWORD dwDesiredAccess,
    BOOL  bInheritHandle,
    DWORD dwProcessId
);

WINIMPORT BOOL CreateProcessA(
    const char* lpApplicationName,
    char*                 lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL                  bInheritHandles,
    DWORD                 dwCreationFlags,
    void*                lpEnvironment,
    const char*                lpCurrentDirectory,
    LPSTARTUPINFOA        lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
);

WINIMPORT BOOL TerminateProcess(
    HANDLE hProcess,
    UINT   uExitCode
);

WINIMPORT HWND FindWindowA(
    LPCSTR lpClassName,
    LPCSTR lpWindowName
);

WINIMPORT LRESULT SendMessageA(
    HWND   hWnd,
    UINT   Msg,
    WPARAM wParam,
    LPARAM lParam
);

WINIMPORT HICON LoadIconA(
    HINSTANCE hInstance,
    LPCSTR    lpIconName
);

WINIMPORT HWND SetFocus(
    HWND hWnd
);

WINIMPORT BOOL FlashWindow(
    HWND hWnd,
    BOOL bInvert
);

WINIMPORT int GetClassNameW(
    HWND   hWnd,
    wchar_t* lpClassName,
    int    nMaxCount
);

WINIMPORT BOOL SetForegroundWindow(
    HWND hWnd
);

WINIMPORT BOOL UnregisterClassW(
    LPCWSTR   lpClassName,
    HINSTANCE hInstance
);

WINIMPORT HMODULE GetModuleHandleA(
    LPCSTR lpModuleName
);

WINIMPORT DWORD GetLastError();

WINIMPORT DWORD FormatMessageA(
    DWORD   dwFlags,
    LPCVOID lpSource,
    DWORD   dwMessageId,
    DWORD   dwLanguageId,
    LPSTR   lpBuffer,
    DWORD   nSize,
    va_list* Arguments
);

WINIMPORT BOOL DestroyWindow(
    HWND hWnd
);

WINIMPORT HANDLE LoadImageA(
    HINSTANCE hInst,
    LPCSTR    name,
    UINT      type,
    int       cx,
    int       cy,
    UINT      fuLoad
);


//------------------------------------------------------------------------------------------------
// TlHelp32
//------------------------------------------------------------------------------------------------

typedef struct tagPROCESSENTRY32
{
    DWORD   dwSize;
    DWORD   cntUsage;
    DWORD   th32ProcessID;          // this process
    ULONG_PTR th32DefaultHeapID;
    DWORD   th32ModuleID;           // associated exe
    DWORD   cntThreads;
    DWORD   th32ParentProcessID;    // this process's parent process
    LONG    pcPriClassBase;         // Base priority of process's threads
    DWORD   dwFlags;
    CHAR    szExeFile[MAX_PATH];    // Path
} PROCESSENTRY32;

WINIMPORT HANDLE CreateToolhelp32Snapshot(
    DWORD dwFlags,
    DWORD th32ProcessID
);

WINIMPORT BOOL Process32First(
    HANDLE           hSnapshot,
    PROCESSENTRY32* lppe
);

WINIMPORT BOOL Process32Next(
    HANDLE           hSnapshot,
    PROCESSENTRY32* lppe
);

//------------------------------------------------------------------------------------------------
// Memory
//------------------------------------------------------------------------------------------------

WINIMPORT BOOL WriteProcessMemory(
    HANDLE  hProcess,
    void*  lpBaseAddress,
    const void* lpBuffer,
    size_t  nSize,
    size_t* lpNumberOfBytesWritten
);

WINIMPORT BOOL ReadProcessMemory(
    HANDLE  hProcess,
    const void* lpBaseAddress,
    void*  lpBuffer,
    size_t  nSize,
    size_t* lpNumberOfBytesRead
);

WINIMPORT HLOCAL LocalFree(
    HLOCAL hMem
);

#endif //_H_WINIMP_