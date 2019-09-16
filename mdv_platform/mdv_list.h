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
#include <stddef.h>


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


/// @cond Doxygen_Suppress
mdv_list_entry_base * _mdv_list_push_back(mdv_list *l, void const *val, size_t size);
void                  _mdv_list_emplace_back(mdv_list *l, mdv_list_entry_base *entry);
void                  _mdv_list_clear(mdv_list *l);
void                  _mdv_list_exclude(mdv_list *l, mdv_list_entry_base *entry);
void                  _mdv_list_remove(mdv_list *l, mdv_list_entry_base *entry);
void                  _mdv_list_pop_back(mdv_list *l);
/// @endcond


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
    _mdv_list_push_back(&(l), &(val), sizeof(val))


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
#define mdv_list_push_back_ptr(l, val_ptr, size)    \
    _mdv_list_push_back(&(l), val_ptr, size)


/**
 * @brief Check is list empty
 *
 * @param l [in]    list
 *
 * @return 1 if list empty
 * @return 1 if list contains values
 */
#define mdv_list_empty(l) ((l).next == 0)


/**
 * @brief Append new entry to the back of list
 *
 * @param l [in]      list
 * @param entry [in]  new entry
 *
 */
#define mdv_list_emplace_back(l, entry)     \
    _mdv_list_emplace_back(&(l), entry)


/**
 * @brief Clear list
 *
 * @param l [in]    list
 */
#define mdv_list_clear(l)                   \
    _mdv_list_clear(&(l))


/**
 * @brief list entries iterator
 *
 * @param l [in]        list
 * @param type [in]     list item type
 * @param entry [out]   list entry
 */
#define mdv_list_foreach(l, type, entry)                                                                            \
    for(type *entry = (l).next ? (type *)((l).next->data): 0;                                                       \
        entry;                                                                                                      \
        entry = ((mdv_list_entry_base*)((void*)entry - offsetof(mdv_list_entry_base, data)))->next                  \
                ? (type *)((mdv_list_entry_base*)((void*)entry - offsetof(mdv_list_entry_base, data)))->next->data  \
                : 0                                                                                                 \
        )


/**
 * @brief Remove entry from the list
 *
 * @param l [in]        list
 * @param entry [in]    list entry
 */
#define mdv_list_remove(l, entry)           \
    _mdv_list_remove(&(l), (mdv_list_entry_base *)(entry))


/**
 * @brief Exclude entry from the list without deallocation
 *
 * @param l [in]        list
 * @param entry [in]    list entry
 */
#define mdv_list_exclude(l, entry)           \
    _mdv_list_exclude(&(l), (mdv_list_entry_base *)(entry))


/**
 * @brief Remove last entry from the list
 *
 * @param l [in]        list
 */
#define mdv_list_pop_back(l)                \
    _mdv_list_pop_back(&(l))
