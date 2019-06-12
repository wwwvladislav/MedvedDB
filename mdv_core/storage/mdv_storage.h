#pragma once
#include <stdint.h>
#include <stdbool.h>


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


enum mdv_map_flags
{
    MDV_MAP_CREATE              = 1 << 0,   // Create the map if it doesn't exist
    MDV_MAP_MULTI               = 1 << 1,   // Duplicate keys may be used in the map
    MDV_MAP_INTEGERKEY          = 1 << 2,   // Keys are binary integers in native byte order
    MDV_MAP_FIXED_SIZE_VALUE    = 1 << 3,   // The data items for this map are all the same size. (used only with DB_MAP_MULTI)
    MDV_MAP_INTEGERVAL          = 1 << 4    // This option specifies that duplicate data items are binary integers
};


bool mdv_map_open(mdv_transaction *ptransaction, char const *name, uint32_t flags, mdv_map *pmap);
void mdv_map_close(mdv_map *pmap);





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
