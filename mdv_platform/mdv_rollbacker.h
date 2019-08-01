/**
 * @file
 * @brief Macros for operations sequence  rollback
 */

#pragma once
#include "mdv_stack.h"
#include "mdv_log.h"


/// Rollback function type with 1 argument
typedef void(*mdv_rollback_fn_1)(void *);


/// Rollback function type with 2 arguments
typedef void(*mdv_rollback_fn_2)(void *, void *);


/// Rollback function type with 3 arguments
typedef void(*mdv_rollback_fn_3)(void *, void *, void *);


/// Rollback operation
typedef struct
{
    void (*rollback)();     ///< Rollback function
    size_t  args_count;     ///< Number of rollback function arguments
    void *  arg[3];         ///< Rollback function arguments
} mdv_rollback_op;


/// Rollbacker declaration macro
#define mdv_rollbacker(sz)                      \
    mdv_stack(mdv_rollback_op, sz)


/// Rollbacker initialization macro
#define mdv_rollbacker_clear(rbr)               \
    mdv_stack_clear(rbr)


/// Selector for rollbacker function
#define mdv_rollbacker_push_N(rbr, fn, _1, _2, _3, N, ...) N


/// Remember rollback function with 1 argument
#define mdv_rollbacker_push_1(rbr, fn, arg1)                                \
    {                                                                       \
        mdv_rollback_op op_##fn = { (mdv_rollback_fn_1)&fn, 1, { arg1 } };  \
        if (!mdv_stack_push(rbr, op_##fn))                                  \
            MDV_LOGE("Rollback stack is full");                             \
    }


/// Remember rollback function with 2 arguments
#define mdv_rollbacker_push_2(rbr, fn, arg1, arg2)                                  \
    {                                                                               \
        mdv_rollback_op op_##fn = { (mdv_rollback_fn_2)&fn, 2, { arg1, arg2 } };    \
        if (!mdv_stack_push(rbr, op_##fn))                                          \
            MDV_LOGE("Rollback stack is full");                                     \
    }


/// Remember rollback function with 3 arguments
#define mdv_rollbacker_push_3(rbr, fn, arg1, arg2, arg3)                                \
    {                                                                                   \
        mdv_rollback_op op_##fn = { (mdv_rollback_fn_3)&fn, 3, { arg1, arg2, arg3 } };  \
        if (!mdv_stack_push(rbr, op_##fn))                                              \
            MDV_LOGE("Rollback stack is full");                                         \
    }


/// Remember rollback function
#define mdv_rollbacker_push(rbr, fn, ...)                           \
    mdv_rollbacker_push_N(rbr, fn, __VA_ARGS__,                     \
            mdv_rollbacker_push_3(rbr, fn, __VA_ARGS__),            \
            mdv_rollbacker_push_2(rbr, fn, __VA_ARGS__),            \
            mdv_rollbacker_push_1(rbr, fn, __VA_ARGS__),            \
            mdv_rollbacker_push_0(rbr, fn, __VA_ARGS__))            \


/// Execute all remembered functions in reverse order
#define mdv_rollback(rbr)                                           \
    mdv_stack_foreach(rbr, mdv_rollback_op, op)                     \
    {                                                               \
        switch(op->args_count)                                      \
        {                                                           \
            case 1:                                                 \
                ((mdv_rollback_fn_1)op->rollback)(op->arg[0]);      \
                break;                                              \
            case 2:                                                 \
                ((mdv_rollback_fn_2)op->rollback)(op->arg[0],       \
                                                  op->arg[1]);      \
                break;                                              \
            case 3:                                                 \
                ((mdv_rollback_fn_3)op->rollback)(op->arg[0],       \
                                                  op->arg[1],       \
                                                  op->arg[2]);      \
                break;                                              \
        }                                                           \
    }
