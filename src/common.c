#include "include/common.h"

char *
Qxl_fstr(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    size_t len = vsnprintf(NULL, 0, format, args);

    char *result = (char *)malloc((len + 1) * sizeof(char));
    if (result == NULL)
    {
        va_end(args);
        return NULL;
    }

    vsnprintf(result, len + 1, format, args);
    va_end(args);
    return result;
}

uint32_t
Qxl_hash_str(const char *key, int length)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++)
    {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

char *
Qxl_num_as_str(int value)
{
    char *str = NULL;
    asprintf(&str, "%d", value);
    return str;
}