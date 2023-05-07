#ifndef _PARALLEL_H_
#define _PARALLEL_H_

#include <stdint.h>
#include "Core.h"

#ifdef _WIN32
#include <Windows.h>
typedef HANDLE _thread_handle;
#else
#include <pthread.h>
typedef pthread_t _thread_handle;
#endif


typedef int(*_start_func)(void*);
typedef void(*_taskpool_complete)();

typedef struct{
    _start_func task;
    void* param;
    int completed;
} Task;

typedef struct
{
    Task* taskpool;
    uint32_t nTasks;
    _taskpool_complete completed;

} TaskPool;

typedef void* parallel_t;

parallel_t parallel_create(_start_func func, void* args);

int parallel_join(parallel_t handle);

void parallel_delete(parallel_t handle);

int parallel_cancel(parallel_t handle);

_thread_handle parallel_get_thread_handle(parallel_t handle);

int parallel_get_thread_id(parallel_t handle);

EXTERNC TaskPool* create_task_pool();

EXTERNC int add_task(TaskPool* pool, _start_func func, void* param);

EXTERNC void execute_taskpool_concurrent(TaskPool* pool);

#endif