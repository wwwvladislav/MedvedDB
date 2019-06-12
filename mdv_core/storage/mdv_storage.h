#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


typedef struct mdv_storage mdv_storage;


mdv_storage * mdv_storage_open(char const *path, uint32_t dbs_num, uint32_t flags);
mdv_storage * mdv_storage_retain(mdv_storage *pstorage);
void          mdv_storage_release(mdv_storage *pstorage);


typedef struct
{
    mdv_storage *pstorage;
    void        *ptransaction;
} mdv_transaction;


bool mdv_transaction_start(mdv_storage *pstorage, mdv_transaction *ptransaction);
bool mdv_transaction_commit(mdv_transaction *ptransaction);
bool mdv_transaction_abort(mdv_transaction *ptransaction);


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


typedef struct
{
    size_t  size;
    void   *data;
} mdv_data;


bool mdv_map_open(mdv_transaction *ptransaction, char const *name, uint32_t flags, mdv_map *pmap);
void mdv_map_close(mdv_map *pmap);
bool mdv_map_put(mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value);
bool mdv_map_put_unique(mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value);
bool mdv_map_get(mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data *value);
bool mdv_map_del(mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value);


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
    MDV_CURSOR_SET                          // Position at specified key
} mdv_cursor_op;


bool mdv_cursor_open(mdv_map *pmap, mdv_transaction *ptransaction, mdv_cursor *pcursor);
void mdv_cursor_close(mdv_cursor *pcursor);
bool mdv_cursor_get(mdv_cursor *pcursor, mdv_data *key, mdv_data *value, mdv_cursor_op op);



typedef struct
{
    //MDB_env            *env;
} mdv_commit_log;


typedef struct
{
    struct
    {
        //MDB_env        *env;
    } data;

    struct
    {
        mdv_commit_log  add;
        mdv_commit_log  del;
    } log;
} mdv_table;
