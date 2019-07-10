#pragma once
#include <stdint.h>


typedef struct
{
    uint16_t msg_id;
    uint16_t req_id;
    uint32_t body_size;
} mdv_msg_hdr;
