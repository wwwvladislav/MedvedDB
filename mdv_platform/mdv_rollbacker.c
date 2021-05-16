#include "mdv_rollbacker.h"
#include "mdv_alloc.h"


struct mdv_rollbacker
{
    size_t          capacity;
    size_t          size;
    mdv_rollback_op ops[1];
};


mdv_rollbacker * mdv_rollbacker_create(size_t capacity)
{
    mdv_rollbacker *rbr = mdv_alloc(offsetof(mdv_rollbacker, ops) + sizeof(mdv_rollback_op) * (capacity + 1));

    if (rbr)
    {
        rbr->capacity = capacity + 1;
        rbr->size = 0;

        mdv_rollbacker_push(rbr, mdv_rollbacker_free, rbr);
    }

    return rbr;
}


void mdv_rollbacker_free(mdv_rollbacker *rbr)
{
    if (rbr)
        mdv_free(rbr);
}


bool _mdv_rollbacker_push(mdv_rollbacker *rbr, mdv_rollback_op const *op)
{
    if (!rbr)
        return false;

    if (rbr->size >= rbr->capacity)
    {
        MDV_LOGE("Rollback stack is full");
        return false;
    }

    rbr->ops[rbr->size++] = *op;

    return true;
}


void mdv_rollback(mdv_rollbacker *rbr)
{
    if(rbr)
    {
        size_t size = rbr->size;

        while(size)
        {
            size--;

            mdv_rollback_op *op = rbr->ops + size;

            switch(op->args_count)
            {
                case 1:
                    ((mdv_rollback_fn_1)op->rollback)(op->arg[0]);
                    break;
                case 2:
                    ((mdv_rollback_fn_2)op->rollback)(op->arg[0],
                                                    op->arg[1]);
                    break;
                case 3:
                    ((mdv_rollback_fn_3)op->rollback)(op->arg[0],
                                                    op->arg[1],
                                                    op->arg[2]);
                    break;
            }
        }
    }
}
