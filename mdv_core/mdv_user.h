/**
 * @file
 * @brief Users connections processing logic.
 */
#pragma once
#include <mdv_def.h>
#include <mdv_chaman.h>
#include <mdv_msg.h>
#include <mdv_ebus.h>
#include <mdv_topology.h>


/**
 * @brief Creates user connection context
 *
 * @param fd [in]       channel descriptor
 * @param uuid [in]     current node uuid
 * @param session [in]  session identifier
 * @param ebus [in]     events bus
 * @param topology [in] current topology
 *
 * @return On success, return pointer to new user connection context
 * @return On error, return NULL pointer
 */
mdv_channel * mdv_user_create(mdv_descriptor  fd,
                              mdv_uuid const *uuid,
                              mdv_uuid const *session,
                              mdv_ebus       *ebus,
                              mdv_topology   *topology);
