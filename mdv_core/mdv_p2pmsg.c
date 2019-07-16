#include "mdv_p2pmsg.h"


char const * mdv_p2p_msg_name(uint32_t id)
{
    switch(id)
    {
        case mdv_message_id(p2p_hello):     return "P2P HELLO";
    }
    return "P2P UNKOWN";
}
