#pragma once


#define mdv_static_assert(cond) typedef char static_assertion_##__LINE__[(cond) ? 1: -1]
