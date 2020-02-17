#pragma once
#include <mdv_messages.h>
#include <mdv_topology.h>
#include <mdv_list.h>
#include <mdv_hashmap.h>
#include "storage/mdv_trlog.h"


/*
    P1                                    P2
    |                                     |
    | HELLO >>>>>                         |   Handshake
    |                         <<<<< HELLO |
    |                                     |
    |                                     |
    | TOPOLOGY SYNC >>>>>                 |   Network topology synchronization. Send topology.
    |       <<<<< BROADCAST TOPOLOGY DIFF |
    |                                     |
    |                                     |
    | TRLOG SYNC >>>>>                    |   Transaction log synchronization request.
    |          <<<<< TRLOG STATE / STATUS |   Last transaction log record identifier.
    | CFSLOG DATA >>>>>                   |   Transaction log records.
    |          <<<<< TRLOG STATE / STATUS |
 */


mdv_message_def(p2p_hello, 1000 + 0,
    char const *listen;             ///< Listening address
);


mdv_message_def(p2p_status, 1000 + 1,
    int         err;                ///< Error code
    char const *message;            ///< Status message
);


mdv_message_def(p2p_linkstate, 1000 + 2,
    mdv_uuid    src;                ///< First peer unique identifier
    mdv_uuid    dst;                ///< Second peer unique identifier
    bool        connected;          ///< true, if first peer connected to second peer
);


mdv_message_def(p2p_toposync, 1000 + 3,
    mdv_topology *topology;         ///< Network topology
);


mdv_message_def(p2p_topodiff, 1000 + 4,
    mdv_topology *topology;         ///< Network topology
);


mdv_message_def(p2p_trlog_sync, 1000 + 5,
    mdv_uuid    trlog;               ///< Transaction log storage unique identifier
);


mdv_message_def(p2p_trlog_state, 1000 + 6,
    mdv_uuid    trlog;              ///< Transaction log storage unique identifier
    uint64_t    trlog_top;          ///< Last transaction log record identifier for requested CF Storage and peer.
);


mdv_message_def(p2p_trlog_data, 1000 + 7,
    mdv_uuid    trlog;              ///< Transaction log storage unique identifier
    uint32_t    count;              ///< log records count
    mdv_list    rows;               ///< transaction log data (list<mdv_trlog_data>)
);


mdv_message_def(p2p_broadcast, 1000 + 8,
    uint16_t     msg_id;            ///< Message identifier
    uint32_t     size;              ///< Data size for broadcasing
    void        *data;              ///< Data for broadcasing
    mdv_hashmap *notified;          ///< Notified nodes UUIDs
);


char const *    mdv_p2p_msg_name                        (uint32_t id);


bool            mdv_binn_p2p_hello                      (mdv_msg_p2p_hello const *msg, binn *obj);
bool            mdv_unbinn_p2p_hello                    (binn const *obj, mdv_msg_p2p_hello *msg);


bool            mdv_binn_p2p_status                     (mdv_msg_p2p_status const *msg, binn *obj);
bool            mdv_unbinn_p2p_status                   (binn const *obj, mdv_msg_p2p_status *msg);


bool            mdv_binn_p2p_linkstate                  (mdv_msg_p2p_linkstate const *msg, binn *obj);
bool            mdv_unbinn_p2p_linkstate                (binn const *obj, mdv_msg_p2p_linkstate *msg);


bool            mdv_binn_p2p_toposync                   (mdv_msg_p2p_toposync const *msg, binn *obj);
bool            mdv_unbinn_p2p_toposync                 (binn const *obj, mdv_msg_p2p_toposync *msg);
void            mdv_p2p_toposync_free                   (mdv_msg_p2p_toposync *msg);


bool            mdv_binn_p2p_topodiff                   (mdv_msg_p2p_topodiff const *msg, binn *obj);
bool            mdv_unbinn_p2p_topodiff                 (binn const *obj, mdv_msg_p2p_topodiff *msg);
void            mdv_p2p_topodiff_free                   (mdv_msg_p2p_topodiff *msg);


bool            mdv_binn_p2p_trlog_sync                 (mdv_msg_p2p_trlog_sync const *msg, binn *obj);
bool            mdv_unbinn_p2p_trlog_sync               (binn const *obj, mdv_msg_p2p_trlog_sync *msg);


bool            mdv_binn_p2p_trlog_state                (mdv_msg_p2p_trlog_state const *msg, binn *obj);
bool            mdv_unbinn_p2p_trlog_state              (binn const *obj, mdv_msg_p2p_trlog_state *msg);


bool            mdv_binn_p2p_trlog_data                 (mdv_msg_p2p_trlog_data const *msg, binn *obj);
bool            mdv_unbinn_p2p_trlog_data               (binn const *obj, mdv_msg_p2p_trlog_data *msg);
void            mdv_p2p_trlog_data_free                 (mdv_msg_p2p_trlog_data *msg);


bool            mdv_binn_p2p_broadcast                  (mdv_msg_p2p_broadcast const *msg, binn *obj);
bool            mdv_unbinn_p2p_broadcast                (binn const *obj, mdv_msg_p2p_broadcast *msg);
