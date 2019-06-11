#include "mdv_uuid.h"
#include "mdv_log.h"
#include <stdlib.h>
#include <time.h>


mdv_uuid mdv_uuid_generate()
{
    srand((unsigned int)time(0));

    mdv_uuid uuid =
    {
        .a = rand(),
        .b = (int16_t)rand(),
        .c = (int16_t)((rand() & 0x0fff) | 0x4000),
        .d = (int16_t)(rand() % 0x3fff + 0x8000),
        .e = (int16_t)rand(),
        .f = (int16_t)rand(),
        .g = (int16_t)rand()

    };
    return uuid;
}


int mdv_uuid_cmp(mdv_uuid const *a, mdv_uuid const *b)
{
    if (a->u64[0] < b->u64[0])
        return -1;
    else if (a->u64[0] > b->u64[0])
        return 1;
    else if (a->u64[1] < b->u64[1])
        return -1;
    else if (a->u64[1] > b->u64[1])
    return 0;
}


bool mdv_uuid_to_str(mdv_uuid const *uuid, mdv_string *dst)
{
    if (dst->size != 2 * sizeof uuid->u8 + 1)
    {
        MDV_LOGE("uuid_to_str failed. Destination buffer is too small.");
        return false;
    }

    static char const hex[] = "0123456789ABCDEF";
    size_t const size = 2 * sizeof uuid->u8;

    for(size_t i = 0; i < size; ++i)
    {
        uint8_t const d = (uuid->u8[i / 2] >> ((1 - i % 2) * 4)) & 0xF;
        dst->ptr[i] = hex[d];
    }

    dst->ptr[size] = 0;

    return true;
}


bool mdv_uuid_from_str(mdv_uuid *uuid, mdv_string const *str)
{
    if (str->size != 2 * sizeof uuid->u8 + 1)
    {
        MDV_LOGE("uuid_from_str failed. String is too small.");
        return false;
    }

    uuid->u64[0] = 0ull;
    uuid->u64[1] = 0ull;

    for(size_t i = 0; i < str->size - 1; ++i)
    {
        int const sh = (1 - i % 2) * 4;

        switch(str->ptr[i])
        {
            case '0': break;
            case '1': uuid->u8[i / 2] |= 1 << sh; break;
            case '2': uuid->u8[i / 2] |= 2 << sh; break;
            case '3': uuid->u8[i / 2] |= 3 << sh; break;
            case '4': uuid->u8[i / 2] |= 4 << sh; break;
            case '5': uuid->u8[i / 2] |= 5 << sh; break;
            case '6': uuid->u8[i / 2] |= 6 << sh; break;
            case '7': uuid->u8[i / 2] |= 7 << sh; break;
            case '8': uuid->u8[i / 2] |= 8 << sh; break;
            case '9': uuid->u8[i / 2] |= 9 << sh; break;
            case 'a':
            case 'A': uuid->u8[i / 2] |= 10 << sh; break;
            case 'b':
            case 'B': uuid->u8[i / 2] |= 11 << sh; break;
            case 'c':
            case 'C': uuid->u8[i / 2] |= 12 << sh; break;
            case 'd':
            case 'D': uuid->u8[i / 2] |= 13 << sh; break;
            case 'e':
            case 'E': uuid->u8[i / 2] |= 14 << sh; break;
            case 'f':
            case 'F': uuid->u8[i / 2] |= 15 << sh; break;
            default:
            {
                MDV_LOGE("uuid_from_str failed. String contains invalid character: '%c'.", str->ptr[i]);
                return false;
            }
        }
    }

    return true;
}