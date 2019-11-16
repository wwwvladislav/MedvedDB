#include "mdv_types.h"
#include <mdv_log.h>


mdv_string mdv_objid_to_str(mdv_objid const *objid)
{
    static _Thread_local char buff[2 * sizeof objid->u8 + 1];
    mdv_string dst = mdv_str_static(buff);

    static char const hex[] = "0123456789ABCDEF";
    size_t const size = 2 * sizeof objid->u8;

    for(size_t i = 0; i < size; ++i)
    {
        uint8_t const d = (objid->u8[i / 2] >> ((1 - i % 2) * 4)) & 0xF;
        dst.ptr[i] = hex[d];
    }

    dst.ptr[size] = 0;

    return dst;
}


uint32_t mdv_field_type_size(mdv_field_type t)
{
    switch(t)
    {
        case MDV_FLD_TYPE_BOOL:     return 1u;
        case MDV_FLD_TYPE_CHAR:     return 1u;
        case MDV_FLD_TYPE_BYTE:     return 1u;
        case MDV_FLD_TYPE_INT8:     return 1u;
        case MDV_FLD_TYPE_UINT8:    return 1u;
        case MDV_FLD_TYPE_INT16:    return 2u;
        case MDV_FLD_TYPE_UINT16:   return 2u;
        case MDV_FLD_TYPE_INT32:    return 4u;
        case MDV_FLD_TYPE_UINT32:   return 4u;
        case MDV_FLD_TYPE_INT64:    return 8u;
        case MDV_FLD_TYPE_UINT64:   return 8u;
        case MDV_FLD_TYPE_FLOAT:    return 4u;
        case MDV_FLD_TYPE_DOUBLE:   return 8u;
    }
    MDV_LOGE("Unknown type: %u", t);
    return 0;
}


mdv_table_base * mdv_table_base_clone(mdv_table_base const *table)
{
    // TODO
    return 0;
}

