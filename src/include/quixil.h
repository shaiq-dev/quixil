#ifndef Qxl_QUIXIL_H
#define Qxl_QUIXIL_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_TRACE_EXECUTION
#define DEBUG_TRACE_COMPILING_CHUNK

#define Qxl_INTERCEPT_ERROR(r, i, c)\ 
    if (r == i) exit(c)

#define Qxl_ERROR(...)\                   
    do { if (__VA_ARGS__) { (void)fprintf(stderr, __VA_ARGS__);} } while (0)

#ifdef __cplusplus
}
#endif

#endif /* Qxl_QUIXIL_H */

