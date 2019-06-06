#include "mdv_string.h"

mdv_string_t mdv_str_pdup(void *mpool, char const *str)
{
    size_t const len = strlen(str) + 1;
    void *ptr = mdv_palloc(mpool, len);
    if (!ptr)
    {
        mdv_string_t empty_str = mdv_str_null;
        return empty_str;
    }
    memcpy(ptr, str, len);
    mdv_string_t dup_str = { len, ptr };
    return dup_str;
}
