#pragma once
#include <stdint.h>
#include <stddef.h>
#include <mdv_uuid.h>


#define MDV_STRG_METAINF                "metainf.mdb"
#define MDV_STRG_METAINF_MAPS           2
#define MDV_MAP_METAINF                 "METAINF"           /// Common information about the DB
#define MDV_MAP_NODES                   "NODES"             /// Known nodes list


#define MDV_STRG_TABLES                 "tables.mdb"
#define MDV_STRG_OBJECTS_MAPS           3
#define MDV_MAP_OBJECTS                 "OBJECTS"           /// DB objects: tables, views, etc
#define MDV_MAP_REMOVED                 "REMOVED"           /// Removed objects identifiers
#define MDV_MAP_IDGEN                   "IDGEN"             /// Identifiers generator for objects


char const *MDV_STRG_UUID(mdv_uuid const *uuid, char *name, size_t size);
#define MDV_STRG_TRLOG_MAPS             2
#define MDV_MAP_TRLOG                   "TRLOG"             /// Transaction log
#define MDV_MAP_APPLIED                 "APPLIED"           /// Transaction logs applied position
