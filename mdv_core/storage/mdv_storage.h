/**
 * @file
 * @brief API for key-value storage access.
 * @details This file contains main functions and type definitions to access the key-value storage.
*/

#pragma once
#include "mdv_map.h"
#include <mdv_types.h>
#include <mdv_def.h>


/// Key-value storage descriptor
typedef struct mdv_storage mdv_storage;


/// Flags for key-value storage opening. Flags might be combined with bitwise OR operator.
typedef enum
{
    MDV_STRG_FIXEDMAP           = 1 << 0,   ///< Use a fixed address for the mmap region.
    MDV_STRG_NOSUBDIR           = 1 << 1,   ///< Path is used as-is for the database main data file.
    MDV_STRG_RDONLY             = 1 << 2,   ///< Open the environment in read-only mode.
    MDV_STRG_WRITEMAP           = 1 << 3,   ///< Use a writeable memory map unless MDB_RDONLY is set.
    MDV_STRG_NOMETASYNC         = 1 << 4,   ///< Flush system buffers to disk only once per transaction, omit the metadata flush.
    MDV_STRG_NOSYNC             = 1 << 5,   ///< Don't flush system buffers to disk when committing a transaction.
    MDV_STRG_MAPASYNC           = 1 << 6,   ///< When using MDV_WRITEMAP, use asynchronous flushes to disk.
    MDV_STRG_NOTLS              = 1 << 7,   ///< Don't use Thread-Local Storage.
    MDV_STRG_NOLOCK             = 1 << 8,   ///< Don't do any locking.
    MDV_STRG_NORDAHEAD          = 1 << 9,   ///< Turn off readahead.
    MDV_STRG_NOMEMINIT          = 1 << 10   ///< Don't initialize malloc'd memory before writing to unused spaces in the data file.
} mdv_storage_flags;


/**
 * @brief Open new key-value storage
 *
 * @param path [in]     Directory where storage placed
 * @param name [in]     Storage name
 * @param dbs_num [in]  Maximum number of named databases
 * @param flags [in]    Flags for key-value storage opening (see mdv_storage_flags)
 * @param mapsize [in]  The size of the memory map (is also the maximum size of the database)
 *
 * @return On success, return nonzero pointer to opened storage
 * @return On error, return NULL pointer
 */
mdv_storage * mdv_storage_open(char const *path, char const *name, uint32_t dbs_num, uint32_t flags, size_t mapsize);


/**
 * @brief Add reference counter
 *
 * @param pstorage [in] storage
 *
 * @return pointer which provided as argument
 */
mdv_storage * mdv_storage_retain(mdv_storage *pstorage);


/**
 * @brief Decrement references counter. Storage is closed and memory is freed if references counter is zero.
 *
 * @param pstorage [in] storage
 *
 * @return references counter
 */
uint32_t mdv_storage_release(mdv_storage *pstorage);


/// Transaction descriptor
typedef struct
{
    mdv_storage *pstorage;      ///< storage
    void        *ptransaction;  ///< transaction
} mdv_transaction;


/**
 * @brief Start new transaction
 *
 * @param pstorage [in] storage opened with mdv_storage_open()
 *
 * @return On success return valid filled transaction descriptor. Validity can be checked with mdv_transaction_ok() macro.
 */
mdv_transaction mdv_transaction_start(mdv_storage *pstorage);


/**
 * @brief Commit transaction. After the successfully commit all data modifications are stored in DB.
 *
 * @param ptransaction [in] transaction started with mdv_transaction_start()
 *
 * @return On success returns true, otherwise returns false.
 */
bool mdv_transaction_commit(mdv_transaction *ptransaction);


/**
 * @brief Abort transaction. After the abort all data modifications are canceled.
 *
 * @param ptransaction [in] transaction started with mdv_transaction_start()
 *
 * @return On success returns true, otherwise returns false.
 */
bool mdv_transaction_abort(mdv_transaction *ptransaction);


/**
 * @brief Check the transaction validity
 *
 * @param t [in] transaction started with mdv_transaction_start()
 *
 * @return If transaction is valid returns true, otherwise returns false.
 */
#define mdv_transaction_ok(t) ((t).ptransaction != 0)


/// Key-value storage map descriptor
typedef struct
{
    mdv_storage *pstorage;  ///< Storage
    unsigned int dbmap;     ///< map internal descriptor
} mdv_map;


/// Flags for key-value storage map opening. Flags might be combined with bitwise OR operator.
typedef enum
{
    MDV_MAP_CREATE              = 1 << 0,   ///< Create the map if it doesn't exist
    MDV_MAP_MULTI               = 1 << 1,   ///< Duplicate keys may be used in the map
    MDV_MAP_INTEGERKEY          = 1 << 2,   ///< Keys are binary integers in native byte order
    MDV_MAP_FIXED_SIZE_VALUE    = 1 << 3,   ///< The data items for this map are all the same size. (used only with DB_MAP_MULTI)
    MDV_MAP_INTEGERVAL          = 1 << 4,   ///< This option specifies that duplicate data items are binary integers
    MDV_MAP_SILENT              = 1 << 5    ///< Don't output error messages to log
} mdv_map_flags;


mdv_map mdv_map_open       (mdv_transaction *ptransaction, char const *name, uint32_t flags);
void    mdv_map_close      (mdv_map *pmap);
bool    mdv_map_put        (mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value);
bool    mdv_map_put_unique (mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value);
bool    mdv_map_get        (mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data *value);
bool    mdv_map_del        (mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value);
#define mdv_map_ok(m)      ((m).pstorage != 0 && (m).dbmap != 0)


/**
 * @brief Cursor for key-value storage iteration.
 *
 * @details Within the transaction, cursor can be created with mdv_cursor_open().
 */
typedef struct
{
    mdv_storage *pstorage;      ///< pointer to the file or memory storage
    void        *pcursor;       ///< pointer to the cursor which points to the map entry
} mdv_cursor;


/// Flags for cursor opening.
typedef enum
{
    MDV_CURSOR_FIRST,                       ///< Position at first key/data item
    MDV_CURSOR_CURRENT,                     ///< Return key/data at current cursor position
    MDV_CURSOR_LAST,                        ///< Position at last key/data item
    MDV_CURSOR_LAST_DUP,                    ///< Position at last data item of current key. Only for MDB_DUPSORT
    MDV_CURSOR_NEXT,                        ///< Position at next data item
    MDV_CURSOR_NEXT_DUP,                    ///< Position at next data item of current key. Only for MDB_DUPSORT
    MDV_CURSOR_PREV,                        ///< Position at previous data item
    MDV_CURSOR_SET,                         ///< Position at specified key
    MDV_CURSOR_SET_RANGE                    ///< Position at first key greater than or equal to specified key.
} mdv_cursor_op;


/**
 * @brief Open new cursor within transaction.
 *
 * @param pmap [in]         Map which was created by the mdv_map_open().
 * @param ptransaction [in] Transaction
 *
 * @return New created cursor. Use mdv_cursor_ok() macro for the validation check.
 */
mdv_cursor  mdv_cursor_open             (mdv_map *pmap, mdv_transaction *ptransaction);
mdv_cursor  mdv_cursor_open_explicit    (mdv_map *pmap, mdv_transaction *ptransaction, mdv_data *key, mdv_data *value, mdv_cursor_op op);
mdv_cursor  mdv_cursor_open_first       (mdv_map *pmap, mdv_transaction *ptransaction, mdv_data *key, mdv_data *value);
bool        mdv_cursor_close            (mdv_cursor *pcursor);
bool        mdv_cursor_get              (mdv_cursor *pcursor, mdv_data *key, mdv_data *value, mdv_cursor_op op);


/**
 * @brief Check cursor validity
 *
 * @param c [in]    Cursor, created with mdv_cursor_open().
 *
 * Usage example:
 * @code
 * mdv_cursor cursor = {};
 *
 * if(mdv_cursor_ok(cursor))
 *     printf("Cursor is valid!");
 * else
 *     printf("Cursor isn't valid!");
 * @endcode
 */
#define     mdv_cursor_ok(c)            ((c).pstorage != 0 && (c).pcursor != 0)


typedef struct mdv_map_foreach_entry
{
    mdv_cursor      cursor;
    mdv_data        key;
    mdv_data        value;
} mdv_map_foreach_entry;


#define mdv_map_foreach_explicit(transaction, map, entry, op_first, op_next)    \
    entry.cursor = mdv_cursor_open_explicit(&map,                               \
                                    &transaction,                               \
                                    &entry.key,                                 \
                                    &entry.value,                               \
                                    op_first);                                  \
    for(; mdv_cursor_ok(entry.cursor);                                          \
        !mdv_cursor_get(&entry.cursor,                                          \
                        &entry.key,                                             \
                        &entry.value,                                           \
                        op_next)                                                \
            ? mdv_cursor_close(&entry.cursor)                                   \
            : false                                                             \
    )



/**
 * @brief Iterate all map entries
 *
 * @param transaction [in]  Transaction, started with mdv_transaction_start().
 * @param map [in]          Map, opened with mdv_map_open()
 * @param entry [out]       Item stored in map
 *
 * Usage example:
 * @code
 *   mdv_map_foreach(transaction, map, entry)
 *   {
 *       mdv_data const *key = &entry.key;
 *       mdv_data const *value = &entry.value;
 *   }
 * @endcode
 */
#define mdv_map_foreach(transaction, map, entry)            \
    mdv_map_foreach_entry entry;                            \
    mdv_map_foreach_explicit(transaction,                   \
                             map,                           \
                             entry,                         \
                             MDV_CURSOR_FIRST,              \
                             MDV_CURSOR_NEXT)


#define mdv_map_foreach_break(entry)                        \
{                                                           \
    mdv_cursor_close(&entry.cursor);                        \
    break;                                                  \
}


void mdv_map_read (mdv_map *pmap, mdv_transaction *ptransaction, void *map_fields);
void mdv_map_write(mdv_map *pmap, mdv_transaction *ptransaction, void *map_fields);
