#pragma once
#include <mdv_def.h>


/// Data
typedef struct mdv_data
{
    uint32_t    size;       ///< Data size
    void       *ptr;        ///< Data pointer
} mdv_data;


/// Key and value pair
typedef struct mdv_kvdata
{
    mdv_data key;           ///< Key
    mdv_data value;         ///< Value
} mdv_kvdata;
