#include "mdv_predicate.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <mdv_rollbacker.h>
#include <stdatomic.h>


struct mdv_predicate
{
    atomic_uint_fast32_t    rc;             ///< References counter
    mdv_vector             *expr;           ///< Compiled VM expression
    mdv_vm_fn const        *fns;            ///< Custom function handlers
    size_t                  fns_count;      ///< Custom function handlers count
};


static const uint8_t MDV_EMPTY_PREDICATE[] =
{
    MDV_VM_PUSH, 0, 1, 0, 0, 0, MDV_VM_TRUE,
    MDV_VM_END
};


static mdv_vm_fn const MDV_PREDICATE_FNS[] =
{
    mdv_vmop_equal,
    mdv_vmop_not_equal,
    mdv_vmop_greater,
    mdv_vmop_greater_or_equal,
    mdv_vmop_less,
    mdv_vmop_less_or_equal,
    mdv_vmop_and,
    mdv_vmop_or,
    mdv_vmop_not
};


mdv_predicate * mdv_predicate_parse(char const *expression)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    mdv_predicate *predicate = mdv_alloc(sizeof(mdv_predicate), "predicate");

    if (!predicate)
    {
        MDV_LOGE("No memory for predicate");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, predicate, "predicate");

    atomic_init(&predicate->rc, 1);

    predicate->fns = MDV_PREDICATE_FNS;
    predicate->fns_count = sizeof MDV_PREDICATE_FNS / sizeof *MDV_PREDICATE_FNS;

    predicate->expr = mdv_vector_create(sizeof MDV_EMPTY_PREDICATE, 1, &mdv_default_allocator);

    if (!predicate->expr)
    {
        MDV_LOGE("No memory for predicate");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, predicate->expr);

    if (expression && *expression)
        MDV_LOGW("Expression '%s' wasn't parsed. Default expression is used.", expression);

    mdv_vector_append(predicate->expr, MDV_EMPTY_PREDICATE, sizeof MDV_EMPTY_PREDICATE);

    mdv_rollbacker_free(rollbacker);

    return predicate;
}


mdv_predicate * mdv_predicate_retain(mdv_predicate *predicate)
{
    atomic_fetch_add_explicit(&predicate->rc, 1, memory_order_acquire);
    return predicate;
}


static void mdv_predicate_free(mdv_predicate *predicate)
{
    mdv_vector_release(predicate->expr);
    mdv_free(predicate, "predicate");
}


uint32_t mdv_predicate_release(mdv_predicate *predicate)
{
    uint32_t rc = 0;

    if (predicate)
    {
        rc = atomic_fetch_sub_explicit(&predicate->rc, 1, memory_order_release) - 1;

        if (!rc)
            mdv_predicate_free(predicate);
    }

    return rc;
}


uint8_t const * mdv_predicate_expr(mdv_predicate const *predicate)
{
    return mdv_vector_data(predicate->expr);
}


mdv_vm_fn const * mdv_predicate_fns(mdv_predicate const *predicate)
{
    return predicate->fns;
}


size_t mdv_predicate_fns_count(mdv_predicate const *predicate)
{
    return predicate->fns_count;
}
