/**
 * @file
 * @brief DB Client
 * @details Client provides API for DB accessing.
 */
#pragma once
#include "mdv_client_config.h"
#include <mdv_table.h>
#include <mdv_rowset.h>
#include <mdv_topology.h>
#include <mdv_errno.h>
#include <mdv_bitset.h>
#include <mdv_systbls.h>


/// Client descriptor
typedef struct mdv_client mdv_client;


/**
 * @brief Client library initialization
 */
bool mdv_initialize();


/**
 * @brief Client library finalization
 */
void mdv_finalize();


/**
 * @brief Connect to DB
 *
 * @param config [in] client configuration
 *
 * @return On success, return nonzero client pointer
 * @return On error, return NULL pointer
 */
mdv_client * mdv_client_connect(mdv_client_config const *config);


/**
 * @brief Close connection and free client
 *
 * @param client [in] DB client
 */
void mdv_client_close(mdv_client *client);


/**
 * @brief Create new table
 *
 * @param client [in]   DB client
 * @param table  [in]   Table description
 *               [out]  Table identifier (table->id)
 *
 * @return On success, returns nonzero table descriptor.
 * @return On error, returns NULL
 */
mdv_table * mdv_create_table(mdv_client *client, mdv_table_desc *table);


/**
 * @brief Finds and returns table with given UUID
 *
 * @param client [in]   DB client
 * @param uuid [in]     Table identifier
 *
 * @return On success, returns nonzero table descriptor.
 * @return On error, returns NULL pointer
 */
mdv_table * mdv_get_table(mdv_client *client, mdv_uuid const *uuid);


/**
 * @brief Request network topology.
 * @details This function should be used only for diagnostic purposes.
 *
 * @param client [in]       DB client
 *
 * @return On success, returns nonzero topology.
 * @return On error, returns NULL pointer
 */
mdv_topology * mdv_get_topology(mdv_client *client);


/**
 * @brief Request network routes.
 * @details This function should be used only for diagnostic purposes. Use mdv_free() to free the allocated links array.
 *
 * @param client [in]       DB client
 * @param routes [out]      Routes
 *
 * @return On success, return routes table (hashmap<UUID>).
 * @return On error, return NULL pointer
 */
mdv_hashmap * mdv_get_routes(mdv_client *client);


/**
 * @brief Insert row to given table
 * @details This function implements insertion functionality.
 *
 * @param client [in]    DB client
 * @param rowset [in]    Set of rows for insert
 *
 * @return On success, return MDV_OK.
 * @return On error, return non zero value
 */
mdv_errno mdv_insert(mdv_client *client, mdv_rowset *rowset);


/**
 * @brief Creates table rows iterator
 *
 * @param client [in]           DB client
 * @param table [in]            table descriptor
 * @param fields [in]           fields mask for reading
 * @param filter [in]           predicate for rows filtering
 * @param result_table [out]    result table descriptor
 *
 * @return On success, return nonzero pointer to result set (mdv_rowset's set)
 * @return On error, return NULL pointer
 */
mdv_rowset * mdv_select(mdv_client *client,
                        mdv_table  *table,
                        mdv_bitset *fields,
                        char const *filter);
