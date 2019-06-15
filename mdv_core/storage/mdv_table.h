#pragma once
#include <mdv_string.h>
#include <mdv_uuid.h>
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
    MDV_FLD_TYPE_UINT64 = 11,   // uint64
    MDV_FLD_TYPE_FLOAT  = 12,   // float
    MDV_FLD_TYPE_DOUBLE = 13    // double
} mdv_field_type;


typedef struct
{
    mdv_field_type  type;       // field type

    /*
     * Field can contain multiple items.
     * 0 - unlimited array size
     * 1 - only one value allowed in field
     * N - field items count is limited by N
     */
    uint32_t        limit;      // field limit
    mdv_string      name;       // field name
} mdv_field;


#define mdv_table(N)                \
    struct mdv_table_##N            \
    {                               \
        mdv_uuid        uuid;       \
        mdv_string      name;       \
        uint32_t        size;       \
        mdv_field       fields[N];  \
    }


typedef mdv_table(1) mdv_table_base;


typedef uint64_t mdv_row_id;


#define mdv_row(N)                  \
    struct mdv_row_##N              \
    {                               \
        mdv_row_id      id;         \
        uint32_t        size;       \
        mdv_data        fields[N];  \
    }


typedef mdv_row(1) mdv_row_base;


typedef struct
{
    mdv_table_base *table;          // table description
    mdv_storage    *data;           // table data storage

    struct
    {
        mdv_storage *ins;           // insertion log storage
        mdv_storage *del;           // deletion log storage
    } log;
} mdv_table_storage;


#define mdv_table_storage_ok(stor) ((stor).data != 0)


uint32_t mdv_field_type_size(mdv_field_type t);


mdv_table_storage mdv_table_create(mdv_storage *metainf_storage, mdv_table_base const *table);
bool              mdv_table_drop(mdv_storage *metainf_storage, mdv_uuid const *uuid);
mdv_table_storage mdv_table_open(mdv_uuid const *uuid);
void              mdv_table_close(mdv_table_storage *storage);
bool              mdv_table_insert(mdv_table_storage *storage, size_t count, mdv_row_base const **rows);
bool              mdv_table_delete(mdv_table_storage *storage);
