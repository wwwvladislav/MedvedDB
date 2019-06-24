#include "mdv_types.h"
#include <mdv_log.h>


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
