#pragma once
#include <mdv_binn.h>
#include <mdv_types.h>
#include <stdbool.h>
#include <stdint.h>


struct mdv_client;


typedef struct mdv_client mdv_client;


/*
 * Create new client.
 */
mdv_client * mdv_client_create(char const *addr);


/*
 * Connect to DB.
 */
bool         mdv_client_connect(mdv_client *client);


/*
 * Disconnect from DB.
 */
void         mdv_client_disconnect(mdv_client *client);


/*
 * Return last error code for client.
 */
int          mdv_client_errno(mdv_client *client);


/*
 * Return client status message.
 */
char const * mdv_client_status_msg(mdv_client *client);


/*
 * Create new table.
 * Return:
 *  true - table was successfully created. The table->uuid contains new table UUID.
 *  false - table was not created.
 */
bool         mdv_create_table(mdv_client *client, mdv_table_base *table);

