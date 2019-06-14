#pragma once
#include <mdv_string.h>
#include "mdv_types.h"
#include "mdv_storage.h"
#include <stdint.h>


typedef enum
{
    MDV_FLD_TYPE_BOOL   = 1,    // bool
    MDV_FLD_TYPE_CHAR   = 2,    // char
    MDV_FLD_TYPE_BYTE   = 3,    // byte
    MDV_FLD_TYPE_INT8   = 4,    // int8
    MDV_FLD_TYPE_UINT8  = 5,    // uint8
    MDV_FLD_TYPE_INT16  = 6,    // int16
    MDV_FLD_TYPE_UINT16 = 7,    // uint16
    MDV_FLD_TYPE_INT32  = 8,    // int32
    MDV_FLD_TYPE_UINT32 = 9,    // uint32
    MDV_FLD_TYPE_INT64  = 10,   // int64
    MDV_FLD_TYPE_UINT64 = 11    // uint64
} mdv_field_type;


typedef struct
{
    mdv_field_type  type;       // field type
    mdv_string      name;       // field name
} mdv_field;


#define mdv_table(N)                \
    struct mdv_table_##N            \
    {                               \
        mdv_string      name;       \
        uint32_t        size;       \
        mdv_field       fields[N];  \
    }


typedef mdv_table(1) mdv_table_base;


typedef struct
{
    mdv_storage *data;              // table data storage

    struct
    {
        mdv_storage *ins;           // insertion log storage
        mdv_storage *del;           // deletion log storage
    } log;
} mdv_table_storage;


#define mdv_table_storage_ok(stor) ((stor).data != 0)


uint32_t mdv_field_size(mdv_field const *fld);


mdv_table_storage mdv_table_create(mdv_storage *metainf_storage, mdv_table_base const *table);
bool              mdv_table_drop(mdv_storage *metainf_storage, char const *name);
void              mdv_table_close(mdv_table_storage *storage);
