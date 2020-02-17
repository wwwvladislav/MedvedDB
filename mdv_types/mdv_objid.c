#include "mdv_objid.h"


char const * mdv_objid_to_str(mdv_objid const *objid)
{
    static _Thread_local char dst[2 * sizeof objid->u8 + 1];

    static char const hex[] = "0123456789ABCDEF";
    size_t const size = 2 * sizeof objid->u8;

    for(size_t i = 0; i < size; ++i)
    {
        uint8_t const d = (objid->u8[i / 2] >> ((1 - i % 2) * 4)) & 0xF;
        dst[i] = hex[d];
    }

    dst[size] = 0;

    return dst;
}
