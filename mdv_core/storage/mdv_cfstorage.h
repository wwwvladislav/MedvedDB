// Conflict-free Replicated Storage

#pragma once
#include "mdv_storage.h"
#include <mdv_types.h>
#include <mdv_uuid.h>


typedef struct mdv_cfstorage mdv_cfstorage;


typedef struct
{
    mdv_data key;
    mdv_data op;
} mdv_cfstorage_op;


typedef bool (*mdv_cfstorage_sync_fn)(void *arg, size_t count, mdv_cfstorage_op const *ops);


mdv_cfstorage * mdv_cfstorage_create(mdv_uuid const *uuid, uint32_t nodes_num);
mdv_cfstorage * mdv_cfstorage_open(mdv_uuid const *uuid, uint32_t nodes_num);
bool            mdv_cfstorage_drop(mdv_uuid const *uuid);
void            mdv_cfstorage_close(mdv_cfstorage *cfstorage);
bool            mdv_cfstorage_add(mdv_cfstorage *cfstorage, uint32_t peer_id, size_t count, mdv_cfstorage_op const *ops);
bool            mdv_cfstorage_rem(mdv_cfstorage *cfstorage, uint32_t peer_id, size_t count, mdv_cfstorage_op const *ops);
bool            mdv_cfstorage_sync(mdv_cfstorage *cfstorage, uint32_t peer_id, void *arg, mdv_cfstorage_sync_fn fn);
bool            mdv_cfstorage_log_apply(mdv_cfstorage *cfstorage);
