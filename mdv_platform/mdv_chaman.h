/**
 * @file
 * @brief Channels manager
 * @details Channels manager (chaman) is used for incoming and outgoing peers connections handling.
  */
#pragma once
#include <stddef.h>
#include "mdv_errno.h"
#include "mdv_string.h"


/// Channels manager descriptor
typedef struct mdv_chaman mdv_chaman;


/**
 * @brief Create new channels manager
 *
 * @param tp_size [in] threads count in thread pool
 *
 * @return On success, return pointer to a new created channels manager
 * @return On error, return NULL
 */
mdv_chaman * mdv_chaman_create(size_t tp_size);


/**
 * @brief Stop and free channels manager
 *
 * @param chaman [in] channels manager
 */
void mdv_chaman_free(mdv_chaman *chaman);


/**
 * @brief Register new listener
 *
 * @param chaman [in] channels manager
 * @param addr [in] address for listening in following format protocol://host:port.
 */
mdv_errno mdv_chaman_listen(mdv_chaman *chaman, mdv_string const addr);


// mdv_errno mdv_chaman_connect(mdv_chaman *chaman, char const *addr);

