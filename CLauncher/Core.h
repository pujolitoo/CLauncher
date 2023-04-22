#ifndef _CORE_H_
#define _CORE_H_

#ifdef _WIN32
#ifdef LIBEXPORT
#define LIBAPI _declspec(dllexport)
#else
#define LIBAPI _declspec(dllimport)
#endif
#endif

#define LOG_INFO(X) fprintf(stdout, "[INFO]: %s\n", X);
#define LOG_WARN(X) fprintf(stdout, "[WARN]: %s\n", X);
#define LOG_ERR(X) fprintf(stderr, "[ERROR]: %s\n", X);

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif


#endif
