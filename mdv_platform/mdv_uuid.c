#include "mdv_uuid.h"
#include "mdv_log.h"
#include <string.h>
#include <uuid4.h>


mdv_uuid mdv_uuid_generate()
{
    UUID4_STATE_T state;
    mdv_uuid uuid;

    uuid4_seed(&state);
    uuid4_gen(&state, (UUID4_T*)&uuid);

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
        return 1;
    return 0;
}


size_t mdv_uuid_hash(mdv_uuid const *a)
{
    static size_t const FNV_offset_basis = 0x811c9dc5;
    static size_t const FNV_prime = 0x01000193;

    uint64_t hash = FNV_offset_basis;

    hash = (hash * FNV_prime) ^ a->u64[0];
    hash = (hash * FNV_prime) ^ a->u64[1];

    return (size_t)hash;
}


char const * mdv_uuid_to_str(mdv_uuid const *uuid, char buf[MDV_UUID_STR_LEN])
{
    static char const hex[] = "0123456789ABCDEF";
    size_t const size = 2 * sizeof uuid->u8;

    for(size_t i = 0; i < size; ++i)
    {
        uint8_t const d = (uuid->u8[i / 2] >> ((1 - i % 2) * 4)) & 0xF;
        buf[i] = hex[d];
    }

    buf[MDV_UUID_STR_LEN - 1] = 0;

    return buf;
}


bool mdv_uuid_from_str(mdv_uuid *uuid, char const *str)
{
    size_t const str_len = strlen(str);

    if (str_len != 2 * sizeof uuid->u8)
    {
        MDV_LOGE("uuid_from_str failed. String is too small.");
        return false;
    }

    uuid->u64[0] = 0ull;
    uuid->u64[1] = 0ull;

    for(size_t i = 0; i < str_len; ++i)
    {
        int const sh = (1 - i % 2) * 4;

        switch(str[i])
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
                MDV_LOGE("uuid_from_str failed. String contains invalid character: '%c'.", str[i]);
                return false;
            }
        }
    }

    return true;
}
