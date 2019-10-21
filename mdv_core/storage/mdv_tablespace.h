/**
 * @file
 * @brief Tablespace is a tables storage.
 * @details Tablespace contains table descriptions, row data and transaction logs.
*/
#pragma once
#include "mdv_trlog.h"
#include <mdv_types.h>
#include <mdv_errno.h>
#include <mdv_vector.h>
#include <mdv_uuid.h>


/// DB tables space
typedef struct mdv_tablespace mdv_tablespace;


/**
 * @brief Open or create tablespace.
 *
 * @param uuid [in]        Current node UUID
 *
 * @return pointer to a tablespace
 */
mdv_tablespace * mdv_tablespace_open(mdv_uuid const *uuid);


/**
 * @brief Close opened tablespace.
 *
 * @param tablespace [in] Pointer to a tablespace
 */
void mdv_tablespace_close(mdv_tablespace *tablespace);


/**
 * @brief Insert new record into the transaction log for new table creation.
 *
 * @param tablespace [in] Pointer to a tablespace structure
 * @param peer_id [in] Peer identifier which produced table
 * @param table [in] [out] Table description
 *
 * @return non zero table identifier pointer if operation successfully completed.
 */
mdv_objid const * mdv_tablespace_create_table(mdv_tablespace *tablespace, mdv_table_base const *table);


/**
 * @brief Return transaction logs uuids vector.
 *
 * @param tablespace [in] Pointer to a tablespace structure
 *
 * @return storages uuids vector.
 */
mdv_vector * mdv_tablespace_trlogs(mdv_tablespace *tablespace);


/**
 * @brief Applies a transaction log to the data storage.
 *
 * @param tablespace [in]   Pointer to a tablespace structure
 * @param storage [in]      Storage UUID
 * @param peer_id [in]      Peer identifier
 *
 * @return true if transaction log was successfully applied
 * @return false if operation failed
 */
bool mdv_tablespace_log_apply(mdv_tablespace *tablespace, mdv_uuid const *storage, uint32_t peer_id);
