/**
 * @file mdv_dijkstra.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Dijkstra's algorithm implementation.
 * @details Dijkstra's algorithm is an algorithm for finding the shortest paths between nodes in a graph.
 * @version 0.1
 * @date 2019-09-11
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#pragma once
#include "mdv_hashmap.h"


typedef struct
{
    mdv_hashmap nodes;
} mdv_dijkstra;


/**
 * @brief Create new dijkstra solver
 * 
 * @param dijkstra 
 * @param nodes_count 
 * @return true 
 * @return false 
 */
bool mdv_dijkstra_create(mdv_dijkstra *dijkstra, size_t nodes_count);


void mdv_dijkstra_free(mdv_dijkstra *dijkstra);
