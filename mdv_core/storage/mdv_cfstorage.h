// Conflict-free Replicated Storage

#pragma once
#include "mdv_storage.h"
#include <mdv_types.h>
#include <mdv_uuid.h>
#include <mdv_list.h>


typedef struct mdv_cfstorage mdv_cfstorage;


/// DB operation
typedef struct
{
    uint32_t op;                ///< operation id
    uint32_t alignment;         ///< alignment (unused)
    uint8_t  payload[1];        ///< operation payload
} mdv_op;


typedef struct
{
    uint64_t    id;
    struct
    {
        uint32_t size;
        mdv_op  *ptr;
    } op;
} mdv_cfstorage_op;


typedef bool (*mdv_cfstorage_sync_fn)(void *arg,
                                      uint32_t peer_src,
                                      uint32_t peer_dst,
                                      size_t count,
                                      mdv_list const *ops);

typedef bool (*mdv_cfstorage_apply_fn)(void *arg,
                                       mdv_cfstorage_op const *op);

mdv_cfstorage * mdv_cfstorage_open(mdv_uuid const *uuid, uint32_t nodes_num);

bool            mdv_cfstorage_drop(mdv_uuid const *uuid);

void            mdv_cfstorage_close(mdv_cfstorage *cfstorage);

uint64_t        mdv_cfstorage_new_id(mdv_cfstorage *cfstorage);

mdv_uuid const* mdv_cfstorage_uuid(mdv_cfstorage *cfstorage);

bool            mdv_cfstorage_log_add(mdv_cfstorage *cfstorage,
                                      uint32_t peer_id,
                                      mdv_list const *ops); // list<mdv_cfstorage_op>

uint64_t        mdv_cfstorage_sync(mdv_cfstorage *cfstorage,
                                   uint64_t sync_pos,
                                   uint32_t peer_src,
                                   uint32_t peer_dst,
                                   void *arg,
                                   mdv_cfstorage_sync_fn fn);

bool            mdv_cfstorage_log_apply(mdv_cfstorage *cfstorage,
                                        uint32_t peer_id,
                                        void *arg,
                                        mdv_cfstorage_apply_fn fn);

bool            mdv_cfstorage_log_changed(mdv_cfstorage *cfstorage, uint32_t peer_id);

uint64_t        mdv_cfstorage_log_last_id(mdv_cfstorage *cfstorage, uint32_t peer_id);
