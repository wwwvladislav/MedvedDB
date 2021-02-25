#pragma once
#include <minunit.h>
#include <mdv_rollbacker.h>


static void test_rollbacker_1(int *arg1)
{
    *arg1 = 1;
}

static void test_rollbacker_2(int *arg1, int *arg2)
{
    *arg1 = 1;
    *arg2 = 2;
}

static void test_rollbacker_3(int *arg1, int *arg2, int *arg3)
{
    *arg1 = 1;
    *arg2 = 2;
    *arg3 = 3;
}


MU_TEST(platform_rollbacker)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    int args1[1] = {};
    int args2[2] = {};
    int args3[3] = {};

    mdv_rollbacker_push(rollbacker, test_rollbacker_1, args1);
    mdv_rollbacker_push(rollbacker, test_rollbacker_2, args2, args2 + 1);
    mdv_rollbacker_push(rollbacker, test_rollbacker_3, args3, args3 + 1, args3 + 2);


    mdv_rollback(rollbacker);

    mu_check(args1[0] == 1);
    mu_check(args2[0] == 1 && args2[1] == 2);
    mu_check(args3[0] == 1 && args3[1] == 2 && args3[2] == 3);
}
