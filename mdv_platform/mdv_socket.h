#pragma once
#include "mdv_def.h"
#include "mdv_string.h"

#ifdef MDV_PLATFORM_LINUX
    #include <sys/socket.h>
#elif defined(MDV_PLATFORM_WINDOWS)
     #include <ws2def.h>
#endif


typedef struct sockaddr_storage mdv_sockaddr;


typedef enum
{
    MDV_SOCK_STREAM = 0,
    MDV_SOCK_DGRAM,
    MDV_SOCK_UNKNOWN
} mdv_socket_type;


mdv_errno      mdv_str2sockaddr(mdv_string const str, mdv_socket_type *protocol, mdv_sockaddr *addr);
mdv_errno      mdv_sockaddr2str(mdv_socket_type protocol, mdv_sockaddr const *addr, mdv_string *str);


mdv_descriptor mdv_socket(mdv_socket_type type);
void           mdv_socket_close(mdv_descriptor sock);
mdv_errno      mdv_socket_listen(mdv_descriptor sock);
mdv_descriptor mdv_socket_accept(mdv_descriptor sock, mdv_sockaddr *peer);
mdv_errno      mdv_socket_bind(mdv_descriptor sock, mdv_sockaddr const *addr);
mdv_errno      mdv_socket_connect(mdv_descriptor sock, mdv_sockaddr const *addr);


mdv_errno      mdv_socket_reuse_addr(mdv_descriptor sock);
mdv_errno      mdv_socket_reuse_port(mdv_descriptor sock);
mdv_errno      mdv_socket_nonblock(mdv_descriptor sock);


/**
 * @brief Specify the minimum number of bytes in the buffer until the socket layer will pass the data to the user on receiving
 *
 * @param sock [in]         socket descriptor
 * @param size [in]         minimum number of bytes for receiving
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero value
 */
mdv_errno      mdv_socket_min_recv_size(mdv_descriptor sock, int size);


/**
 * @brief Specify the minimum number of bytes in the buffer until the socket layer will pass the data to the protocol
 *
 * @param sock [in]         socket descriptor
 * @param size [in]         minimum number of bytes to send
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero value
 */
mdv_errno      mdv_socket_min_send_size(mdv_descriptor sock, int size);


/**
 * @brief Enable keepalive option for TCP socket
 *
 * @param sock [in]         socket descriptor
 * @param keepidle [in]     Start keeplives after this period (in seconds)
 * @param keepcnt [in]      Number of keepalives before death
 * @param keepintvl [in]    Interval between keepalives (in seconds)
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero value
 */
mdv_errno mdv_socket_tcp_keepalive(mdv_descriptor sock, int keepidle, int keepcnt, int keepintvl);


/**
 * @brief Wake up listener only when data arrive
 *
 * @param sock [in]         socket descriptor
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero value
 */
mdv_errno mdv_socket_tcp_defer_accept(mdv_descriptor sock);


/**
 * @brief Never send partially complete segments
 *
 * @param sock [in]         socket descriptor
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero value
 */
mdv_errno mdv_socket_tcp_cork(mdv_descriptor sock);

