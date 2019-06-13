#include "mdv_table.h"
#include "mdv_storages.h"


uint32_t mdv_field_size(mdv_field const *fld)
{
    switch(fld->type)
    {
        case MDV_FLD_TYPE_BOOL:     return 1u;
        case MDV_FLD_TYPE_CHAR:     return 1u;
        case MDV_FLD_TYPE_BYTE:     return 1u;
        case MDV_FLD_TYPE_INT8:     return 1u;
        case MDV_FLD_TYPE_UINT8:    return 1u;
        case MDV_FLD_TYPE_INT16:    return 2u;
        case MDV_FLD_TYPE_UINT16:   return 2u;
        case MDV_FLD_TYPE_INT32:    return 4u;
        case MDV_FLD_TYPE_UINT32:   return 4u;
        case MDV_FLD_TYPE_INT64:    return 8u;
        case MDV_FLD_TYPE_UINT64:   return 8u;
    }
}


bool mdv_table_create(mdv_storage *storage, mdv_table_base const *table)
{
    mdv_transaction transaction = mdv_transaction_start(storage);
    if (mdv_transaction_ok(transaction))
    {
        mdv_map map = mdv_map_open(&transaction, MDV_TBL_TABLES, MDV_MAP_CREATE);
        if (mdv_map_ok(map))
        {
            // TODO: serialize table description and save
        }
        mdv_transaction_commit(&transaction);
        mdv_map_close(&map);
    }

    return true;
}
