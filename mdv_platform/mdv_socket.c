#include "mdv_socket.h"
#include "mdv_log.h"
#include "mdv_limits.h"
#include "mdv_errno.h"
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>


static volatile int mdv_netlib_init_count = 0;


mdv_errno mdv_netlib_init()
{
    if (mdv_netlib_init_count++ == 0)
    {
        // TODO
    }

    return MDV_OK;
}


void mdv_netlib_uninit()
{
    if (--mdv_netlib_init_count == 0)
    {
        // TODO
    }
}


mdv_errno mdv_str2sockaddr(mdv_string const str, mdv_sockaddr *addr)
{
    if (mdv_str_empty(str) || !addr)
    {
        MDV_LOGE("Invalid argument");
        return MDV_INVALID_ARG;
    }

    if (str.size > MDV_ADDR_LEN_MAX)
    {
        MDV_LOGE("Address length is too long");
        return MDV_INVALID_ARG;
    }

    memset(addr, 0, sizeof *addr);

    char host[str.size];
    memcpy(host, str.ptr, str.size);

    char *port = strrchr(host, ':');
    if (port) *(port++) = 0;

    struct addrinfo *result = 0;
    struct addrinfo hints = { 0 };
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int err = getaddrinfo(host, port, &hints, &result);
    if (err)
    {
        MDV_LOGE("getaddrinfo failed");
        return err;
    }

    mdv_errno ret = MDV_FAILED;

    for(struct addrinfo *ptr = result; ptr; ptr = ptr->ai_next)
    {
        if (ptr->ai_family == AF_INET)
        {
            struct sockaddr_in *src_addr = (struct sockaddr_in*)ptr->ai_addr;
            struct sockaddr_in *dst_addr = (struct sockaddr_in*)addr;
            memcpy(dst_addr, src_addr, sizeof *src_addr);
            ret = MDV_OK;
            break;
        }
        else if (!addr->ss_family && ptr->ai_family == AF_INET6)
        {
            struct sockaddr_in6 *src_addr = (struct sockaddr_in6*)ptr->ai_addr;
            struct sockaddr_in6 *dst_addr = (struct sockaddr_in6*)addr;
            memcpy(dst_addr, src_addr, sizeof *src_addr);
            ret = MDV_OK;
        }
    }

    freeaddrinfo(result);

    return ret;
}


mdv_errno mdv_sockaddr2str(mdv_sockaddr const *addr, mdv_string *str)
{
    if (mdv_str_empty(*str) || !addr)
    {
        MDV_LOGE("Invalid argument");
        return MDV_INVALID_ARG;
    }

    int err = getnameinfo((struct sockaddr const *)addr, sizeof *addr, str->ptr, str->size, 0, 0, NI_NUMERICHOST | NI_NUMERICSERV);

    if (err)
        return err;

    unsigned addr_len = strlen(str->ptr);
    if (addr_len < str->size)
        snprintf(str->ptr + addr_len, str->size - addr_len, ":%u", ntohs(((struct sockaddr_in const *)addr)->sin_port));

    return MDV_OK;
}


mdv_descriptor mdv_socket(mdv_socket_type type)
{
    if (mdv_netlib_init() != MDV_OK)
        return MDV_INVALID_DESCRIPTOR;

    static const int types[] = { SOCK_STREAM, SOCK_DGRAM };

    int s = socket(AF_INET, types[type], 0);

    if (s == -1)
    {
        int err = mdv_error();
        MDV_LOGE("Socket creation was failed with error: '%s' (%d)", mdv_strerror(err), err);
        return MDV_INVALID_DESCRIPTOR;
    }

    MDV_LOGD("Socket %d opened", s);

    mdv_descriptor sock = 0;

    *(int*)&sock = s;

    return sock;
}


void mdv_socket_close(mdv_descriptor sock)
{
    if (sock != MDV_INVALID_DESCRIPTOR)
    {
        int s = *(int*)&sock;
        close(s);
        MDV_LOGD("Socket %d closed", s);
        mdv_netlib_uninit();
    }
}


mdv_errno mdv_socket_reuse_addr(mdv_descriptor sock)
{
    int optval = 1;

    int s = *(int*)&sock;

    int err = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (err == -1)
    {
        MDV_LOGE("Unable to reuse address");
        return mdv_error();
    }

    return MDV_OK;
}


mdv_errno mdv_socket_reuse_port(mdv_descriptor sock)
{
    int optval = 1;

    int s = *(int*)&sock;

    int err = setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if (err == -1)
    {
        MDV_LOGE("Unable to reuse port");
        return mdv_error();
    }

    return MDV_OK;
}

mdv_errno mdv_socket_nonblock(mdv_descriptor sock)
{
    int s = *(int*)&sock;

    int err = fcntl(s, F_SETFL, O_NONBLOCK);

    if (err == -1)
    {
        MDV_LOGE("Switching socket %d to nonblocking mode was failed", s);
        return mdv_error();
    }

    return MDV_OK;
}


mdv_errno mdv_socket_listen(mdv_descriptor sock)
{
    int s = *(int*)&sock;

    int err = listen(s, MDV_LISTEN_BACKLOG);

    if (err == -1)
    {
        MDV_LOGE("Socket %d listening was failed", s);
        return mdv_error();
    }

    return MDV_OK;
}


mdv_descriptor mdv_socket_accept(mdv_descriptor sock, mdv_sockaddr *peer)
{
    int s = *(int*)&sock;

    socklen_t addr_len = sizeof(mdv_sockaddr);

    int peer_sock = accept(s, (struct sockaddr *)peer, peer ? &addr_len : 0);

    if (peer_sock == -1)
    {
        MDV_LOGE("Socket %d accepting was failed", s);
        return MDV_INVALID_DESCRIPTOR;
    }

    MDV_LOGD("Socket %d accepted", peer_sock);

    return (mdv_descriptor)(size_t)peer_sock;
}


mdv_errno mdv_socket_bind(mdv_descriptor sock, mdv_sockaddr const *addr)
{
    int s = *(int*)&sock;

    const struct sockaddr *saddr = (const struct sockaddr *)addr;

    int err = bind(s, saddr, sizeof(mdv_sockaddr));

    if (err == -1)
    {
        MDV_LOGE("Socket %d binding was failed", s);
        return mdv_error();
    }

    return MDV_OK;
}


mdv_errno mdv_socket_connect(mdv_descriptor sock, mdv_sockaddr const *addr)
{
    int s = *(int*)&sock;

    const struct sockaddr *saddr = (const struct sockaddr *)addr;

    int err = connect(s, saddr, sizeof(mdv_sockaddr));

    if (err == -1)
    {
        MDV_LOGE("Socket %d connecting was failed", s);
        return mdv_error();
    }

    return MDV_OK;
}


mdv_errno mdv_socket_send(mdv_descriptor sock, void const *data, size_t len, size_t *write_len)
{
    if (!len)
    {
        *write_len = 0;
        return MDV_OK;
    }

    int s = *(int*)&sock;

    int res = send(s, data, len, 0);

    if (res == -1)
    {
        return mdv_error();
    }

    *write_len = res;

    return MDV_OK;
}


mdv_errno mdv_socket_recv(mdv_descriptor sock, void *data, size_t len, size_t *read_len)
{
    if (!len)
    {
        *read_len = 0;
        return MDV_OK;
    }

    int s = *(int*)&sock;

    int res = recv(s, data, len, 0);

    switch(res)
    {
        case -1:    return mdv_error();
        case 0:     return MDV_CLOSED;
        default:    break;
    }

    *read_len = res;

    return MDV_OK;
}
