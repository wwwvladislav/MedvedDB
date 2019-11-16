/**
 * @file mdv_trlog.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Transaction logs storage
 * @version 0.1
 * @date 2019-09-30
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_def.h>
#include <mdv_uuid.h>
#include <mdv_list.h>


/// Transaction logs storage
typedef struct mdv_trlog mdv_trlog;


/// DB operation
typedef struct
{
    uint32_t size;              ///< operation size
    uint32_t type;              ///< operation type
    uint8_t  payload[1];        ///< operation payload
} mdv_trlog_op;


/// Data writtent to transaction log
typedef struct
{
    uint64_t        id;         ///< row identifier
    mdv_trlog_op    op;         ///< DB operation
} mdv_trlog_data;


typedef bool (*mdv_trlog_apply_fn)(void *arg, mdv_trlog_op *op);


typedef mdv_list_entry(mdv_trlog_data) mdv_trlog_entry;


/**
 * @brief Opens or creates new transaction log storage
 *
 * @param dir [in]      directory for transaction logs
 * @param uuid [in]     TR log storage UUID
 *
 * @return transaction log storage
 */
mdv_trlog * mdv_trlog_open(char const *dir, mdv_uuid const *uuid);


/**
 * @brief Add reference counter
 *
 * @param trlog [in] Transaction logs storage
 *
 * @return pointer which provided as argument
 */
mdv_trlog * mdv_trlog_retain(mdv_trlog *trlog);


/**
 * @brief Decrement references counter. Storage is closed and memory is freed if references counter is zero.
 *
 * @param trlog [in]    Transaction logs storage
 *
 * @return references counter
 */
uint32_t mdv_trlog_release(mdv_trlog *trlog);


/**
 * @brief Writes data to the transaction log.
 *
 * @param trlog [in]            Transaction logs storage
 * @param ops [in]              list of the transaction log records
 *
 * @return true if data was successfully written
 * @return false if error was happened
 */
bool mdv_trlog_add(mdv_trlog *trlog,
                   mdv_list/*<mdv_trlog_data>*/ const *ops);


/**
 * @brief Writes data to the transaction log.
 * @details New identifier is generated for new record.
 *
 * @param trlog [in]            Transaction logs storage
 * @param op [in]               DB operation to be written to the transaction log
 *
 * @return true if data was successfully written
 * @return false if error was happened
 */
bool mdv_trlog_add_op(mdv_trlog *trlog,
                      mdv_trlog_op const *op);


/**
 * @brief Returns true if transaction log was changed
 *
 * @param trlog [in]
 */
bool mdv_trlog_changed(mdv_trlog *trlog);


/**
 * @brief Applies transaction log
 * @return number of applied rows
 */
uint32_t mdv_trlog_apply(mdv_trlog         *trlog,
                         uint32_t           batch_size,
                         void              *arg,
                         mdv_trlog_apply_fn fn);
