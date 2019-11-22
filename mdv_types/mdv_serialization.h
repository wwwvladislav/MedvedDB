#pragma once
#include "mdv_types.h"
#include "mdv_table.h"
#include "mdv_rowset.h"
#include <mdv_binn.h>
#include <mdv_topology.h>
#include <stdbool.h>


bool             mdv_binn_table_desc(mdv_table_desc const *table, binn *obj);
mdv_table_desc * mdv_unbinn_table_desc(binn const *obj);

bool             mdv_binn_table(mdv_table const *table, binn *obj);
mdv_table      * mdv_unbinn_table(binn const *obj);

bool             mdv_binn_table_uuid(mdv_uuid const *uuid, binn *obj);
mdv_uuid const * mdv_unbinn_table_uuid(binn const *obj);

bool             mdv_binn_rowset(mdv_rowset *rowset, binn *list);
mdv_rowset *     mdv_unbinn_rowset(binn const *list, mdv_table *table);


/**
 * @brief Serialize network topology
 *
 * @param topology [in] network topology
 * @param obj [out]     serialized topology
 *
 * @return On success returns true.
 * @return On error returns false.
 */
bool mdv_topology_serialize(mdv_topology *topology, binn *obj);


/**
 * @brief Deserialize network topology
 *
 * @param obj [in]       serialized topology
 *
 * @return On success returns non NULL pointer to topology.
 * @return On error returns NULL.
 */
mdv_topology * mdv_topology_deserialize(binn const *obj);


/**
 * @brief UUID serialization
 */
bool mdv_binn_uuid(mdv_uuid const *uuid, binn *obj);


/**
 * @brief UUID deserialization
 */
bool mdv_unbinn_uuid(binn *obj, mdv_uuid *uuid);
