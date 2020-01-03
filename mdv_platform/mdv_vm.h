/**
 * @file mdv_vm.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Stack machine implementation
 * @version 0.1
 * @date 2020-01-02
 * @copyright Copyright (c) 2020, Vladislav Volkov
 */
#pragma once
#include "mdv_stack.h"
#include "mdv_def.h"


/// VM operation type
typedef uint8_t mdv_vm_op_t;


typedef enum
{
    MDV_VM_NOP = 0,     /// NOP
    MDV_VM_PUSH,        /// PUSH Size Data (Size - 2 bytes)
    MDV_VM_CALL,        /// CALL FunctionId (FunctionId - 2 bytes)
    MDV_VM_END = 0xff   /// END
} mdv_vm_commands;


/// VM function type
typedef mdv_errno(*mdv_vm_fn)(mdv_stack_base *);


/**
 * @brief VM program interpretation
 *
 * @param stack [in]   VM stack
 * @param fns [in]     Custom commands handlers
 * @param program [in] Commands sequence
 *
 * @return On success, returns MDV_OK
 * @return On error, returns non zero error code
 */
mdv_errno mdv_vm_run(mdv_stack_base *stack, mdv_vm_fn const *fns, uint8_t const *program);


/**
 * @brief Returns value from the top of stack
 *
 * @param stack [in]   VM stack
 * @param res [out]    Place for result
 *
 * @return On success, returns MDV_OK
 * @return On error, returns non zero error code
 */
mdv_errno mdv_vm_result_as_bool(mdv_stack_base *stack, bool *res);

