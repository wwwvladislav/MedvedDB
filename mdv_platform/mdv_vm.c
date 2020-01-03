#include "mdv_vm.h"
#include "mdv_log.h"


mdv_errno mdv_vm_run(mdv_stack_base *stack, mdv_vm_fn const *fns, uint8_t const *program)
{
    mdv_errno err = MDV_OK;

    register uint8_t const *ip = program;

    do
    {
        switch(*ip)
        {
            case MDV_VM_NOP:
            {
                ++ip;
                break;
            }

            case MDV_VM_PUSH:
            {
                ++ip;
                uint16_t const len = *(uint16_t*)ip;
                ip += sizeof len;

                if(!mdv_stack_push_multiple(*stack, ip, len))
                {
                    ip = 0;
                    err = MDV_STACK_OVERFLOW;
                }

                ip += len;

                break;
            }

            case MDV_VM_END:
            {
                ip = 0;
                break;
            }

            case MDV_VM_CALL:
                // TODO

            default:
            {
                MDV_LOGE("Unknown instruction type");
                ip = 0;
                err = MDV_FAILED;
                break;
            }
        }
    }
    while(ip);

    return err;
}


mdv_errno mdv_vm_result_as_bool(mdv_stack_base *stack, bool *res)
{
    if (mdv_stack_size(*stack) < sizeof *res)
        return MDV_FAILED;

    char const *top = stack->data + stack->size;
    *res = *(bool*)(top - sizeof(bool));

    return MDV_OK;
}

