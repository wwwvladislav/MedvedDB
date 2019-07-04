#include "mdv_list.h"
#include "mdv_alloc.h"
#include "mdv_log.h"
#include <string.h>


int _mdv_list_push_back(mdv_list *l, void const *val, size_t size)
{
    mdv_list_entry_base *entry = (mdv_list_entry_base *)mdv_alloc(offsetof(mdv_list_entry_base, item) + size);

    if (!entry)
    {
        MDV_LOGE("No memory for list entry");
        return 0;
    }

    memcpy(entry->item, val, size);
    entry->next = 0;

    _mdv_list_emplace_back(l, entry);

    return 1;
}


void _mdv_list_clear(mdv_list *l)
{
    for(mdv_list_entry_base *entry = l->next; entry;)
    {
        mdv_list_entry_base *next = entry->next;
        mdv_free(entry);
        entry = next;
    }
    l->next = 0;
    l->last = 0;
}

void _mdv_list_remove_next(mdv_list *l, mdv_list_entry_base *entry)
{
    if (!entry)
    {
        // Remove first entry
        mdv_list_entry_base *entry4remove = l->next;
        if (entry4remove)
        {
            l->next = entry4remove->next;
            mdv_free(entry4remove);
        }
        if (!l->next)
            l->last = 0;
    }
    else
    {
        mdv_list_entry_base *entry4remove = entry->next;
        if (entry4remove)
        {
            entry->next = entry4remove->next;
            mdv_free(entry4remove);
        }
        if (!entry->next)
            l->last = entry;
    }
}


void _mdv_list_emplace_back(mdv_list *l, mdv_list_entry_base *entry)
{
    if (!l->next)
    {
        l->next = entry;
        l->last = entry;
    }
    else
    {
        l->last->next = entry;
        l->last = entry;
    }
}
