#include "mdv_resultset.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <stdatomic.h>


struct mdv_resultset
{
    atomic_uint_fast32_t    rc;         ///< References counter
    mdv_table              *table;      ///< Table descriptor
    mdv_enumerator         *result;     ///< Result set iterator
};


mdv_resultset * mdv_resultset_create(mdv_table *table, mdv_enumerator *result)
{
    mdv_resultset *resultset = mdv_alloc(sizeof(mdv_resultset), "resultset");

    if (!resultset)
    {
        MDV_LOGE("No memory for result set");
        return 0;
    }

    atomic_init(&resultset->rc, 1);

    resultset->table = mdv_table_retain(table);
    resultset->result = mdv_enumerator_retain(result);

    return resultset;
}


mdv_resultset * mdv_resultset_retain(mdv_resultset *resultset)
{
    atomic_fetch_add_explicit(&resultset->rc, 1, memory_order_acquire);
    return resultset;
}


static void mdv_resultset_free(mdv_resultset *resultset)
{
    mdv_table_release(resultset->table);
    mdv_enumerator_release(resultset->result);
    mdv_free(resultset, "resultset");
}


uint32_t mdv_resultset_release(mdv_resultset *resultset)
{
    uint32_t rc = 0;

    if (resultset)
    {
        rc = atomic_fetch_sub_explicit(&resultset->rc, 1, memory_order_release) - 1;

        if (!rc)
            mdv_resultset_free(resultset);
    }

    return rc;
}


mdv_table * mdv_resultset_table(mdv_resultset *resultset)
{
    return mdv_table_retain(resultset->table);
}


mdv_enumerator * mdv_resultset_enumerator(mdv_resultset *resultset)
{
    return mdv_enumerator_retain(resultset->result);
}
