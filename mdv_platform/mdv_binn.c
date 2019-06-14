#include "mdv_binn.h"
#include "mdv_alloc.h"


void mdv_binn_set_allocator()
{
    binn_set_alloc_functions(&mdv_alloc, &mdv_realloc, &mdv_free);
}
