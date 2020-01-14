#pragma once
#include <mdv_table.h>
#include <mdv_rowset.h>


void mdv_cout_table(mdv_table const *table, mdv_enumerator *resultset);
void mdv_cout_table_header(mdv_table const *table);
void mdv_cout_table_separator(mdv_table const *table);
void mdv_cout_table_row(mdv_table const *table, mdv_row const *row);
