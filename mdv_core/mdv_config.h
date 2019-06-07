#pragma once
#include <stdbool.h>
#include <mdv_string.h>
#include <mdv_alloc.h>


typedef struct
{
    mdv_string_t listen;

    mdv_mempool(256) mempool;
} mdv_config;


bool mdv_load_config(char const *path, mdv_config *config);
