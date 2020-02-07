#include "mdv_channel.h"


mdv_channel *    mdv_channel_retain(mdv_channel *channel)       { return channel->vptr->retain(channel); }
uint32_t         mdv_channel_release(mdv_channel *channel)      { return channel ? channel->vptr->release(channel) : 0; }
mdv_channel_t    mdv_channel_type(mdv_channel const *channel)   { return channel->vptr->type(channel); }
mdv_uuid const * mdv_channel_id(mdv_channel const *channel)     { return channel->vptr->id(channel); }
mdv_errno        mdv_channel_recv(mdv_channel *channel)         { return channel->vptr->recv(channel); }
