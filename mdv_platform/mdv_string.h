#pragma once
#include "mdv_alloc.h"
#include <stddef.h>
#include <string.h>


typedef struct
{
    size_t  size;
    char   *ptr;
} mdv_string;


#define mdv_str_is_empty(str)       (str).size < 2
#define mdv_str_static(str)         { sizeof(str), str }
#define mdv_str(str)                { strlen(str) + 1, str }
#define mdv_str_null                { 0, 0 }


mdv_string mdv_str_pdup(void *mpool, char const *str);
mdv_string mdv_str_pcat(void *mpool, mdv_string const *dst, mdv_string const *str);
