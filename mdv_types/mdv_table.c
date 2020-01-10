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


/**
 * @brief Table structure size calculation
 */
static size_t mdv_table_size(mdv_table_desc const *desc, mdv_bitset const *mask, uint32_t *fields_count)
{
    size_t size = 0;

    *fields_count = 0;

    for(size_t i = 0; i < desc->size; ++i)
    {
        if (mask && !mdv_bitset_test(mask, i))
            continue;

        size += desc->fields[i].name.size;
        ++*fields_count;
    }

    size += sizeof(mdv_table)
            + desc->name.size
            + *fields_count * sizeof(mdv_field);

    return size;
}


static mdv_table * mdv_table_create_impl(mdv_uuid const       *id,
                                         mdv_table_desc const *desc,
                                         mdv_bitset const     *mask)
{
    uint32_t fields_count;

    size_t const size = mdv_table_size(desc, mask, &fields_count);

    mdv_table *table = mdv_alloc(size, "table");

    if (!table)
    {
        MDV_LOGE("No memory for table descriptor");
        return 0;
    }

    atomic_init(&table->rc, 1u);

    mdv_field *fields = (mdv_field *)(table + 1);

    char *strings = (char*)(fields + fields_count);

    table->id = *id;

    table->desc.name.size = desc->name.size;
    table->desc.name.ptr = strings;
    memcpy(strings, desc->name.ptr, desc->name.size);
    strings += desc->name.size;

    table->desc.size = fields_count;
    table->desc.fields = fields;

    for(size_t i = 0, n = 0; i < desc->size; ++i)
    {
        if (mask && !mdv_bitset_test(mask, i))
            continue;

        mdv_field       *dst = fields + n;
        mdv_field const *src = desc->fields + i;

        dst->type = src->type;
        dst->limit = src->limit;
        dst->name.size = src->name.size;
        dst->name.ptr = strings;

        memcpy(strings, src->name.ptr, src->name.size);
        strings += src->name.size;

        ++n;
    }

    assert(strings - (char const*)table == size);

    return table;
}


mdv_table * mdv_table_create(mdv_uuid const *id, mdv_table_desc const *desc)
{
    return mdv_table_create_impl(id, desc, 0);
}


mdv_table * mdv_table_slice(mdv_table const *table, mdv_bitset const *fields)
{
    return mdv_table_create_impl(&table->id, &table->desc, fields);
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
