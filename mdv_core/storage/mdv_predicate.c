#include "mdv_predicate.h"


mdv_predicate * mdv_predicate_retain(mdv_predicate *predicate)
{
    return predicate->retain(predicate);
}


uint32_t mdv_predicate_release(mdv_predicate *predicate)
{
    if (predicate)
        return predicate->release(predicate);
    return 0;
}
