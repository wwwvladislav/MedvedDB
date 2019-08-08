#pragma once
#include <mdv_messages.h>


/*
    P1                     P2
     |    --- HELLO -->    |
     |    <-- HELLO ---    |
     |  -- LINK STATE -->  |
 */

mdv_message_def(p2p_hello, 1000,
    uint32_t    version;            ///< Version
    mdv_uuid    uuid;               ///< Unique identifier
    char       *listen;             ///< Listening address
);

mdv_message_def(p2p_linkstate, 1000 + 1,
    mdv_uuid    src_peer;           ///< First peer unique identifier
    mdv_uuid    dst_peer;           ///< Second peer unique identifier
    char const *src_listen;         ///< First peer listening address
    uint8_t     connected:1;        ///< 1, if first peer connected to second peer
    uint32_t    peers_count;        ///< Count of notified peers
    uint32_t   *peers;              ///< Notified peers list
);


char const *            mdv_p2p_msg_name        (uint32_t id);


bool                    mdv_binn_p2p_hello                  (mdv_msg_p2p_hello const *msg, binn *obj);
uint32_t *              mdv_unbinn_p2p_hello_version        (binn const *obj);
mdv_uuid *              mdv_unbinn_p2p_hello_uuid           (binn const *obj);
char const *            mdv_unbinn_p2p_hello_listen         (binn const *obj);

bool                    mdv_binn_p2p_linkstate              (mdv_msg_p2p_linkstate const *msg, binn *obj);
mdv_uuid *              mdv_unbinn_p2p_linkstate_src_peer   (binn const *obj);
mdv_uuid *              mdv_unbinn_p2p_linkstate_dst_peer   (binn const *obj);
char const *            mdv_unbinn_p2p_linkstate_src_listen (binn const *obj);
bool *                  mdv_unbinn_p2p_linkstate_connected  (binn const *obj);
uint32_t *              mdv_unbinn_p2p_linkstate_peers_count(binn const *obj);
bool                    mdv_unbinn_p2p_linkstate_peers      (binn const *obj, uint32_t *peers, uint32_t peers_count);
