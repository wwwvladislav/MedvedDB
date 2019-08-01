#include "mdv_jobber.h"
#include "mdv_queuefd.h"


enum
{
    MDV_JOBBER_QUEUE_SIZE = 256             ///< Job queues size
};


typedef mdv_queuefd(int, MDV_JOBBER_QUEUE_SIZE) mdv_jobber_queue;


struct mdv_jobber
{
    mdv_threadpool     *threads;
    mdv_jobber_queue    jobs[1];
};


mdv_jobber * mdv_jobber_create(mdv_jobber_config const *config)
{}


void mdv_jobber_free(mdv_jobber *jobber)
{}
