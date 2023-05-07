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
#else
        pthread_join(CAST(handle)->hThread, NULL);
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

int TaskExecutorConcurrent( void* param )
{
    TaskPool* pool = (TaskPool*)param;
    for(int i = 0; i < pool->nTasks; i++)
    {
        Task task = pool->taskpool[i];
        task.task(task.param);
    }
    pool->completed();
    return 0;
}

TaskPool* create_task_pool(_taskpool_complete completed_callback)
{
    TaskPool* temp = malloc(sizeof(TaskPool));
    if(!temp)
    {
        return NULL;
    }
    memset(temp, 0, sizeof(TaskPool));
    temp->completed = completed_callback;
    return temp;
}

int add_task(TaskPool* pool, _start_func func, void* param)
{
    if(!func || !pool)
    {
        return 0;
    }

    pool->nTasks++;
    Task* temp = pool->taskpool;
    pool->taskpool = (Task*)realloc(pool->taskpool, sizeof(Task)*pool->nTasks);

    if(temp)
    {
        free(temp);
    }

    Task* currTask = &(pool->taskpool[pool->nTasks-1]);
    memset(currTask, 0, sizeof(Task));
    currTask->task = func;
    currTask->param = param;

    return 1;
}

void execute_taskpool_concurrent(TaskPool* pool)
{
    if(!pool || pool->nTasks < 0)
        return;
    parallel_t thread = parallel_create((_start_func)TaskExecutorConcurrent, (void*)pool);
}