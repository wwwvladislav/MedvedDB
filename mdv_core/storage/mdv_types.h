#pragma once
#include <stdint.h>
#include <stddef.h>


typedef struct
{
    uint32_t    size;
    void       *ptr;
} mdv_data;


typedef struct s_mdv_map_field_desc
{
    mdv_data                     key;
    mdv_data                     value;
    struct s_mdv_map_field_desc *next;
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

