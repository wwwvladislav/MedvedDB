#pragma once
#include <mdv_string.h>
#include <mdv_uuid.h>
#include <mdv_bigint.h>
#include <mdv_def.h>


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Data
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct
{
    uint32_t    size;       ///< Data size
    void       *ptr;        ///< Data pointer
} mdv_data;


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Object ID
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef union __attribute__((packed))
{
    uint8_t  u8[12];    ///< representation as u8 array

    struct __attribute__((packed))
    {
        uint32_t node;  ///< Node identifier
        uint64_t id;    ///< Object identifier
    };
} mdv_objid;


mdv_string mdv_objid_to_str(mdv_objid const *objid);


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Global obj ID
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct
{
    mdv_uuid node;          ///< Peer/node UUID
    uint64_t id;            ///< Global and unique for node identifier
} mdv_gobjid;


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
        mdv_string      name;       \
        mdv_uuid        id;         \
        uint32_t        size;       \
        mdv_field       fields[N];  \
    }


typedef mdv_table(0) mdv_table_base;


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Row
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define mdv_row(N)                  \
    struct mdv_row_##N              \
    {                               \
        uint32_t        size;       \
        mdv_data        fields[N];  \
    }


typedef mdv_row(0) mdv_row_base;


uint32_t mdv_field_type_size(mdv_field_type t);


mdv_table_base * mdv_table_base_clone(mdv_table_base const *table);
