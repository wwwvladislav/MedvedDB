#pragma once
#include <stdbool.h>
#include <mdv_string.h>

typedef struct
{
    mdv_string_t listen;
} mdv_config_t;

bool mdv_load_config(char const *path);

extern mdv_config_t mdv_config;
