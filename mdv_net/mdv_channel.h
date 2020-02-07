/**
 * @file mdv_channel.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Channel interface definition
 * @version 0.1
 * @date 2020-01-27
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_def.h>
#include <mdv_uuid.h>


/// Channel
typedef struct mdv_channel mdv_channel;


/// Channel type
typedef uint8_t mdv_channel_t;


/// Channel direction
typedef enum
{
    MDV_CHIN = 0,       ///< Incomming connection
    MDV_CHOUT           ///< Outgoing connection
} mdv_channel_dir;


typedef mdv_channel *    (*mdv_channel_retain_fn) (mdv_channel *);
typedef uint32_t         (*mdv_channel_release_fn)(mdv_channel *);
typedef mdv_channel_t    (*mdv_channel_type_fn)   (mdv_channel const *);
typedef mdv_uuid const * (*mdv_channel_id_fn)     (mdv_channel const *);
typedef mdv_errno        (*mdv_channel_recv_fn)   (mdv_channel *);


/// Interface for channels
typedef struct
{
    mdv_channel_retain_fn  retain;          ///< Function for channel retain
    mdv_channel_release_fn release;         ///< Function for channel release
    mdv_channel_type_fn    type;            ///< Function for channel type getting
    mdv_channel_id_fn      id;              ///< Function for channel identifier getting
    mdv_channel_recv_fn    recv;            ///< Data receiving
} mdv_ichannel;


struct mdv_channel
{
    mdv_ichannel const     *vptr;           ///< Virtual methods table
};


mdv_channel *    mdv_channel_retain(mdv_channel *channel);
uint32_t         mdv_channel_release(mdv_channel *channel);
mdv_channel_t    mdv_channel_type(mdv_channel const *channel);
mdv_uuid const * mdv_channel_id(mdv_channel const *channel);
mdv_errno        mdv_channel_recv(mdv_channel *channel);
