#pragma once
#include <minunit.h>
#include <mdv_vm.h>


static void mdv_platform_vmop_equal()
{
    mdv_stack(char, 256) st;

    mdv_stack_base *stack = (mdv_stack_base *)&st;

    mdv_vm_datum a =
    {
        .external = false,
        .size = 4,
        .data = "1234"
    };

    mdv_vm_datum b =
    {
        .external = false,
        .size = 5,
        .data = "12345"
    };

    mdv_vm_datum c =
    {
        .external = false,
        .size = 0,
        .data = ""
    };

    bool res = false;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &b) == MDV_OK);
    mu_check(mdv_vmop_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vmop_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vmop_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    a.external = true;
    b.external = true;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &b) == MDV_OK);
    mu_check(mdv_vmop_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);
}


static void mdv_platform_vmop_not_equal()
{
    mdv_stack(char, 256) st;

    mdv_stack_base *stack = (mdv_stack_base *)&st;

    mdv_vm_datum a =
    {
        .external = false,
        .size = 4,
        .data = "1234"
    };

    mdv_vm_datum b =
    {
        .external = false,
        .size = 5,
        .data = "12345"
    };

    mdv_vm_datum c =
    {
        .external = false,
        .size = 0,
        .data = ""
    };

    bool res = false;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_not_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &b) == MDV_OK);
    mu_check(mdv_vmop_not_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vmop_not_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vmop_not_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    a.external = true;
    b.external = true;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &b) == MDV_OK);
    mu_check(mdv_vmop_not_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);
}


static void mdv_platform_vmop_greater()
{
    mdv_stack(char, 256) st;

    mdv_stack_base *stack = (mdv_stack_base *)&st;

    mdv_vm_datum a =
    {
        .external = false,
        .size = 4,
        .data = "1234"
    };

    mdv_vm_datum b =
    {
        .external = false,
        .size = 5,
        .data = "12345"
    };

    mdv_vm_datum c =
    {
        .external = false,
        .size = 0,
        .data = ""
    };

    bool res = false;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_greater(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &b) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_greater(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vmop_greater(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vmop_greater(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    a.external = true;
    b.external = true;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &b) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_greater(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);
}


static void mdv_platform_vmop_greater_or_equal()
{
    mdv_stack(char, 256) st;

    mdv_stack_base *stack = (mdv_stack_base *)&st;

    mdv_vm_datum a =
    {
        .external = false,
        .size = 4,
        .data = "1234"
    };

    mdv_vm_datum b =
    {
        .external = false,
        .size = 5,
        .data = "12345"
    };

    mdv_vm_datum c =
    {
        .external = false,
        .size = 0,
        .data = ""
    };

    bool res = false;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_greater_or_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &b) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_greater_or_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vmop_greater_or_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_greater_or_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    a.external = true;
    b.external = true;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &b) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_greater_or_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);
}


static void mdv_platform_vmop_less()
{
    mdv_stack(char, 256) st;

    mdv_stack_base *stack = (mdv_stack_base *)&st;

    mdv_vm_datum a =
    {
        .external = false,
        .size = 4,
        .data = "1234"
    };

    mdv_vm_datum b =
    {
        .external = false,
        .size = 5,
        .data = "12345"
    };

    mdv_vm_datum c =
    {
        .external = false,
        .size = 0,
        .data = ""
    };

    bool res = false;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_less(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &b) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_less(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vmop_less(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vmop_less(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    a.external = true;
    b.external = true;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &b) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_less(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);
}


static void mdv_platform_vmop_less_or_equal()
{
    mdv_stack(char, 256) st;

    mdv_stack_base *stack = (mdv_stack_base *)&st;

    mdv_vm_datum a =
    {
        .external = false,
        .size = 4,
        .data = "1234"
    };

    mdv_vm_datum b =
    {
        .external = false,
        .size = 5,
        .data = "12345"
    };

    mdv_vm_datum c =
    {
        .external = false,
        .size = 0,
        .data = ""
    };

    bool res = false;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_less_or_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &b) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_less_or_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vmop_less_or_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &c) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_less_or_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    a.external = true;
    b.external = true;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &b) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &a) == MDV_OK);
    mu_check(mdv_vmop_less_or_equal(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);
}


static void mdv_platform_vmop_and()
{
    mdv_stack(char, 256) st;

    mdv_stack_base *stack = (mdv_stack_base *)&st;

    uint8_t const a = MDV_VM_TRUE;
    uint8_t const b = MDV_VM_FALSE;

    mdv_vm_datum O =
    {
        .external = false,
        .size = sizeof b,
        .data = &b
    };

    mdv_vm_datum I =
    {
        .external = false,
        .size = sizeof a,
        .data = &a
    };

    bool res = false;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &O) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &O) == MDV_OK);
    mu_check(mdv_vmop_and(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &O) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &I) == MDV_OK);
    mu_check(mdv_vmop_and(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &I) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &O) == MDV_OK);
    mu_check(mdv_vmop_and(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &I) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &I) == MDV_OK);
    mu_check(mdv_vmop_and(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    O.external = true;
    I.external = true;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &O) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &I) == MDV_OK);
    mu_check(mdv_vmop_and(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);
}


static void mdv_platform_vmop_or()
{
    mdv_stack(char, 256) st;

    mdv_stack_base *stack = (mdv_stack_base *)&st;

    uint8_t const a = MDV_VM_TRUE;
    uint8_t const b = MDV_VM_FALSE;

    mdv_vm_datum O =
    {
        .external = false,
        .size = sizeof b,
        .data = &b
    };

    mdv_vm_datum I =
    {
        .external = false,
        .size = sizeof a,
        .data = &a
    };

    bool res = false;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &O) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &O) == MDV_OK);
    mu_check(mdv_vmop_or(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &O) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &I) == MDV_OK);
    mu_check(mdv_vmop_or(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &I) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &O) == MDV_OK);
    mu_check(mdv_vmop_or(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &I) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &I) == MDV_OK);
    mu_check(mdv_vmop_or(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);

    O.external = true;
    I.external = true;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &O) == MDV_OK);
    mu_check(mdv_vm_stack_push(stack, &I) == MDV_OK);
    mu_check(mdv_vmop_or(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);
}


static void mdv_platform_vmop_not()
{
    mdv_stack(char, 256) st;

    mdv_stack_base *stack = (mdv_stack_base *)&st;

    uint8_t b = MDV_VM_FALSE;

    mdv_vm_datum O =
    {
        .external = false,
        .size = sizeof b,
        .data = &b
    };
    bool res = false;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &O) == MDV_OK);
    mu_check(mdv_vmop_not(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);
    mu_check(mdv_vmop_not(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);

    O.external = true;

    mdv_stack_clear(st);
    mu_check(mdv_vm_stack_push(stack, &O) == MDV_OK);
    mu_check(mdv_vmop_not(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == true);
    mu_check(mdv_vmop_not(stack) == MDV_OK);
    mu_check(mdv_vm_result_as_bool(stack, &res) == MDV_OK && res == false);
}


MU_TEST(platform_vm)
{
    mdv_platform_vmop_equal();              // "a" == "b"
    mdv_platform_vmop_not_equal();          // "a" != "b"
    mdv_platform_vmop_greater();            // "a" > "b"
    mdv_platform_vmop_greater_or_equal();   // "a" >= "b"
    mdv_platform_vmop_less();               // "a" < "b"
    mdv_platform_vmop_less_or_equal();      // "a" <= "b"
    mdv_platform_vmop_and();                // a & b
    mdv_platform_vmop_or();                 // a | b
    mdv_platform_vmop_not();                // !a
}
