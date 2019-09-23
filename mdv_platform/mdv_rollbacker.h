/**
 * @file
 * @brief Macros for operations sequence  rollback
 */

#pragma once
#include "mdv_stack.h"
#include "mdv_log.h"
#include "mdv_def.h"


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
typedef struct mdv_rollbacker mdv_rollbacker;


/**
 * @brief Creates new rollbacker given capacity
 *
 * @param capacity [in] rollbacker capacity
 */
mdv_rollbacker * mdv_rollbacker_create(size_t capacity);


/**
 * @brief Frees rollbacker crteated by mdv_rollbacker_create()
 */
void mdv_rollbacker_free(mdv_rollbacker *rbr);


/**
 * @brief Stores rollback operation
 */
bool mdv_rollbacker_push_(mdv_rollbacker *rbr, mdv_rollback_op const *op);


/// Selector for rollbacker function
#define mdv_rollbacker_push_N(rbr, fn, _1, _2, _3, N, ...) N


/// Remember rollback function with 1 argument
#define mdv_rollbacker_push_1(rbr, fn, arg1)                                                    \
    {                                                                                           \
        mdv_rollback_op const op_##fn = { (mdv_rollback_fn_1)&fn, 1, { arg1 } };                \
        mdv_rollbacker_push_(rbr, &op_##fn);                                                    \
    }


/// Remember rollback function with 2 arguments
#define mdv_rollbacker_push_2(rbr, fn, arg1, arg2)                                              \
    {                                                                                           \
        mdv_rollback_op const op_##fn = { (mdv_rollback_fn_2)&fn, 2, { arg1, arg2 } };          \
        mdv_rollbacker_push_(rbr, &op_##fn);                                                    \
    }


/// Remember rollback function with 3 arguments
#define mdv_rollbacker_push_3(rbr, fn, arg1, arg2, arg3)                                        \
    {                                                                                           \
        mdv_rollback_op const op_##fn = { (mdv_rollback_fn_3)&fn, 3, { arg1, arg2, arg3 } };    \
        mdv_rollbacker_push_(rbr, &op_##fn);                                                    \
    }


/// Remember rollback function
#define mdv_rollbacker_push(rbr, fn, ...)                           \
    mdv_rollbacker_push_N(rbr, fn, __VA_ARGS__,                     \
            mdv_rollbacker_push_3(rbr, fn, __VA_ARGS__),            \
            mdv_rollbacker_push_2(rbr, fn, __VA_ARGS__),            \
            mdv_rollbacker_push_1(rbr, fn, __VA_ARGS__),            \
            mdv_rollbacker_push_0(rbr, fn, __VA_ARGS__))            \


/// Execute all remembered functions in reverse order
void mdv_rollback(mdv_rollbacker *rbr);
