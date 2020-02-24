#include "mdv_datum.h"
#include <mdv_log.h>
#include <string.h>


struct mdv_datum
{
    mdv_field_type       type;      ///< Field type
    uint32_t             count;     ///< Data items count
    mdv_allocator const *allocator; ///< Memory allocator
    void                *ptr;       ///< Data pointer
};


#define mdv_datum_type(T, name, field_type)             \
    typedef struct                                      \
    {                                                   \
        mdv_datum            base;                      \
        T                    data[1];                   \
    } mdv_datum_##name;                                 \
                                                        \
    mdv_datum_constructor(T, name)                      \
    {                                                   \
        mdv_datum_##name *datum =                       \
            allocator->alloc(                           \
                offsetof(mdv_datum_##name, data)        \
                    + sizeof(T) * count,                \
                    "datum_"#name);                     \
        if (!datum)                                     \
        {                                               \
            MDV_LOGE("No memory for new datum " #name); \
            return 0;                                   \
        }                                               \
                                                        \
        datum->base.type = field_type;                  \
        datum->base.count = count;                      \
        datum->base.allocator = allocator;              \
        datum->base.ptr = &datum->data;                 \
        memcpy(datum->data, v, sizeof(T) * count);      \
                                                        \
        return &datum->base;                            \
    }                                                   \
                                                        \
    mdv_datum_destructor(name)                          \
    {                                                   \
        datum->allocator->free(datum, #name);           \
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


uint32_t mdv_datum_size(mdv_datum *datum)
{
    return mdv_field_type_size(datum->type)
                * datum->count;
}


void * mdv_datum_ptr(mdv_datum *datum)
{
    return datum->ptr;
}

