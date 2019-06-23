#pragma once
#include "mdv_messages.h"


typedef struct mdv_message
{
    uint32_t  id;
    binn     *body;
} mdv_message;


extern const mdv_message mdv_no_message;


typedef struct
{
    void *arg;
    mdv_message (*fn)(mdv_message, void *);
} mdv_message_handler;

