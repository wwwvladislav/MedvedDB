#pragma once
#include <mdv_table.h>
#include <mdv_rowset.h>
#include <mdv_limits.h>
#include <stdio.h>


extern char result_file_path[MDV_PATH_MAX];


int MDV_OUT(const char *format, ...);
int MDV_INF(const char *format, ...);


void mdv_cout_table(mdv_rowset *rowset);
