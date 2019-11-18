#include "mdv_table.h"
#include <stdatomic.h>


struct mdv_table
{
    atomic_uint_fast32_t    rc;             ///< References counter
    mdv_uuid                id;             ///< Table unique identifier
    mdv_table_desc          desc;           ///< Table Description
};


mdv_table_desc * mdv_table_desc_clone(mdv_table_desc const *table)
{
    // TODO
    return 0;
}

