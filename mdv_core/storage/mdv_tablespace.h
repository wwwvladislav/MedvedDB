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
#include <mdv_ebus.h>


/// DB tables space
typedef struct mdv_tablespace mdv_tablespace;


/**
 * @brief Open or create tablespace.
 *
 * @param uuid [in]        Current node UUID
 * @param ebus [in]        Events bus
 *
 * @return pointer to a tablespace
 */
mdv_tablespace * mdv_tablespace_open(mdv_uuid const *uuid, mdv_ebus *ebus);


/**
 * @brief Close opened tablespace.
 *
 * @param tablespace [in] Pointer to a tablespace
 */
void mdv_tablespace_close(mdv_tablespace *tablespace);


/**
 * @brief Applies a transaction log to the data storage.
 *
 * @param tablespace [in]   Pointer to a tablespace structure
 * @param storage [in]      Storage UUID
 *
 * @return true if transaction log was successfully applied
 * @return false if operation failed
 */
bool mdv_tablespace_log_apply(mdv_tablespace *tablespace, mdv_uuid const *storage);
