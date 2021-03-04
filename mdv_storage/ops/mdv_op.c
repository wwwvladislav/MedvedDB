#include "mdv_op.h"


mdv_op * mdv_op_retain(mdv_op *op)
{
    return op->vptr->retain(op);
}


uint32_t mdv_op_release(mdv_op *op)
{
    return op ? op->vptr->release(op) : 0;
}


mdv_errno mdv_op_reset(mdv_op *op)
{
    return op->vptr->reset(op);
}


mdv_errno mdv_op_next(mdv_op *op, mdv_kvdata *kvdata)
{
    return op->vptr->next(op, kvdata);
}
