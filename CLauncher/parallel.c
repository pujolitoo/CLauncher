#include "parallel.h"
#include <stdint.h>

#define CAST(x) ((__parallel_t*)(x))

typedef struct
{
    _thread_handle hThread;
    _start_func hFunc;
    void* arg;
    uint32_t tId;
}__parallel_t;

int parallel_get_thread_id(parallel_t handle)
{
    return CAST(handle)->tId;
}

_thread_handle parallel_get_thread_handle(parallel_t handle)
{
    return CAST(handle)->hThread;
}

parallel_t parallel_create(_start_func func, void* args)
{
    __parallel_t* temp = (__parallel_t*)malloc(sizeof(__parallel_t));
    memset(temp, 0, sizeof(__parallel_t));
    temp->arg = args;
    temp->hFunc = func;

#ifdef _WIN32
    temp->hThread = CreateThread(
        NULL, 
        0, 
        (LPTHREAD_START_ROUTINE)temp->hFunc, 
        temp->arg, 
        0, 
        &temp->tId
    );
#else
    pthread_create(&temp->hThread, NULL, (start_routine)temp->hFunc, temp->arg);
    pthread_getunique_np(&temp->hThread, &temp->tId);
#endif
    return temp;
}

int parallel_join(parallel_t handle)
{
    __parallel_t* temp = CAST(handle);
    int retVal = 0;
#ifdef _WIN32
    WaitForSingleObject(temp->hThread, INFINITE);
    GetExitCodeThread(temp->hThread, &retVal);
#else
    retVal = pthread_join(temp->hThread, NULL);
#endif
    return retVal;
}

void parallel_delete(parallel_t handle)
{
    if(!handle) return;
    if(CAST(handle)->hThread){
#ifdef _WIN32
        CloseHandle(CAST(handle)->hThread);
#endif
    }
    free(CAST(handle));
}

int parallel_cancel(parallel_t handle)
{
#ifdef _WIN32
    return TerminateThread(CAST(handle)->hThread, 0);
#else
    return pthread_cancel(CAST(handle)->hThread);
#endif
}