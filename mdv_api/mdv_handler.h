#pragma once
#include "mdv_messages.h"


typedef struct mdv_message
{
    uint32_t id;
    binn     body;
} mdv_message;


extern const mdv_message mdv_no_message;


typedef struct
{
    void *arg;
    bool (*fn)(mdv_message const *, void *, mdv_message *);
} mdv_message_handler;

