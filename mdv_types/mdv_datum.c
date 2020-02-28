#include "mdv_datum.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <string.h>


struct mdv_datum
{
    mdv_field_type       type;      ///< Data type
    mdv_data             data;      ///< Data
};


#define mdv_datum_type(T, name, field_type)                     \
    typedef struct                                              \
    {                                                           \
        mdv_datum            base;                              \
        T                    data[1];                           \
    } mdv_datum_##name;                                         \
                                                                \
    mdv_datum * mdv_datum_##name##_create(                      \
                    T const *v,                                 \
                    uint32_t count)                             \
    {                                                           \
        size_t datum_size = offsetof(mdv_datum_##name, data)    \
                                + sizeof(T) * count;            \
                                                                \
        /* Tail for the last zero */                            \
        if (field_type == MDV_FLD_TYPE_CHAR)                    \
            datum_size++;                                       \
                                                                \
        mdv_datum_##name *datum =                               \
            mdv_alloc(                                          \
                offsetof(mdv_datum_##name, data)                \
                    + sizeof(T) * count,                        \
                    "datum_"#name);                             \
        if (!datum)                                             \
        {                                                       \
            MDV_LOGE("No memory for new datum " #name);         \
            return 0;                                           \
        }                                                       \
                                                                \
        datum->base.type = field_type;                          \
        datum->base.data.size = sizeof(T) * count;              \
        datum->base.data.ptr = datum->data;                     \
        memcpy(datum->data, v, sizeof(T) * count);              \
                                                                \
        if (field_type == MDV_FLD_TYPE_CHAR)                    \
            datum->data[count] = 0;                             \
                                                                \
        return &datum->base;                                    \
    }                                                           \
                                                                \
    T const * mdv_datum_as_##name(mdv_datum const *datum)       \
    {                                                           \
        if(datum->type != field_type)                           \
        {                                                       \
            MDV_LOGE("Invalid datum cast for "#name" type");    \
            return 0;                                           \
        }                                                       \
        return (T const *)datum->data.ptr;                      \
    }


mdv_datum_type(bool,        bool,   MDV_FLD_TYPE_BOOL);
mdv_datum_type(char,        char,   MDV_FLD_TYPE_CHAR);
mdv_datum_type(uint8_t,     byte,   MDV_FLD_TYPE_BYTE);
mdv_datum_type(int8_t,      int8,   MDV_FLD_TYPE_INT8);
mdv_datum_type(uint8_t,     uint8,  MDV_FLD_TYPE_UINT8);
mdv_datum_type(int16_t,     int16,  MDV_FLD_TYPE_INT16);
mdv_datum_type(uint16_t,    uint16, MDV_FLD_TYPE_UINT16);
mdv_datum_type(int32_t,     int32,  MDV_FLD_TYPE_INT32);
mdv_datum_type(uint32_t,    uint32, MDV_FLD_TYPE_UINT32);
mdv_datum_type(int64_t,     int64,  MDV_FLD_TYPE_INT64);
mdv_datum_type(uint64_t,    uint64, MDV_FLD_TYPE_UINT64);
mdv_datum_type(float,       float,  MDV_FLD_TYPE_FLOAT);
mdv_datum_type(double,      double, MDV_FLD_TYPE_DOUBLE);

#undef mdv_datum_type


mdv_datum * mdv_datum_create(mdv_field_type type, void const *v, uint32_t size)
{
    switch(type)
    {
        case MDV_FLD_TYPE_BOOL:     return mdv_datum_bool_create(v, size / mdv_field_type_size(type));
        case MDV_FLD_TYPE_CHAR:     return mdv_datum_char_create(v, size / mdv_field_type_size(type));
        case MDV_FLD_TYPE_BYTE:     return mdv_datum_byte_create(v, size / mdv_field_type_size(type));
        case MDV_FLD_TYPE_INT8:     return mdv_datum_int8_create(v, size / mdv_field_type_size(type));
        case MDV_FLD_TYPE_UINT8:    return mdv_datum_uint8_create(v, size / mdv_field_type_size(type));
        case MDV_FLD_TYPE_INT16:    return mdv_datum_int16_create(v, size / mdv_field_type_size(type));
        case MDV_FLD_TYPE_UINT16:   return mdv_datum_uint16_create(v, size / mdv_field_type_size(type));
        case MDV_FLD_TYPE_INT32:    return mdv_datum_int32_create(v, size / mdv_field_type_size(type));
        case MDV_FLD_TYPE_UINT32:   return mdv_datum_uint32_create(v, size / mdv_field_type_size(type));
        case MDV_FLD_TYPE_INT64:    return mdv_datum_int64_create(v, size / mdv_field_type_size(type));
        case MDV_FLD_TYPE_UINT64:   return mdv_datum_uint64_create(v, size / mdv_field_type_size(type));
        case MDV_FLD_TYPE_FLOAT:    return mdv_datum_float_create(v, size / mdv_field_type_size(type));
        case MDV_FLD_TYPE_DOUBLE:   return mdv_datum_double_create(v, size / mdv_field_type_size(type));
        default:
            MDV_LOGE("Inknown type");
    }

    return 0;
}


void mdv_datum_free(mdv_datum *datum)
{
    mdv_free(datum, "datum");
}


uint32_t mdv_datum_size(mdv_datum const *datum)
{
    return datum->data.size;
}


uint32_t mdv_datum_count(mdv_datum const *datum)
{
    return datum->data.size / mdv_field_type_size(datum->type);
}


void * mdv_datum_ptr(mdv_datum const *datum)
{
    return datum->data.ptr;
}


mdv_field_type mdv_datum_type(mdv_datum const *datum)
{
    return datum->type;
}
