#pragma once
#include <mdv_messages.h>


mdv_message_def(p2p_hello, 1000,
    uint32_t    version;            ///< Version
    mdv_uuid    uuid;               ///< Unique identifier
    char       *listen;             ///< Listening address
);

mdv_message_def(p2p_linkstate, 1000 + 1,
    mdv_uuid    peer_1;             ///< First peer unique identifier
    mdv_uuid    peer_2;             ///< Second peer unique identifier
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
mdv_uuid *              mdv_unbinn_p2p_linkstate_peer_1     (binn const *obj);
mdv_uuid *              mdv_unbinn_p2p_linkstate_peer_2     (binn const *obj);
bool *                  mdv_unbinn_p2p_linkstate_connected  (binn const *obj);
uint32_t *              mdv_unbinn_p2p_linkstate_peers_count(binn const *obj);
