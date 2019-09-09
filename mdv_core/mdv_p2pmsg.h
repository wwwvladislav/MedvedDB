#pragma once
#include <mdv_messages.h>
#include <mdv_topology.h>


/*
    P1                                  P2
    | HELLO >>>>>                       |   Handshake
    |                       <<<<< HELLO |
    |                                   |
    | TOPOLOGY SYNC >>>>>               |   Network topology synchronization. Send topology.
    |               <<<<< TOPOLOGY DIFF |
    |                                   |
    | LINK STATE >>>>>                  |   [broadcast]
 */

mdv_message_def(p2p_hello, 1000,
    uint32_t    version;            ///< Version
    mdv_uuid    uuid;               ///< Unique identifier
    char       *listen;             ///< Listening address
);


mdv_message_def(p2p_linkstate, 1000 + 1,
    mdv_toponode    src;            ///< First peer unique identifier
    mdv_toponode    dst;            ///< Second peer unique identifier
    uint8_t         connected:1;    ///< 1, if first peer connected to second peer
    uint32_t        peers_count;    ///< Count of notified peers
    uint32_t       *peers;          ///< Notified peers list
);


mdv_message_def(p2p_toposync, 1000 + 2,
    mdv_topology *topology;         ///< Network topology
);


mdv_message_def(p2p_topodiff, 1000 + 3,
    mdv_topology *topology;         ///< Network topology
);


char const *            mdv_p2p_msg_name                        (uint32_t id);


bool                    mdv_binn_p2p_hello                      (mdv_msg_p2p_hello const *msg, binn *obj);
bool                    mdv_unbinn_p2p_hello                    (binn const *obj, mdv_msg_p2p_hello *msg);


bool                    mdv_binn_p2p_linkstate                  (mdv_msg_p2p_linkstate const *msg, binn *obj);
mdv_toponode *          mdv_unbinn_p2p_linkstate_src            (binn const *obj);
mdv_toponode *          mdv_unbinn_p2p_linkstate_dst            (binn const *obj);
bool *                  mdv_unbinn_p2p_linkstate_connected      (binn const *obj);
uint32_t *              mdv_unbinn_p2p_linkstate_peers_count    (binn const *obj);
bool                    mdv_unbinn_p2p_linkstate_peers          (binn const *obj, uint32_t *peers, uint32_t peers_count);


bool                    mdv_binn_p2p_toposync                   (mdv_msg_p2p_toposync const *msg, binn *obj);
bool                    mdv_unbinn_p2p_toposync                 (binn const *obj, mdv_msg_p2p_toposync *msg);
void                    mdv_p2p_toposync_free                   (mdv_msg_p2p_toposync *msg);


bool                    mdv_binn_p2p_topodiff                   (mdv_msg_p2p_topodiff const *msg, binn *obj);
bool                    mdv_unbinn_p2p_topodiff                 (binn const *obj, mdv_msg_p2p_topodiff *msg);
void                    mdv_p2p_topodiff_free                   (mdv_msg_p2p_topodiff *msg);
