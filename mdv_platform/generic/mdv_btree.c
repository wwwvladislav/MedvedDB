#include "mdv_btree.h"
#include "mdv_alloc.h"
#include <mdv_log.h>
#include <stdatomic.h>


struct mdv_btree
{
    atomic_uint_fast32_t rc;                                ///< References counter
    size_t               size;                              ///< Items number stored in btree
    uint32_t             order;                             ///< Btree order
    uint32_t             item_size;                         ///< Item size
    uint32_t             key_offset;                        ///< key offset inside btree value
    mdv_cmp_fn           key_cmp_fn;                        ///< Keys comparison function
};
