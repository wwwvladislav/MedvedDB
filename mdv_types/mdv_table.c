#include "mdv_table.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <stdatomic.h>
#include <assert.h>


struct mdv_table
{
    atomic_uint_fast32_t    rc;             ///< References counter
    mdv_uuid                id;             ///< Table unique identifier
    mdv_table_desc          desc;           ///< Table Description
};


mdv_table * mdv_table_create(mdv_uuid const *id, mdv_table_desc const *desc)
{
    // Structure size calculation
    size_t size = sizeof(mdv_table) + desc->size * sizeof(mdv_field);
    size += desc->name.size;
    for(size_t i = 0; i < desc->size; ++i)
        size += desc->fields[i].name.size;

    mdv_table *table = mdv_alloc(size, "table");

    if (!table)
    {
        MDV_LOGE("No memory for table descriptor");
        return 0;
    }

    atomic_init(&table->rc, 1u);

    mdv_field *fields = (mdv_field *)(table + 1);

    char *strings = (char*)(fields + desc->size);

    table->id = *id;

    table->desc.name.size = desc->name.size;
    table->desc.name.ptr = strings;
    memcpy(strings, desc->name.ptr, desc->name.size);
    strings += desc->name.size;

    table->desc.size = desc->size;
    table->desc.fields = fields;

    for(size_t i = 0; i < desc->size; ++i)
    {
        mdv_field       *dst = fields + i;
        mdv_field const *src = desc->fields + i;

        dst->type = src->type;
        dst->limit = src->limit;
        dst->name.size = src->name.size;
        dst->name.ptr = strings;

        memcpy(strings, src->name.ptr, src->name.size);
        strings += src->name.size;
    }

    assert(strings - (char const*)table == size);

    return table;
}


mdv_table * mdv_table_retain(mdv_table *table)
{
    atomic_fetch_add_explicit(&table->rc, 1, memory_order_acquire);
    return table;
}


uint32_t mdv_table_release(mdv_table *table)
{
    if (!table)
        return 0;

    uint32_t rc = atomic_fetch_sub_explicit(&table->rc, 1, memory_order_release) - 1;

    if (!rc)
        mdv_free(table, "table");

    return rc;
}


mdv_uuid const * mdv_table_uuid(mdv_table const *table)
{
    return &table->id;
}


mdv_table_desc const * mdv_table_description(mdv_table const *table)
{
    return &table->desc;
}
