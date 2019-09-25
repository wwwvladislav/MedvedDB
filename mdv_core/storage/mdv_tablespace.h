/**
 * @file
 * @brief Tablespace is a tables storage.
 * @details Tablespace contains table descriptions, row data and transaction logs.
*/
#pragma once
#include "mdv_cfstorage.h"
#include <mdv_types.h>
#include <mdv_errno.h>
#include <mdv_hashmap.h>


/// DB tables space
typedef struct
{
    mdv_hashmap cfstorages;     ///< storages map (UUID -> mdv_cfstorage)
} mdv_tablespace;


/**
 * @brief Open or create tablespace.
 *
 * @param tablespace [out] Pointer to a tablespace structure
 * @param nodes_num [in] Cluster nodes count
 *
 * @return MDV_OK if wait operation successfully completed
 * @return non zero value if error has occurred
 */
mdv_errno mdv_tablespace_open(mdv_tablespace *tablespace, uint32_t nodes_num);


/**
 * @brief Delete existing tablespace.
 *
 * @return MDV_OK if wait operation successfully completed
 * @return non zero value if error has occurred
 */
mdv_errno mdv_tablespace_drop();


/**
 * @brief Close opened tablespace.
 *
 * @param tablespace [in] Pointer to a tablespace structure
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
mdv_rowid const * mdv_tablespace_create_table(mdv_tablespace *tablespace, mdv_table_base const *table);

