#pragma once
#include <mdv_string.h>


typedef struct
{
    mdv_string addr;
} mdv_node;


typedef struct
{
    mdv_node nodes[1];
} mdv_nodes;


