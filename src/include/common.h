#ifndef Qxl_COMMON_H
#define Qxl_COMMON_H

#include "quixil.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Remember to free the string after using */
char* Qxl_fstr(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* Qxl_COMMON_H */