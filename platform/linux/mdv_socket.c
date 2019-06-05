#include "../mdv_socket.h"
#include "../mdv_log.h"
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


bool mdv_str2addr(mdv_string_t const str, mdv_address_t *addr)
{
    if (mdv_str_is_empty(str) || !addr)
    {
        MDV_LOGE("str2addr: Invalid argument");
        return false;
    }

    memset(addr, 0, sizeof *addr);

    char host[str.length + 1];
    strncpy(host, str.data, str.length + 1);

    char *port = strrchr(host, ':');
    if (port) *(port++) = 0;

    struct addrinfo *result = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int err = getaddrinfo(host, port, &hints, &result);
    if (err)
    {
        MDV_LOGE("getaddrinfo: failed with error '%s'", gai_strerror(err));
        return false;
    }

    bool ret = false;

    for(struct addrinfo *ptr = result; ptr; ptr = ptr->ai_next)
    {
        if (ptr->ai_family == AF_INET)
        {
            struct sockaddr_in *src_addr = (struct sockaddr_in*)ptr->ai_addr;
            struct sockaddr_in *dst_addr = (struct sockaddr_in*)addr;
            memcpy(dst_addr, src_addr, sizeof *src_addr);
            ret = true;
            break;
        }
        else if (!addr->ss_family && ptr->ai_family == AF_INET6)
        {
            struct sockaddr_in6 *src_addr = (struct sockaddr_in6*)ptr->ai_addr;
            struct sockaddr_in6 *dst_addr = (struct sockaddr_in6*)addr;
            memcpy(dst_addr, src_addr, sizeof *src_addr);
            ret = true;
        }
    }

    freeaddrinfo(result);

    return ret;
}


bool mdv_addr2str(mdv_address_t const *addr, mdv_string_t *str)
{
    if (mdv_str_is_empty(*str) || !addr)
    {
        MDV_LOGE("addr2str: Invalid argument");
        return false;
    }

    if (getnameinfo((struct sockaddr const *)addr, sizeof *addr, str->data, str->length, 0, 0, NI_NUMERICHOST | NI_NUMERICSERV) != 0)
    {
        MDV_LOGE("getnameinfo: could not resolve hostname");
        return false;
    }

    unsigned addr_len = strlen(str->data);
    if (addr_len + 7 <= str->length)     // length(':XXXXX\0') == 7
    {
        int len = snprintf(str->data + addr_len, str->length - addr_len, ":%u", ntohs(((struct sockaddr_in const *)addr)->sin_port));
        if (len < 0)
        {
            MDV_LOGE("addr2str: not enough space for address");
            return false;
        }
        str->length = addr_len + len;
        return true;
    }
    else
    {
        MDV_LOGE("addr2str: not enough space for address");
        return false;
    }

    return false;
}
