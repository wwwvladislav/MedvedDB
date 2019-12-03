#include "mdv_list.h"
#include "mdv_log.h"
#include <string.h>


mdv_list_entry_base * mdv_list_push_back_ptr(mdv_list *l, void const *val, size_t size)
{
    mdv_list_entry_base *entry = (mdv_list_entry_base *)mdv_alloc(offsetof(mdv_list_entry_base, data) + size, "list_entry");

    if (!entry)
    {
        MDV_LOGE("No memory for list entry");
        return 0;
    }

    memcpy(entry->data, val, size);

    mdv_list_emplace_back(l, entry);

    return entry;
}


void mdv_list_clear(mdv_list *l)
{
    for(mdv_list_entry_base *entry = l->next; entry;)
    {
        mdv_list_entry_base *next = entry->next;
        mdv_free(entry, "list_entry");
        entry = next;
    }
    l->next = 0;
    l->last = 0;
}


mdv_list mdv_list_move(mdv_list *l)
{
    mdv_list list =
    {
        .next = l->next,
        .last = l->last
    };

    l->next = 0;
    l->last = 0;

    return list;
}


void mdv_list_exclude(mdv_list *l, mdv_list_entry_base *entry)
{
    mdv_list_entry_base *entry4remove = entry;

    if (entry->prev)
        entry->prev->next = entry->next;
    else
        l->next = entry->next;

    if (entry->next)
        entry->next->prev = entry->prev;
    else
        l->last = entry->prev;
}


void mdv_list_remove(mdv_list *l, mdv_list_entry_base *entry)
{
    mdv_list_exclude(l, entry);
    mdv_free(entry, "list_entry");
}


void mdv_list_emplace_back(mdv_list *l, mdv_list_entry_base *entry)
{
    if (!l->next)
    {
        entry->prev = 0;
        entry->next = 0;
        l->next = entry;
        l->last = entry;
    }
    else
    {
        entry->prev = l->last;
        entry->next = 0;
        l->last->next = entry;
        l->last = entry;
    }
}


void mdv_list_pop_back(mdv_list *l)
{
    mdv_list_remove(l, l->last);
}


bool mdv_list_empty(mdv_list *l)
{
    return l->next == 0;
}
