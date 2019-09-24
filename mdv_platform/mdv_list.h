/**
 * @file
 * @brief Double linked list.
 */

/*
    List structure:
        head <-> entry1<-> ... <->entryN-> NULL

    Each list entry contains pointer to the next and previous entry and data. Last entry points to null.
 */

#pragma once
#include "mdv_alloc.h"


/// Base type for list entry
typedef struct mdv_list_entry_base
{
    struct mdv_list_entry_base *next;       ///< Pointer to the next list entry
    struct mdv_list_entry_base *prev;       ///< Pointer to the previous list entry
    char data[1];                           ///< List item data
} mdv_list_entry_base;


/**
 * @brief List entry definition
 *
 * @param type [in] list item type
 */
#define mdv_list_entry(type)                \
    struct                                  \
    {                                       \
        mdv_list_entry_base *next;          \
        mdv_list_entry_base *prev;          \
        type data;                          \
    }


/// List
typedef struct mdv_list
{
    mdv_list_entry_base *next;              ///< Pointer to the next list entry
    mdv_list_entry_base *last;              ///< Pointer to the last list entry
} mdv_list;


/**
 * @brief Append new item to the back of list
 *
 * @param l [in]        list
 * @param val_ptr [in]  value pointer
 * @param size [in]     value size
 *
 *
 * @return nonzero if new item is inserted
 * @return zero if non memory
 */
mdv_list_entry_base * mdv_list_push_back_ptr(mdv_list *l, void const *val, size_t size);


/**
 * @brief Append new item to the back of list
 *
 * @param l [in]    list
 * @param val [in]  value
 *
 * @return nonzero pointer to entry if new item is inserted
 * @return NULL if non memory
 */
#define mdv_list_push_back(l, val)          \
    mdv_list_push_back_ptr(l, &(val), sizeof(val))


/**
 * @brief Check is list empty
 *
 * @param l [in]    list
 *
 * @return true if list empty
 * @return false if list contains values
 */
bool mdv_list_empty(mdv_list *l);


/**
 * @brief Append new entry to the back of list
 *
 * @param l [in]      list
 * @param entry [in]  new entry
 *
 */
void mdv_list_emplace_back(mdv_list *l, mdv_list_entry_base *entry);


/**
 * @brief Clear list
 *
 * @param l [in]    list
 */
void mdv_list_clear(mdv_list *l);


/**
 * @brief list entries iterator
 *
 * @param l [in]        list
 * @param type [in]     list item type
 * @param entry [out]   list entry
 */
#define mdv_list_foreach(l, type, entry)                                                                            \
    for(type *entry = (l)->next ? (type *)((l)->next->data): 0;                                                     \
        entry;                                                                                                      \
        entry = ((mdv_list_entry_base*)((char*)entry - offsetof(mdv_list_entry_base, data)))->next                  \
                ? (type *)((mdv_list_entry_base*)((char*)entry - offsetof(mdv_list_entry_base, data)))->next->data  \
                : 0                                                                                                 \
        )


/**
 * @brief Remove entry from the list
 *
 * @param l [in]        list
 * @param entry [in]    list entry
 */
void mdv_list_remove(mdv_list *l, mdv_list_entry_base *entry);


/**
 * @brief Exclude entry from the list without deallocation
 *
 * @param l [in]        list
 * @param entry [in]    list entry
 */
void mdv_list_exclude(mdv_list *l, mdv_list_entry_base *entry);


/**
 * @brief Remove last entry from the list
 *
 * @param l [in]        list
 */
void mdv_list_pop_back(mdv_list *l);


/**
 * @brief Returns reference to the last element in the list.
 */
#define mdv_list_back(l, type) ((l)->last ? (type*)(l)->last->data : 0)
