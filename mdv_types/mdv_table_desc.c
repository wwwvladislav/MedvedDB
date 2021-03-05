#include "mdv_table_desc.h"
#include <mdv_list.h>
#include <mdv_vector.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <string.h>


mdv_table_desc * mdv_table_desc_create(char const *name)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    size_t const desc_size = sizeof(mdv_table_desc)
                                + sizeof(mdv_vector*)
                                + sizeof(mdv_list);

    mdv_table_desc *desc = mdv_alloc(desc_size, "table_desc");

    if(!desc)
    {
        MDV_LOGE("No memory for table description");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, desc, "table_desc");

    memset(desc, 0, desc_size);

    mdv_vector **ppfields = (mdv_vector**)(desc + 1);

    *ppfields = mdv_vector_create(2, sizeof(mdv_field), &mdv_default_allocator);

    if (!*ppfields)
    {
        MDV_LOGE("No memory for fields vector");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, *ppfields);

    mdv_list *strings = (mdv_list *)(ppfields + 1);

    mdv_list_entry_base *tbl_name = mdv_list_push_back_data(strings, name, strlen(name) + 1);

    if(!tbl_name)
    {
        MDV_LOGE("No memory for table name");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    desc->name          = tbl_name->data;
    desc->size          = 0;
    desc->dynamic_alloc = true;
    desc->fields        = mdv_vector_data(*ppfields);

    return desc;
}


void mdv_table_desc_free(mdv_table_desc *desc)
{
    if(desc && desc->dynamic_alloc)
    {
        mdv_vector **ppfields = (mdv_vector**)(desc + 1);
        mdv_list *strings = (mdv_list *)(ppfields + 1);
        mdv_vector_release(*ppfields);
        mdv_list_clear(strings);
        mdv_free(desc, "table_desc");
    }
}


bool mdv_table_desc_append(mdv_table_desc *desc, mdv_field const *field)
{
    if (!desc->dynamic_alloc)
    {
        MDV_LOGE("Table description isn't extendable");
        return false;
    }

    mdv_vector **ppfields = (mdv_vector**)(desc + 1);
    mdv_list *strings = (mdv_list *)(ppfields + 1);

    mdv_field *new_field = mdv_vector_push_back(*ppfields, field);

    if(!new_field)
    {
        MDV_LOGE("No memory for new field");
        return false;
    }

    mdv_list_entry_base *name = mdv_list_push_back_data(strings, field->name, strlen(field->name) + 1);

    if(!name)
    {
        MDV_LOGE("No memory for field name");
        mdv_vector_erase(*ppfields, new_field);
        return false;
    }

    new_field->name = name->data;

    desc->size = (uint32_t)mdv_vector_size(*ppfields);
    desc->fields = mdv_vector_data(*ppfields);

    return true;
}
