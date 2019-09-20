#pragma once
#include "mdv_storage.h"


typedef struct mdv_idmap mdv_idmap;


mdv_idmap * mdv_idmap_open(mdv_storage *storage, char const *name, size_t size);
void mdv_idmap_free(mdv_idmap *idmap);

