/**
 * @file mdv_op.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Base type for all MDB operators
 * @version 0.1
 * @date 2021-23-02
 *
 * @copyright Copyright (c) 2021, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_def.h>
#include <mdv_data.h>


/// Base type for all MDB operators
typedef struct mdv_op mdv_op;


typedef mdv_op *           (*mdv_op_retain_fn)(mdv_op *);
typedef uint32_t           (*mdv_op_release_fn)(mdv_op *);
typedef mdv_errno          (*mdv_op_reset_fn)(mdv_op *);
typedef mdv_kvdata const * (*mdv_op_next_fn)(mdv_op *);


/// Interface for operator
typedef struct
{
    mdv_op_retain_fn        retain;     ///< Function for operator retain
    mdv_op_release_fn       release;    ///< Function for operator release
    mdv_op_reset_fn         reset;      ///< Function for operator reset
    mdv_op_next_fn          next;       ///< Function for next row accessing
} mdv_iop;


struct mdv_op
{
    mdv_iop const          *vptr;       ///< Interface for operator
};


mdv_op *           mdv_op_retain(mdv_op *op);
uint32_t           mdv_op_release(mdv_op *op);
mdv_errno          mdv_op_reset(mdv_op *op);
mdv_kvdata const * mdv_op_next(mdv_op *op);
