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


/// A piece of some data
typedef struct
{
    bool            external;       ///< Data pointer is external
    uint32_t        size;           ///< Data size
    void const     *data;           ///< Data pointer
} mdv_vm_datum;


typedef enum
{
    MDV_VM_NOP = 0,     /// NOP                                     [0]
    MDV_VM_PUSH,        /// PUSH ExtFlag Size Data (Size is 4 bytes)[1][x][xxxx][...]
    MDV_VM_CALL,        /// CALL FunctionId (FunctionId is 2 bytes) [2][xx]
    MDV_VM_END = 0xff   /// END                                     [0xFF]
} mdv_vm_commands;


typedef enum
{
    MDV_VM_TRUE = 0xFF,
    MDV_VM_FALSE = 0
} mdv_vm_bool;


/// VM function type
typedef mdv_errno(*mdv_vm_fn)(mdv_stack_base *);


/// VM functions
mdv_errno mdv_vmop_equal(mdv_stack_base *stack);
mdv_errno mdv_vmop_not_equal(mdv_stack_base *stack);
mdv_errno mdv_vmop_greater(mdv_stack_base *stack);
mdv_errno mdv_vmop_greater_or_equal(mdv_stack_base *stack);
mdv_errno mdv_vmop_less(mdv_stack_base *stack);
mdv_errno mdv_vmop_less_or_equal(mdv_stack_base *stack);
mdv_errno mdv_vmop_and(mdv_stack_base *stack);
mdv_errno mdv_vmop_or(mdv_stack_base *stack);
mdv_errno mdv_vmop_not(mdv_stack_base *stack);


/**
 * @brief Push data to VM stack
 *
 * @param stack [in]        VM stack
 * @param data [in]         Data to be pushed
 *
 * @return On success, returns MDV_OK
 * @return On error, returns non zero error code
 */
mdv_errno mdv_vm_stack_push(mdv_stack_base *stack, mdv_vm_datum const *data);


/**
 * @brief Pop data from VM stack
 *
 * @param stack [in]        VM stack
 * @param data [out]        Pointer to place popped data
 *
 * @return On success, returns MDV_OK
 * @return On error, returns non zero error code
 */
mdv_errno mdv_vm_stack_pop(mdv_stack_base *stack, mdv_vm_datum *data);


/**
 * @brief VM program interpretation
 *
 * @param stack [in]        VM stack
 * @param fns [in]          Custom commands handlers
 * @param fns_count [in]    Custom commands handlers count
 * @param program [in]      Commands sequence
 *
 * @return On success, returns MDV_OK
 * @return On error, returns non zero error code
 */
mdv_errno mdv_vm_run(mdv_stack_base *stack, mdv_vm_fn const *fns, size_t fns_count, uint8_t const *program);


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

