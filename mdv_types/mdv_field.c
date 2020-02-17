#include "mdv_field.h"
#include <mdv_log.h>
#include <mdv_assert.h>


uint32_t mdv_field_type_size(mdv_field_type t)
{
    switch(t)
    {
        case MDV_FLD_TYPE_BOOL:     return sizeof(bool);
        case MDV_FLD_TYPE_CHAR:     return sizeof(char);
        case MDV_FLD_TYPE_BYTE:     return sizeof(uint8_t);
        case MDV_FLD_TYPE_INT8:     return sizeof(int8_t);
        case MDV_FLD_TYPE_UINT8:    return sizeof(uint8_t);
        case MDV_FLD_TYPE_INT16:    return sizeof(int16_t);
        case MDV_FLD_TYPE_UINT16:   return sizeof(uint16_t);
        case MDV_FLD_TYPE_INT32:    return sizeof(int32_t);
        case MDV_FLD_TYPE_UINT32:   return sizeof(uint32_t);
        case MDV_FLD_TYPE_INT64:    return sizeof(int64_t);
        case MDV_FLD_TYPE_UINT64:   return sizeof(uint64_t);
        case MDV_FLD_TYPE_FLOAT:    return sizeof(float);
        case MDV_FLD_TYPE_DOUBLE:   return sizeof(double);
    }
    MDV_LOGE("Unknown type: %u", t);
    return 0;
}


mdv_static_assert(sizeof(bool) == 1);
mdv_static_assert(sizeof(char) == 1);
mdv_static_assert(sizeof(int8_t) == 1);
mdv_static_assert(sizeof(uint8_t) == 1);
mdv_static_assert(sizeof(int16_t) == 2);
mdv_static_assert(sizeof(uint16_t) == 2);
mdv_static_assert(sizeof(int32_t) == 4);
mdv_static_assert(sizeof(uint32_t) == 4);
mdv_static_assert(sizeof(int64_t) == 8);
mdv_static_assert(sizeof(uint64_t) == 8);
mdv_static_assert(sizeof(float) == 4);
mdv_static_assert(sizeof(double) == 8);


