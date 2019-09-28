#pragma once
#include <mdv_jobber.h>
#include <mdv_threads.h>
#include <mdv_eventfd.h>
#include <mdv_tracker.h>
#include <mdv_errno.h>
#include <stdatomic.h>
#include "storage/mdv_tablespace.h"


/// Data committer
typedef struct
{
    mdv_tablespace *tablespace;     ///< DB tables space
    mdv_tracker    *tracker;        ///< Nodes and network topology tracker
    mdv_jobber     *jobber;         ///< Jobs scheduler
    mdv_descriptor  start;          ///< Signal for starting
    mdv_thread      thread;         ///< Committer thread
    atomic_bool     active;         ///< Status
    atomic_size_t   active_jobs;    ///< Active jobs counter
} mdv_committer;


/**
 * @brief Create data committer
 *
 * @param committer [out]   Data committer for initialization
 * @param tablespace [in]   Storage
 * @param tracker [in]      Nodes and network topology tracker
 * @param jobber [in]       Jobs scheduler
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_committer_create(mdv_committer *committer,
                               mdv_tablespace *tablespace,
                               mdv_tracker *tracker,
                               mdv_jobber *jobber);


/**
 * @brief Stop data committer
 *
 * @param committer [in] Data committer
 */
void mdv_committer_stop(mdv_committer *committer);


/**
 * @brief Free data committer created by mdv_committer_create()
 *
 * @param committer [in] Data committer for freeing
 */
void mdv_committer_free(mdv_committer *committer);


/**
 * @brief Start data committer
 *
 * @param committer [in] Data committer
 *
 */
void mdv_committer_start(mdv_committer *committer);
