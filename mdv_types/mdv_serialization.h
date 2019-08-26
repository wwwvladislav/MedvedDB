#pragma once
#include "mdv_types.h"
#include <mdv_binn.h>
#include <mdv_topology.h>
#include <stdbool.h>


bool             mdv_binn_table(mdv_table_base const *table, binn *obj);
mdv_table_base * mdv_unbinn_table(binn const *obj);
bool             mdv_binn_row(mdv_field const *fields, mdv_row_base const *row, binn *list);
mdv_row_base *   mdv_unbinn_row(binn const *obj, mdv_field const *fields);


/**
 * @brief Serialize network topology
 *
 * @param topology [in] network topology
 * @param obj [out]     serialized topology
 *
 * @return On success returns true.
 * @return On error returns false.
 */
bool mdv_topology_serialize(mdv_topology const *topology, binn *obj);


/**
 * @brief Deserialize network topology
 *
 * @param obj [in]       serialized topology
 * @param topology [out] network topology
 *
 * @return On success returns true.
 * @return On error returns false.
 */
bool mdv_topology_deserialize(binn const *obj, mdv_topology *topology);
