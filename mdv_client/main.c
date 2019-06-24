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
        if (mdv_client_connect(client))
        {
            mdv_table(3) table =
            {
                .name = mdv_str_static("MyTable"),
                .size = 3,
                .fields =
                {
                    { MDV_FLD_TYPE_CHAR,  0, mdv_str_static("Col1") },  // char *
                    { MDV_FLD_TYPE_INT32, 2, mdv_str_static("Col2") },  // pair { int, int }
                    { MDV_FLD_TYPE_BOOL,  1, mdv_str_static("Col3") }   // bool
                }
            };

            if (mdv_create_table(client, (mdv_table_base *)&table))
            {
                mdv_uuid_str(uuid);
                mdv_uuid_to_str(&table.uuid, &uuid);
                printf("New table '%s' with UUID '%s' was successfully created\n", uuid.ptr, table.name.ptr);
            }
            else
            {
                printf("Unable to create table '%s' due the error '%s' (%d)\n", table.name.ptr, mdv_client_status_msg(client), mdv_client_errno(client));
            }
        }
        mdv_client_disconnect(client);
    }

    return 0;
}
