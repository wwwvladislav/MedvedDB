#pragma once
#include <stdint.h>
#include <stddef.h>
#include <mdv_string.h>
#include <mdv_uuid.h>
#include "mdv_bigint.h"


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Data
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct
{
    uint32_t    size;
    void       *ptr;
} mdv_data;


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Map
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct mdv_map_field_desc
{
    mdv_data                     key;
    mdv_data                     value;
    struct mdv_map_field_desc   *next;
    uint8_t                      empty:1;
    uint8_t                      changed:1;
} mdv_map_field_desc;


#define mdv_map_field(key_t, val_t)                             \
    struct                                                      \
    {                                                           \
        mdv_map_field_desc  m;                                  \
        key_t               key;                                \
        val_t               value;                              \
    }


#define mdv_map_field_set(field, val)                           \
    {                                                           \
        field.value         = val;                              \
        field.m.empty       = 0;                                \
        field.m.changed     = 1;                                \
    }


#define mdv_map_field_init(field, key_val)                      \
    {                                                           \
        field.key           = key_val;                          \
        field.m.key.size    = sizeof(field.key);                \
        field.m.key.ptr     = &field.key;                       \
        field.m.value.size  = sizeof(field.value);              \
        field.m.value.ptr   = &field.value;                     \
        field.m.empty       = 1;                                \
        field.m.changed     = 0;                                \
        field.m.next        = (mdv_map_field_desc*)(&field + 1);\
    }


#define mdv_map_field_init_last(field, key_val)                 \
    mdv_map_field_init(field, key_val)                          \
    field.m.next        = 0;


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Field
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Table
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define mdv_table(N)                \
    struct mdv_table_##N            \
    {                               \
        mdv_uuid        uuid;       \
        mdv_string      name;       \
        uint32_t        size;       \
        mdv_field       fields[N];  \
    }


typedef mdv_table(1) mdv_table_base;


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Row
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define mdv_row(N)                  \
    struct mdv_row_##N              \
    {                               \
        uint32_t        size;       \
        mdv_data        fields[N];  \
    }


typedef mdv_row(1) mdv_row_base;


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Row ID
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define mdv_rowid(NL, RL)               \
    struct                              \
    {                                   \
        mdv_bigint(NL)  nide_id;        \
        mdv_bigint(RL)  row_id;         \
    }


uint32_t mdv_field_type_size(mdv_field_type t);
