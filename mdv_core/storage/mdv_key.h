#pragma once
#include <stdint.h>
#include <stddef.h>


#define mdv_key(S)                      \
    struct __attribute__((packed))      \
    {                                   \
        uint32_t    node;               \
        uint8_t     size;               \
        uint8_t     id[S];              \
    }


typedef mdv_key(1) mdv_key_base;


#define mdv_key_size(key) (offsetof(mdv_key_base, id) + (key).size)

