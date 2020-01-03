#include "mdv_predicate.h"
#include <mdv_vm.h>
#include <mdv_log.h>


static const uint8_t MDV_EMPTY_PREDICATE[] =
{
    MDV_VM_PUSH, 1, 0, 1,
    MDV_VM_END
};


mdv_vector * mdv_predicate_parse(char const *expression)
{
    mdv_vector *program = mdv_vector_create(sizeof MDV_EMPTY_PREDICATE, 1, &mdv_default_allocator);

    if (program)
    {
        // TODO:
        MDV_LOGW("Expression '%s' wasn't parsed. Default expression is used.", expression);
        mdv_vector_append(program, MDV_EMPTY_PREDICATE, sizeof MDV_EMPTY_PREDICATE);
    }
    else
        MDV_LOGE("No memory for predicate");

    return program;
}
