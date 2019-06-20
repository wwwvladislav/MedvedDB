#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <mdv_binn.h>


#define mdv_message(name, id, fields)                   \
    enum { mdv_msg_##name##_id = id };                  \
    static char const *mdv_msg_##name##_name = #name;   \
    typedef struct                                      \
    {                                                   \
        fields;                                         \
    } mdv_msg_##name;


#define MDV_HELLO_SIGNATURE                             \
    {                                                   \
        0xee, 0xe1, 0x78, 0xa7, 0x13, 0xed, 0xeb, 0xd0, \
        0xdb, 0x54, 0x3e, 0xb4, 0x59, 0x9b, 0xa5, 0x66, \
        0x87, 0x88, 0xf1, 0xd6, 0xa1, 0x21, 0xd4, 0xf6, \
        0xd3, 0x9c, 0x1d, 0xed, 0x25, 0xa8, 0x13, 0x96  \
    }


mdv_message(status, 0,
    int         err;
    char        message[1];
);


mdv_message(hello, 1,
    uint8_t     signature[32];
    uint32_t    version;
);


char const *        mdv_msg_name        (uint32_t id);

void                mdv_msg_free        (void *msg);

binn *              mdv_binn_hello      (mdv_msg_hello const *msg);
bool                mdv_unbinn_hello    (binn *obj, mdv_msg_hello *msg);

binn *              mdv_binn_status     (mdv_msg_status const *msg);
mdv_msg_status *    mdv_unbinn_status   (binn *obj);
