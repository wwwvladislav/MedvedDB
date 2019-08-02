#include "mdv_nodes.h"
#include <mdv_alloc.h>


typedef struct mdv_nodes_job_data
{
    mdv_storage *storage;
    mdv_node     node;
} mdv_nodes_job_data;


typedef mdv_job(mdv_nodes_job_data) mdv_nodes_store_job;


static void mdv_nodes_store_fn(mdv_job_base *job)
{
    mdv_nodes_job_data *data = (mdv_nodes_job_data *)job->data;
    mdv_nodes_store(data->storage, &data->node);
}


static void mdv_nodes_store_finalize(mdv_job_base *job)
{
    mdv_nodes_job_data *data = (mdv_nodes_job_data *)job->data;
    mdv_storage_release(data->storage);
    mdv_free(job, "nodes_store_job");
}


mdv_errno mdv_nodes_store_async(mdv_jobber *jobber, mdv_storage *storage, mdv_node const *node)
{
    mdv_nodes_store_job *job = mdv_alloc(offsetof(mdv_nodes_store_job, data) + offsetof(mdv_nodes_job_data, node) + node->size, "nodes_store_job");

    if (!job)
        return MDV_NO_MEM;

    job->fn         = mdv_nodes_store_fn;
    job->finalize   = mdv_nodes_store_finalize;
    job->data.storage   = mdv_storage_retain(storage);

    memcpy(&job->data.node, node, node->size);

    mdv_errno err = mdv_jobber_push(jobber, (mdv_job_base*)job);

    if (err != MDV_OK)
    {
        mdv_storage_release(job->data.storage);
        mdv_free(job, "nodes_store_job");
    }

    return err;
}

