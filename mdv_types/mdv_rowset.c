#include "mdv_rowset.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_list.h>
#include <assert.h>


typedef mdv_list_entry(mdv_row) mdv_rowlist_entry;


/// Set of rows
typedef struct
{
    mdv_rowset              base;       ///< Base type for rowset
    mdv_list                rows;       ///< Rows list (list<mdv_row>)
} mdv_rowset_impl;


/// Set of rows enumerator
typedef struct
{
    mdv_enumerator          base;       ///< Base type for rowset enumerator
    mdv_rowset_impl        *rowset;     ///< Set of rows
    mdv_rowlist_entry      *current;    ///< Current row
} mdv_rowset_enumerator_impl;


static mdv_rowset * mdv_rowset_impl_retain(mdv_rowset *rowset)
{
    atomic_fetch_add_explicit(&rowset->rc, 1, memory_order_acquire);
    return rowset;
}


static uint32_t mdv_rowset_impl_release(mdv_rowset *rowset)
{
    uint32_t rc = 0;

    if (rowset)
    {
        mdv_rowset_impl *impl = (mdv_rowset_impl *)rowset;

        rc = atomic_fetch_sub_explicit(&rowset->rc, 1, memory_order_release) - 1;

        if (!rc)
        {
            mdv_list_clear(&impl->rows);
            mdv_table_release(rowset->table);
            mdv_free(impl, "mem_rowset");
        }
    }

    return rc;
}


static size_t mdv_rowset_impl_append(mdv_rowset *rowset, mdv_data const **rows, size_t count)
{
    mdv_rowset_impl *impl = (mdv_rowset_impl *)rowset;

    size_t appended = 0;

    uint32_t const cols = mdv_rowset_columns(rowset);

    for(size_t i = 0; i < count; ++i)
    {
        mdv_data const *row = rows[i];

        // Memory size calculation which is required for row
        size_t row_size = offsetof(mdv_rowlist_entry, data)
                            + offsetof(mdv_row, fields)
                            + sizeof(mdv_data) * cols;
        for(uint32_t j = 0; j < cols; ++j)
            row_size += row[i].size;

        // Memory allocation for new row
        mdv_rowlist_entry *entry = mdv_alloc(row_size, "rowlist_entry");

        if (!entry)
        {
            MDV_LOGE("No memory for new row");
            return appended;
        }

        char *dataspace = (char *)(entry->data.fields + cols);

        // Copy fields to row
        for(uint32_t j = 0; j < cols; ++j)
        {
            mdv_data       *dst = entry->data.fields + j;
            mdv_data const *src = row + j;

            dst->size = src->size;
            dst->ptr = dataspace;

            memcpy(dataspace, src->ptr, src->size);

            dataspace += src->size;
        }

        assert(dataspace - (char const*)entry == row_size);

        // Add row to rows set
        mdv_list_emplace_back(&impl->rows, (mdv_list_entry_base*)entry);

        ++appended;
    }

    return appended;
}


static mdv_enumerator * mdv_rowset_enumerator_impl_retain(mdv_enumerator *enumerator)
{
    atomic_fetch_add_explicit(&enumerator->rc, 1, memory_order_acquire);
    return enumerator;
}


static uint32_t mdv_rowset_enumerator_impl_release(mdv_enumerator *enumerator)
{
    uint32_t rc = 0;

    if (enumerator)
    {
        mdv_rowset_enumerator_impl *impl = (mdv_rowset_enumerator_impl *)enumerator;

        rc = atomic_fetch_sub_explicit(&enumerator->rc, 1, memory_order_release) - 1;

        if (!rc)
        {
            mdv_rowset_release(&impl->rowset->base);
            mdv_free(enumerator, "rowset_enumerator");
        }
    }

    return rc;
}


static mdv_errno mdv_rowset_enumerator_impl_reset(mdv_enumerator *enumerator)
{
    mdv_rowset_enumerator_impl *impl = (mdv_rowset_enumerator_impl *)enumerator;
    impl->current = 0;
    return MDV_OK;
}


static mdv_errno mdv_rowset_enumerator_impl_next(mdv_enumerator *enumerator)
{
    mdv_rowset_enumerator_impl *impl = (mdv_rowset_enumerator_impl *)enumerator;
    mdv_list *rows = &impl->rowset->rows;

    if (!impl->current)
        impl->current = (mdv_rowlist_entry*)rows->next;
    else
        impl->current = (mdv_rowlist_entry*)impl->current->next;

    return impl->current ? MDV_OK : MDV_FAILED;
}


static void * mdv_rowset_enumerator_impl_current(mdv_enumerator *enumerator)
{
    mdv_rowset_enumerator_impl *impl = (mdv_rowset_enumerator_impl *)enumerator;
    return impl->current ? &impl->current->data : 0;
}


static mdv_enumerator * mdv_rowset_impl_enumerator(mdv_rowset *rowset)
{
    mdv_rowset_enumerator_impl *enumerator = mdv_alloc(sizeof(mdv_rowset_enumerator_impl), "rowset_enumerator");

    if (!enumerator)
    {
        MDV_LOGE("No memory for rows iterator");
        return 0;
    }

    atomic_init(&enumerator->base.rc, 1);

    static mdv_ienumerator const vtbl =
    {
        .retain = mdv_rowset_enumerator_impl_retain,
        .release = mdv_rowset_enumerator_impl_release,
        .reset = mdv_rowset_enumerator_impl_reset,
        .next = mdv_rowset_enumerator_impl_next,
        .current = mdv_rowset_enumerator_impl_current
    };

    enumerator->base.vptr = &vtbl;

    enumerator->rowset = (mdv_rowset_impl*)mdv_rowset_retain(rowset);

    mdv_rowset_enumerator_impl_reset(&enumerator->base);

    return &enumerator->base;
}


mdv_rowset * mdv_rowset_create(mdv_table *table)
{
    mdv_rowset_impl *impl = mdv_alloc(sizeof(mdv_rowset_impl), "mem_rowset");

    if (!impl)
    {
        MDV_LOGE("No memory for rowset");
        return 0;
    }

    static mdv_irowset const vtbl =
    {
        .retain = mdv_rowset_impl_retain,
        .release = mdv_rowset_impl_release,
        .append = mdv_rowset_impl_append,
        .enumerator = mdv_rowset_impl_enumerator,
    };

    atomic_init(&impl->base.rc, 1);

    impl->base.vptr = &vtbl;

    impl->base.table = mdv_table_retain(table);

    memset(&impl->rows, 0, sizeof(impl->rows));

    return &impl->base;
}


mdv_rowset * mdv_rowset_retain(mdv_rowset *rowset)
{
    return rowset->vptr->retain(rowset);
}


uint32_t mdv_rowset_release(mdv_rowset *rowset)
{
    if (rowset)
        return rowset->vptr->release(rowset);
    return 0;
}


uint32_t mdv_rowset_columns(mdv_rowset const *rowset)
{
    return mdv_table_description(rowset->table)->size;
}


size_t mdv_rowset_append(mdv_rowset *rowset, mdv_data const **rows, size_t count)
{
    return rowset->vptr->append(rowset, rows, count);
}


mdv_enumerator * mdv_rowset_enumerator(mdv_rowset *rowset)
{
    return rowset->vptr->enumerator(rowset);
}
