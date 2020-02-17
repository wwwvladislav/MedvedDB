#pragma once
#include "../minunit.h"
#include <mdv_socket.h>


MU_TEST(platform_socket)
{
    char const *str = "tcp://127.0.0.1:12345";

    mdv_socket_type protocol;
    mdv_sockaddr addr;

    mu_check(mdv_str2sockaddr(str, &protocol, &addr) == MDV_OK);

    mu_check(protocol == MDV_SOCK_STREAM);

    char const *tmp = mdv_sockaddr2str(protocol, &addr);

    mu_check(!!tmp);

    mu_check(strcmp(str, tmp) == 0);
}
