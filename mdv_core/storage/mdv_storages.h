#pragma once
#include <stdint.h>


#define MDV_STRG_METAINF                "metainf.mdb"
#define MDV_STRG_METAINF_MAPS           3
#define MDV_MAP_METAINF                 "METAINF"
#define MDV_MAP_NODES                   "NODES"
#define MDV_MAP_LINKS                   "LINKS"


#define MDV_STRG_DATA                   "data.mdb"
#define MDV_STRG_DATA_MAPS              2
#define MDV_MAP_METAINF                 "METAINF"
#define MDV_MAP_DATA                    "DATA"


#define MDV_STRG_TRANSACTION_LOG        "transaction.log"
#define MDV_STRG_TRANSACTION_LOG_MAPS(N)(3 + N)
#define MDV_MAP_METAINF                 "METAINF"
#define MDV_MAP_ADDED                   "ADDED"
#define MDV_MAP_REMOVED                 "REMOVED"


char const *MDV_MAP_TRANSACTION_LOG(uint32_t node_id);
