#pragma once
#include <stdint.h>
#include <mdv_uuid.h>


#define MDV_STRG_METAINF                "metainf.mdb"
#define MDV_STRG_METAINF_MAPS           3
#define MDV_MAP_METAINF                 "METAINF"           /// Common information about the DB
#define MDV_MAP_NODES                   "NODES"             /// Known nodes list
#define MDV_MAP_APPLIED                 "APPLIED"           /// Transaction logs applied positions


#define MDV_STRG_OBJECTS                "objects.mdb"
#define MDV_STRG_OBJECTS_MAPS           2
#define MDV_MAP_OBJECTS                 "OBJECTS"           /// DB objects: tables, rows, etc
#define MDV_MAP_REMOVED                 "REMOVED"           /// Removed objects identifiers
/*
    ObjectID - NodeID + TRLogId (4 bytes + 8 bytes)
*/

#define MDV_STRG_TRANSACTION_LOG        "trlog.mdb"
#define MDV_STRG_TRANSACTION_LOG_MAPS(N)(2 + N)
#define MDV_MAP_REMOVED                 "REMOVED"


char const *MDV_STRG_TRLOG(mdv_uuid const *uuid);
#define MDV_STRG_TRLOG_MAPS             1
#define MDV_MAP_TRLOG                   "TRLOG"             /// Transaction log


char const *MDV_MAP_TRANSACTION_LOG(uint32_t node_id);
