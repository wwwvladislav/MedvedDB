/**
 * @file mdv_transport.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Transport interface definition
 * @version 0.1
 * @date 2020-02-07
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_def.h>


/// Transport
typedef struct mdv_transport mdv_transport;


typedef mdv_transport * (*mdv_transport_retain_fn)    (mdv_transport *tr);
typedef uint32_t        (*mdv_transport_release_fn)   (mdv_transport *tr);
typedef mdv_errno       (*mdv_transport_write_fn)     (mdv_transport *tr, void const *data, size_t *len);
typedef mdv_errno       (*mdv_transport_write_all_fn) (mdv_transport *tr, void const *data, size_t len);
typedef mdv_errno       (*mdv_transport_read_fn)      (mdv_transport *tr, void *data, size_t *len);
typedef mdv_errno       (*mdv_transport_read_all_fn)  (mdv_transport *tr, void *data, size_t len);
typedef mdv_errno       (*mdv_transport_skip_fn)      (mdv_transport *tr, size_t len);
typedef mdv_descriptor  (*mdv_transport_fd_fn)        (mdv_transport *tr);


/// Interface for transports
typedef struct
{
    mdv_transport_retain_fn     retain;         ///< Function for transport retain
    mdv_transport_release_fn    release;        ///< Function for transport release
    mdv_transport_write_fn      write;          ///< Function for writing to transport channel
    mdv_transport_write_all_fn  write_all;      ///< Function for writing to transport channel
    mdv_transport_read_fn       read;           ///< Function for reading from transport channel
    mdv_transport_read_all_fn   read_all;       ///< Function for reading from transport channel
    mdv_transport_skip_fn       skip;           ///< Function for data skipping
    mdv_transport_fd_fn         fd;             ///< Function for file descriptor obtaining
} mdv_itransport;


struct mdv_transport
{
    mdv_itransport const    *vptr;              ///< Virtual methods table
};


mdv_transport * mdv_transport_retain    (mdv_transport *tr);
uint32_t        mdv_transport_release   (mdv_transport *tr);
mdv_errno       mdv_transport_write     (mdv_transport *tr, void const *data, size_t *len);
mdv_errno       mdv_transport_write_all (mdv_transport *tr, void const *data, size_t len);
mdv_errno       mdv_transport_read      (mdv_transport *tr, void *data, size_t *len);
mdv_errno       mdv_transport_read_all  (mdv_transport *tr, void *data, size_t len);
mdv_errno       mdv_transport_skip      (mdv_transport *tr, size_t len);
mdv_descriptor  mdv_transport_fd        (mdv_transport *tr);
