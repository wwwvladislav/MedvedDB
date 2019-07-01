/**
 * @file
 * @brief This header contains the API for event notifications management.
*/

#pragma once
#include "mdv_def.h"


mdv_descriptor mdv_eventfd(unsigned int initval);
void           mdv_eventfd_close(mdv_descriptor fd);
