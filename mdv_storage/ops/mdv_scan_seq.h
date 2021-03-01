/**
 * @file mdv_scan_seq.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Scaner for entire relation (table) stored on disk
 * @version 0.1
 * @date 2021-23-02
 *
 * @copyright Copyright (c) 2021, Vladislav Volkov
 *
 */
#pragma once
#include <ops/mdv_op.h>
#include <mdv_2pset.h>


/**
 * @brief Create new objects scanner
 *
 * @param objects [in] Objects storage
 *
 * @return new table scanner
 */
mdv_op * mdv_scan_seq(mdv_2pset *objects);
