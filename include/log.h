
#pragma once
#include "pico/stdio.h"

#define LOGGING_LEVEL_NONE 0
#define LOGGING_LEVEL_ERROR 1
#define LOGGING_LEVEL_WARNING 2
#define LOGGING_LEVEL_INFO 3
#define LOGGING_LEVEL_DEBUG 4
#define LOGGING_LEVEL_TRACE 5

#if LOGGING_LEVEL < LOGGING_LEVEL_TRACE
#define log_t(...)
#else
#define log_t(...) printf("trace: " __VA_ARGS__)
#endif

#if LOGGING_LEVEL < LOGGING_LEVEL_DEBUG
#define log_d(...)
#else
#define log_d(...) printf("debug: " __VA_ARGS__)
#endif

#if LOGGING_LEVEL < LOGGING_LEVEL_WARNING
#define log_w(...)
#else
#define log_w(...) printf("warning: " __VA_ARGS__)
#endif

#if LOGGING_LEVEL < LOGGING_LEVEL_INFO
#define log_i(...)
#else
#define log_i(...) printf("info: " __VA_ARGS__)
#endif

#if LOGGING_LEVEL < LOGGING_LEVEL_ERROR
#define log_e(...)
#else
#define log_e(...) printf("error: " __VA_ARGS__)
#endif
