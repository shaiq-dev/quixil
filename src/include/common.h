#ifndef Qxl_COMMON_H
#define Qxl_COMMON_H

#include "quixil.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Remember to free the string after using */
    char *Qxl_fstr (const char *format, ...);

    /* Hash strings using FNV-1a algorithim */
    uint32_t Qxl_hash_str (const char *key, int length);

    /* Convert a double to string */
    char *Qxl_num_as_str (int value);

#ifdef __cplusplus
}
#endif

#endif /* Qxl_COMMON_H */