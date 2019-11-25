#pragma once
#include <mdv_ebus.h>
#include <mdv_jobber.h>
#include <mdv_topology.h>


/// Data committer
typedef struct mdv_committer mdv_committer;


/**
 * @brief Creates data committer
 *
 * @param ebus [in]         Events bus
 * @param jconfig [in]      Jobs scheduler configuration
 * @param topology [in]     Network topology
 *
 * @return Data committer
 */
mdv_committer * mdv_committer_create(mdv_ebus *ebus, mdv_jobber_config const *jconfig, mdv_topology *topology);


/**
 * @brief Retains data committer.
 * @details Reference counter is increased by one.
 */
mdv_committer * mdv_committer_retain(mdv_committer *committer);


/**
 * @brief Releases data committer.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the data committer is stopped and freed.
 */
uint32_t mdv_committer_release(mdv_committer *committer);


/**
 * @brief Start data committer
 *
 * @param committer [in] Data committer
 *
 */
void mdv_committer_start(mdv_committer *committer);


/**
 * @brief Stop data committer
 *
 * @param committer [in] Data committer
 */
void mdv_committer_stop(mdv_committer *committer);
