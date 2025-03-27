
#pragma once
#include "stdio.h"

#define LOGGING_LEVEL_NONE 0
#define LOGGING_LEVEL_ERROR 1
#define LOGGING_LEVEL_WARNING 2
#define LOGGING_LEVEL_INFO 3
#define LOGGING_LEVEL_DEBUG 4
#define LOGGING_LEVEL_TRACE 5

#if LOGGING_LEVEL < LOGGING_LEVEL_TRACE
#define log_t(message, ...)
#else
#define log_tm(message, ...) printf("trace: " message, ##__VA_ARGS__)
#define log_t(message, ...) log_tm(message "\n", ##__VA_ARGS__)
#endif

#if LOGGING_LEVEL < LOGGING_LEVEL_DEBUG
#define log_d(message, ...)
#else
#define log_dm(message, ...) printf("debug: " message, ##__VA_ARGS__)
#define log_d(message, ...) log_dm(message "\n", ##__VA_ARGS__)
#endif

#if LOGGING_LEVEL < LOGGING_LEVEL_WARNING
#define log_w(message, ...)
#else
/**
 * @brief Log warning, without new line at the end
 * 
 */
#define log_wm(message, ...) printf("warning: " message, ##__VA_ARGS__)
/**
 * @brief Log warning line
 * 
 */
#define log_w(message, ...) log_wm(message "\n", ##__VA_ARGS__)
#endif

#if LOGGING_LEVEL < LOGGING_LEVEL_INFO
#define log_i(message, ...)
#else
/**
 * @brief Log info, without new line at the end
 * 
 */

#define log_im(message, ...) printf("info: " message, ##__VA_ARGS__)
/**
 * @brief Log info line
 * 
 */
#define log_i(message, ...) log_im(message "\n", ##__VA_ARGS__)
#endif

#if LOGGING_LEVEL < LOGGING_LEVEL_ERROR
#define log_e(message, ...)
#else
#define log_em(message, ...) printf("error: " message, ##__VA_ARGS__)
#define log_e(message, ...) log_em(message "\n", ##__VA_ARGS__)
#endif
