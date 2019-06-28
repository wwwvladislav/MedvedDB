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
    MDV_SOCK_DGRAM
} mdv_socket_type;


mdv_errno      mdv_str2sockaddr(mdv_string const str, mdv_sockaddr *addr);
mdv_errno      mdv_sockaddr2str(mdv_sockaddr const *addr, mdv_string *str);

mdv_descriptor mdv_socket(mdv_socket_type type);
void           mdv_socket_close(mdv_descriptor sock);
mdv_errno      mdv_socket_reuse_addr(mdv_descriptor sock);
mdv_errno      mdv_socket_reuse_port(mdv_descriptor sock);
mdv_errno      mdv_socket_nonblock(mdv_descriptor sock);
mdv_errno      mdv_socket_listen(mdv_descriptor sock);
mdv_descriptor mdv_socket_accept(mdv_descriptor sock, mdv_sockaddr *peer);
mdv_errno      mdv_socket_bind(mdv_descriptor sock, mdv_sockaddr const *addr);
mdv_errno      mdv_socket_connect(mdv_descriptor sock, mdv_sockaddr const *addr);
mdv_errno      mdv_socket_send(mdv_descriptor sock, void const *data, size_t len, size_t *write_len);
mdv_errno      mdv_socket_recv(mdv_descriptor sock, void *data, size_t len, size_t *read_len);
