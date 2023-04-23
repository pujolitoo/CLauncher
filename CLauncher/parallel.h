#ifndef _PARALLEL_H_
#define _PARALLEL_H_

#ifdef _WIN32
#include <Windows.h>
typedef HANDLE _thread_handle;
#else
#include <pthread.h>
typedef pthread_t _thread_handle;
#endif

typedef int(*_start_func)(void*);

typedef void* parallel_t;

parallel_t parallel_create(_start_func func, void* args);

int parallel_join(parallel_t handle);

void parallel_delete(parallel_t handle);

int parallel_cancel(parallel_t handle);

_thread_handle parallel_get_thread_handle(parallel_t handle);

int parallel_get_thread_id(parallel_t handle);

#endif