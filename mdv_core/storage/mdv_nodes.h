#pragma once
#include "mdv_storage.h"
#include <mdv_string.h>
#include <mdv_uuid.h>
#include <mdv_deque.h>
#include <stdbool.h>
#include <stdatomic.h>


typedef struct
{
    mdv_uuid   uuid;
    mdv_string addr;
    uint32_t   id;
} mdv_node;


typedef struct
{
    atomic_uint_fast32_t max_id;
    mdv_deque           *nodes;
    mdv_storage         *storage;
} mdv_nodes;


bool mdv_nodes_load(mdv_nodes *nodes, mdv_storage *storage);
void mdv_nodes_free(mdv_nodes *nodes);
bool mdv_nodes_add (mdv_nodes *nodes, size_t size, mdv_node const *new_nodes);
