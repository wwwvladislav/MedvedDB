#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <mdv_string.h>
#include <mdv_stack.h>


typedef struct
{
    struct
    {
        mdv_string level;
    } log;

    struct
    {
        mdv_string listen;
    } server;

    struct
    {
        mdv_string path;
    } storage;

    mdv_stack(uint8_t, 1024) mempool;
} mdv_config;


bool mdv_load_config(char const *path, mdv_config *config);

