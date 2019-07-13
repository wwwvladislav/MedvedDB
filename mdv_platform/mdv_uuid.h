#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "mdv_string.h"


typedef struct
{
    union
    {
        uint8_t  u8[16];
        uint64_t u64[2];

        struct
        {
            int32_t a;
            int16_t b;
            int16_t c;
            int16_t d;
            int16_t e;
            int16_t f;
            int16_t g;
        };
    };
} mdv_uuid;


mdv_uuid    mdv_uuid_generate();
int         mdv_uuid_cmp(mdv_uuid const *a, mdv_uuid const *b);
size_t      mdv_uuid_hash(mdv_uuid const *a);
mdv_string  mdv_uuid_to_str(mdv_uuid const *uuid);
bool        mdv_uuid_from_str(mdv_uuid *uuid, mdv_string const *str);
