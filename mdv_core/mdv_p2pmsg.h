#pragma once
#include <mdv_messages.h>
#include <mdv_topology.h>


/*
    P1                                  P2
    |                                   |
    | HELLO >>>>>                       |   Handshake
    |                       <<<<< HELLO |
    |                                   |
    |                                   |
    | TOPOLOGY SYNC >>>>>               |   Network topology synchronization. Send topology.
    |               <<<<< TOPOLOGY DIFF |
    |                                   |
    |                                   |
    | LINK STATE >>>>>                  |   [broadcast]
    |                                   |
    |                                   |
    | CFSLOG SYNC >>>>>                 |   Storage synchronization request.
    |       <<<<< CFSLOG STATE / STATUS |   Last transaction log record identifier.
    | CFSLOG DATA >>>>>                 |   Transaction log records.
    |                      <<<<< STATUS |
 */


mdv_message_def(p2p_hello, 1000,
    uint32_t    version;            ///< Version
    mdv_uuid    uuid;               ///< Unique identifier
    char       *listen;             ///< Listening address
);


mdv_message_def(p2p_status, 1000 + 1,
    int         err;                ///< Error code
    char const *message;            ///< Status message
);


mdv_message_def(p2p_linkstate, 1000 + 2,
    mdv_toponode    src;            ///< First peer unique identifier
    mdv_toponode    dst;            ///< Second peer unique identifier
    uint8_t         connected:1;    ///< 1, if first peer connected to second peer
    uint32_t        peers_count;    ///< Count of notified peers
    uint32_t       *peers;          ///< Notified peers list
);


mdv_message_def(p2p_toposync, 1000 + 3,
    mdv_topology *topology;         ///< Network topology
);


mdv_message_def(p2p_topodiff, 1000 + 4,
    mdv_topology *topology;         ///< Network topology
);


mdv_message_def(p2p_cfslog_sync, 1000 + 5,
    mdv_uuid    uuid;               ///< CF Storage unique identifier
    mdv_uuid    peer;               ///< Peer unique identifier
);


mdv_message_def(p2p_cfslog_state, 1000 + 6,
    mdv_uuid    uuid;               ///< CF Storage unique identifier
    mdv_uuid    peer;               ///< Peer unique identifier
    uint64_t    trlog_top;          ///< Last transaction log record identifier for requested CF Storage and peer.
);


typedef struct
{
    uint64_t                row_id;
    mdv_data                op;
} mdv_cfslog_data;

mdv_message_def(p2p_cfslog_data, 1000 + 7,
    mdv_uuid         uuid;          ///< CF Storage unique identifier
    mdv_uuid         peer;          ///< Peer unique identifier
    uint32_t         count;         ///< log records count
    mdv_cfslog_data *rows;          ///< transaction log data
);


char const *            mdv_p2p_msg_name                        (uint32_t id);


bool                    mdv_binn_p2p_hello                      (mdv_msg_p2p_hello const *msg, binn *obj);
bool                    mdv_unbinn_p2p_hello                    (binn const *obj, mdv_msg_p2p_hello *msg);


bool                    mdv_binn_p2p_status                     (mdv_msg_p2p_status const *msg, binn *obj);
bool                    mdv_unbinn_p2p_status                   (binn const *obj, mdv_msg_p2p_status *msg);


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


bool                    mdv_binn_p2p_cfslog_sync                (mdv_msg_p2p_cfslog_sync const *msg, binn *obj);
bool                    mdv_unbinn_p2p_cfslog_sync              (binn const *obj, mdv_msg_p2p_cfslog_sync *msg);


bool                    mdv_binn_p2p_cfslog_state               (mdv_msg_p2p_cfslog_state const *msg, binn *obj);
bool                    mdv_unbinn_p2p_cfslog_state             (binn const *obj, mdv_msg_p2p_cfslog_state *msg);


bool                    mdv_binn_p2p_cfslog_data                (mdv_msg_p2p_cfslog_data const *msg, binn *obj);
mdv_uuid *              mdv_unbinn_p2p_cfslog_data_uuid         (binn const *obj);
mdv_uuid *              mdv_unbinn_p2p_cfslog_data_peer         (binn const *obj);
uint32_t *              mdv_unbinn_p2p_cfslog_data_count        (binn const *obj);
uint64_t *              mdv_unbinn_p2p_cfslog_data_size         (binn const *obj);
bool                    mdv_unbinn_p2p_cfslog_data_rows         (binn const *obj,
                                                                 mdv_cfslog_data *rows, uint32_t rows_count,
                                                                 uint8_t *dataspace, size_t dataspace_size);
