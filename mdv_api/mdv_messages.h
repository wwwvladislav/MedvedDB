#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <mdv_binn.h>
#include <mdv_types.h>
#include <mdv_table.h>
#include <mdv_rowset.h>
#include <mdv_topology.h>


/*
   User                               Server
     | HELLO >>>>>                      |
     |             <<<<< HELLO / STATUS |
     |                                  |
     | CREATE TABLE >>>>>               |
     |        <<<<< TABLE INFO / STATUS |
     |                                  |
     | GET TABLE >>>>>                  |
     |        <<<<< TABLE DESC / STATUS |
     |                                  |
     | GET TOPOLOGY >>>>>               |
     |          <<<<< TOPOLOGY / STATUS |
     |                                  |
     | INSERT INTO >>>>>                |
     |                     <<<<< STATUS |
 */


#define mdv_message_id_def(name, id)                    \
    enum { mdv_msg_##name##_id = id };


#define mdv_message_id(name) mdv_msg_##name##_id


#define mdv_message_def(name, id, fields)               \
    mdv_message_id_def(name, id);                       \
    typedef struct                                      \
    {                                                   \
        fields;                                         \
    } mdv_msg_##name;


mdv_message_def(status, 1,
    int         err;
    char const *message;
);


mdv_message_def(hello, 2,
    uint32_t    version;
    mdv_uuid    uuid;
);


mdv_message_def(create_table, 3,
    mdv_table_desc *desc;
);


mdv_message_def(get_table, 4,
    mdv_uuid    id;
);


mdv_message_def(table_info, 5,
    mdv_uuid    id;
);


mdv_message_def(table_desc, 6,
    mdv_table_desc const *desc;
);


mdv_message_def(get_topology, 7,
);


mdv_message_def(topology, 8,
    mdv_topology *topology;
);


mdv_message_def(insert_into, 9,
    mdv_uuid    table;
    binn       *rows;
);


char const *                mdv_msg_name                    (uint32_t id);


bool                        mdv_binn_hello                  (mdv_msg_hello const *msg, binn *obj);
bool                        mdv_unbinn_hello                (binn const *obj, mdv_msg_hello *msg);


bool                        mdv_binn_status                 (mdv_msg_status const *msg, binn *obj);
bool                        mdv_unbinn_status               (binn const *obj, mdv_msg_status *msg);


bool                        mdv_binn_create_table           (mdv_msg_create_table const *msg, binn *obj);
bool                        mdv_unbinn_create_table         (binn const *obj, mdv_msg_create_table *msg);
void                        mdv_create_table_free           (mdv_msg_create_table *msg);


bool                        mdv_binn_get_table              (mdv_msg_get_table const *msg, binn *obj);
bool                        mdv_unbinn_get_table            (binn const *obj, mdv_msg_get_table *msg);


bool                        mdv_binn_table_info             (mdv_msg_table_info const *msg, binn *obj);
bool                        mdv_unbinn_table_info           (binn const *obj, mdv_msg_table_info *msg);


bool                        mdv_binn_get_topology           (mdv_msg_get_topology const *msg, binn *obj);
bool                        mdv_unbinn_get_topology         (binn const *obj, mdv_msg_get_topology *msg);


bool                        mdv_binn_topology               (mdv_msg_topology const *msg, binn *obj);
mdv_topology              * mdv_unbinn_topology             (binn const *obj);


bool                        mdv_binn_insert_into            (mdv_msg_insert_into const *msg, binn *obj);
bool                        mdv_unbinn_insert_into          (binn const * obj, mdv_msg_insert_into *msg);
