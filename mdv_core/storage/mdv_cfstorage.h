#pragma once
#include "mdv_storage.h"
#include "mdv_key.h"
#include <mdv_uuid.h>
#include <mdv_binn.h>
#include <stdatomic.h>


typedef struct
{
    mdv_storage     *data;                  // data storage

    struct
    {
        struct
        {
            atomic_uint_fast64_t    top;    // last insertion point
            atomic_uint_fast64_t    pos;    // applied point
            mdv_storage            *set;    // add-set
        } log;                              // transaction log

        mdv_storage *rem;                   // remove-set
    } ops;
} mdv_cfstorage;


#define mdv_cfstorage_ok(storage) ((storage).data != 0)


typedef struct
{
    mdv_key_base const *key;
    size_t              size;
    void const         *op;
} mdv_cfstorage_op;


typedef bool (*mdv_cfstorage_log_handler)(mdv_cfstorage_op const *op, mdv_storage *data);


mdv_cfstorage mdv_cfstorage_create(mdv_uuid const *uuid);
mdv_cfstorage mdv_cfstorage_open(mdv_uuid const *uuid);
bool          mdv_cfstorage_drop(mdv_uuid const *uuid);
void          mdv_cfstorage_close(mdv_cfstorage *cfstorage);
bool          mdv_cfstorage_add(mdv_cfstorage *cfstorage, size_t count, mdv_cfstorage_op const **ops);
bool          mdv_cfstorage_rem(mdv_cfstorage *cfstorage, size_t count, mdv_cfstorage_op const **ops);
bool          mdv_cfstorage_is_key_deleted(mdv_cfstorage *cfstorage, mdv_key_base const *key);
bool          mdv_cfstorage_log_seek(mdv_cfstorage *cfstorage, uint64_t pos);
bool          mdv_cfstorage_log_tell(mdv_cfstorage *cfstorage, uint64_t *pos);
bool          mdv_cfstorage_log_last(mdv_cfstorage *cfstorage, uint64_t *pos);
bool          mdv_cfstorage_log_apply(mdv_cfstorage *cfstorage, mdv_cfstorage_log_handler handler);

