/**
 * @file mdv_netaddr.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Network address
 * @version 0.1
 * @date 2020-05-09
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_def.h>

#ifdef MDV_PLATFORM_LINUX
    #include <sys/socket.h>         // sockaddr_storage
#elif defined(MDV_PLATFORM_WINDOWS)
     #include <ws2def.h>            // sockaddr_storage
#endif


typedef enum
{
    MDV_SOCK_STREAM = 0,
    MDV_SOCK_DGRAM,
    MDV_SOCK_UNKNOWN
} mdv_socket_type;


typedef struct sockaddr_storage mdv_sockaddr;


/// Network address
typedef struct
{
    struct
    {
        mdv_socket_type type;   ///< Socket type
        mdv_sockaddr    addr;   ///< Network address
    } sock;
} mdv_netaddr;


mdv_errno      mdv_str2netaddr(char const *str, mdv_netaddr *addr);
char const *   mdv_netaddr2str(mdv_netaddr const *addr, char *buf, size_t size);
