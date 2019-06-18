#pragma once
#include "mdv_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


typedef struct mdv_storage mdv_storage;


typedef enum
{
    MDV_STRG_FIXEDMAP           = 1 << 0,   // Use a fixed address for the mmap region.
    MDV_STRG_NOSUBDIR           = 1 << 1,   // Path is used as-is for the database main data file.
    MDV_STRG_RDONLY             = 1 << 2,   // Open the environment in read-only mode.
    MDV_STRG_WRITEMAP           = 1 << 3,   // Use a writeable memory map unless MDB_RDONLY is set.
    MDV_STRG_NOMETASYNC         = 1 << 4,   // Flush system buffers to disk only once per transaction, omit the metadata flush.
    MDV_STRG_NOSYNC             = 1 << 5,   // Don't flush system buffers to disk when committing a transaction.
    MDV_STRG_MAPASYNC           = 1 << 6,   // When using MDB_WRITEMAP, use asynchronous flushes to disk.
    MDV_STRG_NOTLS              = 1 << 7,   // Don't use Thread-Local Storage.
    MDV_STRG_NOLOCK             = 1 << 8,   // Don't do any locking.
    MDV_STRG_NORDAHEAD          = 1 << 9,   // Turn off readahead.
    MDV_STRG_NOMEMINIT          = 1 << 10   // Don't initialize malloc'd memory before writing to unused spaces in the data file.
} mdv_storage_flags;


mdv_storage * mdv_storage_open      (char const *path, char const *name, uint32_t dbs_num, uint32_t flags);
mdv_storage * mdv_storage_retain    (mdv_storage *pstorage);
void          mdv_storage_release   (mdv_storage *pstorage);


typedef struct
{
    mdv_storage *pstorage;
    void        *ptransaction;
} mdv_transaction;


mdv_transaction mdv_transaction_start   (mdv_storage *pstorage);
bool            mdv_transaction_commit  (mdv_transaction *ptransaction);
bool            mdv_transaction_abort   (mdv_transaction *ptransaction);
#define         mdv_transaction_ok(t)   ((t).ptransaction != 0)


typedef struct
{
    mdv_storage *pstorage;
    unsigned int dbmap;
} mdv_map;


typedef enum
{
    MDV_MAP_CREATE              = 1 << 0,   // Create the map if it doesn't exist
    MDV_MAP_MULTI               = 1 << 1,   // Duplicate keys may be used in the map
    MDV_MAP_INTEGERKEY          = 1 << 2,   // Keys are binary integers in native byte order
    MDV_MAP_FIXED_SIZE_VALUE    = 1 << 3,   // The data items for this map are all the same size. (used only with DB_MAP_MULTI)
    MDV_MAP_INTEGERVAL          = 1 << 4    // This option specifies that duplicate data items are binary integers
} mdv_map_flags;


mdv_map mdv_map_open       (mdv_transaction *ptransaction, char const *name, uint32_t flags);
void    mdv_map_close      (mdv_map *pmap);
bool    mdv_map_put        (mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value);
bool    mdv_map_put_unique (mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value);
bool    mdv_map_get        (mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data *value);
bool    mdv_map_del        (mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value);
#define mdv_map_ok(m)      ((m).pstorage != 0 && (m).dbmap != 0)


typedef struct
{
    mdv_storage *pstorage;
    void        *pcursor;
} mdv_cursor;


typedef enum
{
    MDV_CURSOR_FIRST,                       // Position at first key/data item
    MDV_CURSOR_CURRENT,                     // Return key/data at current cursor position
    MDV_CURSOR_LAST,                        // Position at last key/data item
    MDV_CURSOR_LAST_DUP,                    // Position at last data item of current key. Only for MDB_DUPSORT
    MDV_CURSOR_NEXT,                        // Position at next data item
    MDV_CURSOR_NEXT_DUP,                    // Position at next data item of current key. Only for MDB_DUPSORT
    MDV_CURSOR_PREV,                        // Position at previous data item
    MDV_CURSOR_SET,                         // Position at specified key
    MDV_SET_RANGE                           // Position at first key greater than or equal to specified key.
} mdv_cursor_op;


mdv_cursor  mdv_cursor_open             (mdv_map *pmap, mdv_transaction *ptransaction);
mdv_cursor  mdv_cursor_open_explicit    (mdv_map *pmap, mdv_transaction *ptransaction, mdv_data *key, mdv_data *value, mdv_cursor_op op);
mdv_cursor  mdv_cursor_open_first       (mdv_map *pmap, mdv_transaction *ptransaction, mdv_data *key, mdv_data *value);
bool        mdv_cursor_close            (mdv_cursor *pcursor);
bool        mdv_cursor_get              (mdv_cursor *pcursor, mdv_data *key, mdv_data *value, mdv_cursor_op op);
#define     mdv_cursor_ok(c)            ((c).pstorage != 0 && (c).pcursor != 0)


#define mdv_map_foreach_explicit(transaction, map, entry, op_first, op_next)    \
    for(struct {                                                                \
            mdv_cursor      cursor;                                             \
            mdv_data        key;                                                \
            mdv_data        value;                                              \
        } entry = {                                                             \
            mdv_cursor_open_explicit(&map,                                      \
                                     &transaction,                              \
                                     &entry.key,                                \
                                     &entry.value,                              \
                                     op_first)                                  \
        };                                                                      \
        mdv_cursor_ok(entry.cursor);                                            \
        !mdv_cursor_get(&entry.cursor,                                          \
                        &entry.key,                                             \
                        &entry.value,                                           \
                        op_next)                                                \
            ? mdv_cursor_close(&entry.cursor)                                   \
            : false                                                             \
    )


#define mdv_map_foreach(transaction, map, entry)            \
    mdv_map_foreach_explicit(transaction,                   \
                             map,                           \
                             entry,                         \
                             MDV_CURSOR_FIRST,              \
                             MDV_CURSOR_NEXT)


#define mdv_map_foreach_break(entry)                        \
    mdv_cursor_close(&entry.cursor);                        \
    break;


void mdv_map_read (mdv_map *pmap, mdv_transaction *ptransaction, void *map_fields);
void mdv_map_write(mdv_map *pmap, mdv_transaction *ptransaction, void *map_fields);
