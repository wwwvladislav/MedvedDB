#pragma once
#include <stdbool.h>
#include <mdv_string.h>
#include <mdv_alloc.h>


typedef struct
{
    struct
    {
        mdv_string listen;
    } server;

    struct
    {
        mdv_string path;
    } storage;

    mdv_mempool(1024) mempool;
} mdv_config;


bool mdv_load_config(char const *path, mdv_config *config);
