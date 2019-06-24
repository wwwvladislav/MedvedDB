#pragma once
#include "mdv_stack.h"
#include "mdv_log.h"


typedef void(*mdv_rollback_fn)(void *);


typedef struct
{
    mdv_rollback_fn rollback;
    void           *arg;
} mdv_rollback_op;


#define mdv_rollbacker(sz)                      \
    mdv_stack(mdv_rollback_op, sz)


#define mdv_rollbacker_clear(rbr)               \
    mdv_stack_clear(rbr)


#define mdv_rollbacker_push(rbr, fn, arg)                           \
    {                                                               \
        mdv_rollback_op op_##fn = { (mdv_rollback_fn)&fn, arg };    \
        if (!mdv_stack_push(rbr, op_##fn))                          \
            MDV_LOGE("Rollback stack is full");                     \
    }


#define mdv_rollback(rbr)                       \
    mdv_stack_foreach(rbr, mdv_rollback_op, op) \
        op->rollback(op->arg);

