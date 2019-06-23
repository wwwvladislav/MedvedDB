#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <mdv_client.h>
#include <mdv_log.h>


static int help()
{
    printf("NAME\n");
    printf("    mdv - client for distributed, column store, NoSQL database\n\n");
    printf("SYNOPSIS\n");
    printf("    mdv protocol://address:port\n\n");
    printf("OPTIONS\n");
    printf("    --help display this help and exit\n");
    return 0;
}


static int usage()
{
    printf("Usage: mdv protocol://address:port\n");
    printf("Try 'mdv --help' for more information.\n");
    return 0;
}


int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("mdv: missing operand\n");
        usage();
        return 0;
    }

    static struct option long_options[] =
    {
        { "help",   no_argument,        0, 0 },
        { 0,        0,                  0, 0 }
    };

    int option_index = 0;
    int ret = 0;

    while((ret = getopt_long(argc, argv, "h", long_options, &option_index)) != -1)
    {
        char op = (char)ret;

        if (!op)
        {
            switch(option_index)
            {
                case 0: op = 'h'; break;
            }
        }

        switch(op)
        {
            case 'h':
                return help();
        }
    }

    mdv_logf_set_level(ZF_LOG_WARN);

    mdv_client *client = mdv_client_create(argv[1]);
    if (client)
    {
        mdv_client_connect(client);
        mdv_client_disconnect(client);
    }

    return 0;
}
