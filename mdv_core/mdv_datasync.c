#include "mdv_datasync.h"
#include <mdv_alloc.h>
#include <mdv_errno.h>
#include <mdv_log.h>


mdv_errno mdv_datasync_create(mdv_datasync *datasync, mdv_tablespace *tablespace)
{
    datasync->tablespace = tablespace;

    mdv_errno err = mdv_mutex_create(&datasync->mutex);

    if (err != MDV_OK)
    {
        MDV_LOGE("Mutex creation failed with error %d", err);
        return err;
    }

    if (!mdv_vector_create(datasync->routes, 64, mdv_default_allocator))
    {
        MDV_LOGE("No memorty for routes");
        mdv_mutex_free(&datasync->mutex);
        return MDV_NO_MEM;
    }

    return MDV_OK;
}


void mdv_datasync_free(mdv_datasync *datasync)
{
    mdv_vector_free(datasync->routes);
    mdv_mutex_free(&datasync->mutex);
}


mdv_errno mdv_datasync_update_routes(mdv_datasync *datasync, mdv_tracker *tracker)
{
    mdv_routes routes;

    if (!mdv_vector_create(routes, 64, mdv_default_allocator))
    {
        MDV_LOGE("No memorty for routes");
        return MDV_NO_MEM;
    }

    mdv_errno err = mdv_routes_find(&routes, tracker);

    if (err != MDV_OK)
    {
        mdv_vector_free(routes);
        return err;
    }

    if (mdv_mutex_lock(&datasync->mutex) == MDV_OK)
    {
        mdv_vector_free(datasync->routes);
        datasync->routes = routes;

        for(size_t i = 0; i < mdv_vector_size(routes); ++i)
        {
            MDV_LOGF(">>>>>>>>>>>>>>>>>>>>>: %u", routes.data[i]);
        }

        mdv_mutex_unlock(&datasync->mutex);
        return MDV_OK;
    }

    return MDV_FAILED;
}


bool mdv_datasync_do(mdv_datasync *datasync)
{
    // TODO:
    return false;
}
