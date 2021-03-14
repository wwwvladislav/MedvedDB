#pragma once
#include "mdv_enumerator.h"
#include "mdv_limits.h"
#include "mdv_def.h"


bool mdv_mkdir(char const *path);
bool mdv_rmdir(char const *path);
mdv_enumerator * mdv_dir_enumerator(char const *path);
