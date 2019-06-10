#include "mdv_string.h"
#include "mdv_log.h"


mdv_string mdv_str_pdup(void *mpool, char const *str)
{
    size_t const size = strlen(str) + 1;
    void *ptr = mdv_palloc(mpool, size);

    if (!ptr)
    {
        mdv_string empty_str = mdv_str_null;
        return empty_str;
    }

    memcpy(ptr, str, size);

    mdv_string dup_str = { size, ptr };

    return dup_str;
}

mdv_string mdv_str_pcat(void *mpool, mdv_string const *dst, mdv_string const *str)
{
    if (mdv_str_is_empty(*str))
        return *dst;

    char const * mempool_ptr = (char const *)mdv_mempool_ptr(mpool);
    size_t const mempool_size = mdv_mempool_size(mpool);

    mdv_string empty_str = mdv_str_null;

    if (dst->ptr < mempool_ptr
        || dst->ptr + dst->size != mempool_ptr + mempool_size)
    {
        MDV_LOGE("str_pcat failed. Destination string must be on the top of the memory pool.");
        return empty_str;
    }

    void *ptr = mdv_palloc(mpool, str->size - 1);
    if (!ptr)
        return empty_str;

    memcpy(dst->ptr + dst->size - 1, str->ptr, str->size - 1);
    dst->ptr[dst->size + str->size - 2] = 0;

    return (mdv_string) { dst->size + str->size - 1, dst->ptr };
}

