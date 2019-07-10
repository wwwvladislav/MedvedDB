#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <mdv_string.h>
#include <mdv_stack.h>


enum { MDV_MAX_CONFIGURED_CLUSTER_NODES = 256 };


typedef struct
{
    struct
    {
        mdv_string level;
    } log;

    struct
    {
        mdv_string listen;
        uint32_t   workers;
    } server;

    struct
    {
        uint32_t retry_interval;
        uint32_t keep_idle;
        uint32_t keep_count;
        uint32_t keep_interval;
    } connection;

    struct
    {
        mdv_string path;
    } storage;

    struct
    {
        uint32_t   batch_size;
    } transaction;

    struct
    {
        uint32_t    size;
        char const *nodes[MDV_MAX_CONFIGURED_CLUSTER_NODES];
    } cluster;

    mdv_stack(uint8_t, 4 * 1024) mempool;
} mdv_config;


extern mdv_config MDV_CONFIG;


bool mdv_load_config(char const *path);

