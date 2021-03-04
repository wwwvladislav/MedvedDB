#pragma once
#include <minunit.h>
#include <ops/mdv_scan_seq.h>
#include <mdv_filesystem.h>
#include <stdio.h>
#include <string.h>


MU_TEST(op_scan_seq)
{
    mdv_rmdir("./test");

    char tmp[64];

    mdv_2pset *storage = mdv_2pset_open("./test", "op_scan_seq");

    for(int i = 0; i < 10; ++i)
    {
        mdv_data key =
        {
            .size = sizeof i,
            .ptr = &i
        };

        snprintf(tmp, sizeof tmp, "object-%u", i);

        mdv_data data =
        {
            .size = strlen(tmp) + 1,
            .ptr = tmp
        };

        mu_check(mdv_2pset_add(storage, &key, &data) == MDV_OK);
    }

    mdv_op *scanner = mdv_scan_seq(storage);
    mu_check(scanner);

    mdv_kvdata kvdata;

    for(int i = 0; i < 10; ++i)
    {
        mu_check(mdv_op_next(scanner, &kvdata) == MDV_OK);

        mu_check(kvdata.key.size == sizeof(int));
        mu_check(*(int*)kvdata.key.ptr == i);

        snprintf(tmp, sizeof tmp, "object-%u", i);

        mu_check(kvdata.value.size == strlen(tmp) + 1);
        mu_check(strcmp(kvdata.value.ptr, tmp) == 0);
    }

    mu_check(mdv_op_next(scanner, &kvdata) == MDV_FALSE);

    mdv_op_release(scanner);

    mdv_2pset_release(storage);

    mdv_rmdir("./test");
}
