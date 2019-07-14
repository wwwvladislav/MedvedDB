#include "mdv_msg_dispatcher.h"
#include "mdv_condvar.h"
#include "mdv_mutex.h"
#include "mdv_alloc.h"
#include "mdv_log.h"


/// Request state
typedef struct
{
    mdv_condvar    *cv;             ///< Condition variable is used by client for response waiting
    uint16_t        request_id;     ///< Request identifier
    uint8_t         busy;           ///< Request was sent and client waits response on eventfd
    uint8_t         is_ready;       ///< Response is ready
} mdv_request_state;


/// Messages dispatcher
struct mdv_msg_dispatcher
{
    mdv_descriptor      fd;         ///< File descriptor
    mdv_mutex          *mutex;      ///< Mutex for requests list guarding
    size_t              size;       ///< Limit for the maximum number of requests that require an answer
    mdv_request_state   reqs[1];    ///< Requests list
};


mdv_msg_dispatcher * mdv_msg_dispatcher_create(size_t size)
{
    mdv_msg_dispatcher *pd = (mdv_msg_dispatcher *)mdv_alloc(offsetof(mdv_msg_dispatcher, reqs) + size * sizeof(mdv_request_state));

    if (!pd)
    {
        MDV_LOGE("No memory for messages dispatcher");
        return 0;
    }

    // TODO
}


void mdv_msg_dispatcher_free(mdv_msg_dispatcher *pd)
{}
