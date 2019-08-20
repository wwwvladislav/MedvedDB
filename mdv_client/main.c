#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <mdv_client.h>
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <linenoise.h>


static int help()
{
    printf("NAME\n");
    printf("    mdv - client for distributed, column store, NoSQL database\n\n");
    printf("SYNOPSIS\n");
    printf("    mdv [option...] [protocol://address:port]\n\n");
    printf("OPTIONS\n");
    printf("    -h, --help  Display this help and exit\n");
    printf("    -t, --topo  Show network topology\n");
    printf("    -T, --test  Create test table\n");
    return 0;
}


static int usage()
{
    printf("Usage: mdv protocol://address:port\n");
    printf("Try 'mdv --help' for more information.\n");
    return 0;
}


static int run_test_scenario(char const *addr)
{
    mdv_client_config config =
    {
        .db =
        {
            .addr = addr
        },
        .connection =
        {
            .timeout            = 15,
            .keepidle           = 5,
            .keepcnt            = 10,
            .keepintvl          = 5,
            .response_timeout   = 15
        },
        .threadpool =
        {
            .size = 4
        }
    };

    mdv_client *client = mdv_client_connect(&config);

    if (client)
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

        mdv_errno err = mdv_create_table(client, (mdv_table_base *)&table);

        if (err == MDV_OK)
        {
            printf("New table '%s' with UUID '%s' was successfully created\n", table.name.ptr, mdv_uuid_to_str(&table.uuid).ptr);
        }
        else
        {
            printf("Table '%s' creation failed with error '%s' (%d)\n", table.name.ptr, mdv_strerror(err), err);
        }

        mdv_client_close(client);
    }

    return 0;
}


static int show_topology(char const *addr)
{
    mdv_client_config config =
    {
        .db =
        {
            .addr = addr
        },
        .connection =
        {
            .timeout            = 15,
            .keepidle           = 5,
            .keepcnt            = 10,
            .keepintvl          = 5,
            .response_timeout   = 15
        },
        .threadpool =
        {
            .size = 4
        }
    };

    mdv_client *client = mdv_client_connect(&config);

    if (client)
    {
        size_t links_count = 0;
        mdv_node_link *links = 0;

        mdv_errno err = mdv_get_topology(client, &links_count, &links);

        if (err == MDV_OK)
        {
            // Display topology
            fprintf(stderr, "Usage: neato topology.dot -O -Tpng\n\n");

            printf("graph topology {\n");

            for (size_t i = 0; i < links_count; ++i)
            {
                printf("  \"%s\" -- \"%s\"\n",
                    mdv_uuid_to_str(&links[i].node[0]).ptr,
                    mdv_uuid_to_str(&links[i].node[1]).ptr);
            }

            printf("}\n");

            mdv_free(links, "topology");
        }
        else
            printf("Topology request failed with error '%s' (%d)\n", mdv_strerror(err), err);

        mdv_client_close(client);
    }

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

    struct option long_options[] =
    {
        { "help",   no_argument,    0, 0 },
        { "topo",   no_argument,    0, 0 },
        { "test",   no_argument,    0, 0 },
        { 0,        0,              0, 0 }
    };

    mdv_logf_set_level(ZF_LOG_WARN);

    int option_index = 0;
    int op = 0;

    while((op = getopt_long(argc - 1, argv, "htT", long_options, &option_index)) != -1)
    {
        switch(op)
        {
            case '?':
            case 'h':
                return help();

            case 't':
                return show_topology(argv[argc - 1]);

            case 'T':
                return run_test_scenario(argv[argc - 1]);
        }
    }

    return help();
}
