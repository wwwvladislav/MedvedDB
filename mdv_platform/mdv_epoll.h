/**
 * @file
 * @brief API for monitoring multiple file descriptors to see if I/O is possible on any of them.
 *
 * @details This file contains main functions and type definitions for epoll API.
*/

#pragma once
#include "mdv_def.h"


/// Epoll event types. Events might be combined with bitwise OR operator.
typedef enum mdv_epoll_events
{
    MDV_EPOLLIN         = 1 << 0,   ///< The associated file is available for read operations.
    MDV_EPOLLOUT        = 1 << 1,   ///< The associated file is available for write operations.
    MDV_EPOLLRDHUP      = 1 << 2,   ///< Stream socket peer closed connection, or shut down writing half of connection.
    MDV_EPOLLERR        = 1 << 3,   ///< Error condition happened on the associated file descriptor.
    MDV_EPOLLET         = 1 << 4,   ///< Sets the Edge Triggered behavior for the associated file descriptor.
    MDV_EPOLLONESHOT    = 1 << 5,   ///< Sets the one-shot behavior for the associated file descriptor. The user must call mdv_epoll_add if event was thrown on associated file descriptor.
    MDV_EPOLLEXCLUSIVE  = 1 << 6    ///< Sets an exclusive wakeup mode. Only one waiting thread is wake up.
} mdv_epoll_events;


/**
 * @brief The event description which is associated with file descriptor in mdv_epoll_add/mdv_epoll_del functions.
 */
typedef struct
{
    uint32_t     events;    ///< Epoll events. It should be the bitwise OR combination of the mdv_epoll_events.
    void        *data;      ///< User data associated with event
} mdv_epoll_event;


/**
 * @brief Creates a new epoll instance.
 *
 * @return  a file descriptor referring to Epoll instance.
 * @return  MDV_INVALID_DESCRIPTOR if error is occured.
 */
mdv_descriptor mdv_epoll_create();


/**
 * @brief Closes epoll instance.
 *
 * @details Function isn't failed if epoll instance is MDV_INVALID_DESCRIPTOR.
 *
 * @param epfd [in] Epoll instance which created by mdv_epoll_create().
 */
void mdv_epoll_close(mdv_descriptor epfd);


/**
 * @brief Register the target file descriptor fd on the epoll instance referred to by the epfd and associate the event with fd.
 *
 * @param epfd [in]     epoll instance descriptor
 * @param fd [in]       file descriptor
 * @param event [in]    event description
 *
 * @return MDV_OK if fd is successfully registered in epoll instance
 * @return non zero value if error has occurred
 */
mdv_errno mdv_epoll_add(mdv_descriptor epfd, mdv_descriptor fd, mdv_epoll_event event);


/**
 * @brief Change the event associated with the target file descriptor fd.
 *
 * @param epfd [in]     epoll instance descriptor
 * @param fd [in]       file descriptor
 * @param event [in]    event description
 *
 * @return MDV_OK if fd is successfully registered in epoll instance
 * @return non zero value if error has occurred
 */
mdv_errno mdv_epoll_mod(mdv_descriptor epfd, mdv_descriptor fd, mdv_epoll_event event);


/**
 * @brief Remove the target file descriptor fd from the epoll instance epfd.
 *
 * @param epfd [in] epoll instance descriptor
 * @param fd [in]   file descriptor
 *
 * @return MDV_OK if fd is successfully removed from epoll instance
 * @return non zero value if error has occurred
 */
mdv_errno mdv_epoll_del(mdv_descriptor epfd, mdv_descriptor fd);


/**
 *
 * @param epfd [in]             epoll instance descriptor
 * @param events [out]          events that will be available for the caller
 * @param size [in]             events array size\n
 *             [out]            number of events which were written into the events array
 * @param timeout [in]          the number of milliseconds that epoll_wait will block. -1 causes epoll_wait to block indefinitely.
 *
 * @return MDV_OK if wait operation successfully completed
 * @return non zero value if error has occurred
 */
mdv_errno mdv_epoll_wait(mdv_descriptor epfd, mdv_epoll_event *events, uint32_t *size, int timeout);
