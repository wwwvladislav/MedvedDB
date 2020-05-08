#include "mdv_netaddr.h"
#include <mdv_log.h>
#include <mdv_limits.h>
#include <string.h>
#include <stdio.h>


#ifdef MDV_PLATFORM_LINUX
    #include <linux/tcp.h>
    #include <sys/types.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <arpa/inet.h>
#endif


static mdv_socket_type mdv_str2socket_type(char const *str)
{
    if(strncasecmp("tcp", str, 3) == 0)
        return MDV_SOCK_STREAM;
    if(strncasecmp("udp", str, 3) == 0)
        return MDV_SOCK_DGRAM;
    return MDV_SOCK_UNKNOWN;
}


static char const * mdv_socket_type2str(mdv_socket_type t)
{
    switch(t)
    {
        case MDV_SOCK_STREAM:   return "tcp";
        case MDV_SOCK_DGRAM:    return "udp";
        default:
            break;
    }
    return 0;
}


mdv_errno mdv_str2netaddr(char const *str, mdv_netaddr *addr)
{
    if (!str || !addr)
    {
        MDV_LOGE("Invalid argument");
        return MDV_INVALID_ARG;
    }

    size_t const str_size = strlen(str) + 1;

    if (str_size > MDV_ADDR_LEN_MAX)
    {
        MDV_LOGE("Address length is too long");
        return MDV_INVALID_ARG;
    }

    memset(addr, 0, sizeof *addr);

    char buf[str_size];
    memcpy(buf, str, str_size);

    char *host = buf;

    char *proto = strstr(host, "://");
    if (proto)
    {
        *(proto++) = 0;
        host = proto + 2;
        proto = buf;
    }

    addr->sock.type = mdv_str2socket_type(proto);

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
            struct sockaddr_in *dst_addr = (struct sockaddr_in*)&addr->sock.addr;
            memcpy(dst_addr, src_addr, sizeof *src_addr);
            ret = MDV_OK;
            break;
        }
        else if (!addr->sock.addr.ss_family && ptr->ai_family == AF_INET6)
        {
            struct sockaddr_in6 *src_addr = (struct sockaddr_in6*)ptr->ai_addr;
            struct sockaddr_in6 *dst_addr = (struct sockaddr_in6*)&addr->sock.addr;
            memcpy(dst_addr, src_addr, sizeof *src_addr);
            ret = MDV_OK;
        }
    }

    freeaddrinfo(result);

    return ret;
}


char const * mdv_netaddr2str(mdv_netaddr const *addr, char *buf, size_t size)
{
    if (!addr || !buf || !size)
    {
        MDV_LOGE("Invalid argument");
        return 0;
    }

    buf[0] = 0;

    unsigned addr_len = 0;

    char const *proto = mdv_socket_type2str(addr->sock.type);

    if (proto)
    {
        snprintf(buf, size, "%s://", proto);
        addr_len += strlen(proto) + 3;
    }

    int err = getnameinfo((struct sockaddr const *)&addr->sock.addr, sizeof addr->sock.addr,
                          buf + addr_len, size - addr_len,
                          0, 0,
                          NI_NUMERICHOST | NI_NUMERICSERV);

    if (err)
        return 0;

    addr_len = strlen(buf);

    if (addr_len < size)
        snprintf(buf + addr_len, size - addr_len, ":%u", ntohs(((struct sockaddr_in const *)&addr->sock.addr)->sin_port));

    return buf;
}
