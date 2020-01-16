#include "mdv_cout.h"
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <mdv_client.h>
#include <mdv_log.h>
#include <mdv_limits.h>
#include <mdv_alloc.h>
#include <stdlib.h>
#include <linenoise.h>
#include <ctype.h>
#include <inttypes.h>


static char const *MDV_HISTORY      = "history.txt";
static int const   MDV_HISTORY_LEN  = 64;


static mdv_client *client = 0;


typedef void (*mdv_command_handler)(char const *);


typedef struct
{
    char const          *cmd;
    char const          *descriprion;
    mdv_command_handler  handler;
} mdv_command;


static void mdv_commands_list(char const *);
static void mdv_redirect_output(char const *);
static void mdv_show_topology(char const *);
static void mdv_show_routes(char const *);
static void mdv_show_tables(char const *);
static void mdv_test_scenario(char const *);


static mdv_command const commands[] =
{
    { "\\?",    "Show all available mdv commands",          &mdv_commands_list },
    { "\\o",    "Save an output of query to a text file",   &mdv_redirect_output },
    { "\\t",    "Show network topology",                    &mdv_show_topology },
    { "\\r",    "Show routing table",                       &mdv_show_routes },
    { "\\dt",   "Show tables in the current database",      &mdv_show_tables },
    { "\\q",    "Quit mdv",                                 0 },
    { "\\test", "Run test scenario",                        &mdv_test_scenario },
};


static mdv_command const * mdv_command_find(char const *cmd, size_t cmd_len)
{
    for (unsigned int i = 0; i < sizeof commands / sizeof *commands; ++i)
    {
        if (strncmp(commands[i].cmd, cmd, cmd_len) == 0)
            return &commands[i];
    }

    return 0;
}


static void mdv_command_completion(const char *buf, linenoiseCompletions *lc)
{
    size_t const buf_len = strlen(buf);

    for (unsigned int i = 0; i < sizeof commands / sizeof *commands; ++i)
    {
        if (strncmp(buf, commands[i].cmd, buf_len) == 0)
            linenoiseAddCompletion(lc, commands[i].cmd);
    }
}


static int mdv_help()
{
    MDV_INF("NAME\n");
    MDV_INF("    mdv - client for distributed, column store, NoSQL database\n\n");
    MDV_INF("SYNOPSIS\n");
    MDV_INF("    mdv [option...] [protocol://address:port]\n\n");
    MDV_INF("OPTIONS\n");
    MDV_INF("    -h, --help  Display this help and exit\n");
    return 0;
}


static int mdv_usage()
{
    MDV_INF("Usage: mdv protocol://address:port\n");
    MDV_INF("Try 'mdv --help' for more information.\n");
    return 0;
}


static void mdv_commands_list(char const *args)
{
    (void)args;
    for (unsigned int i = 0; i < sizeof commands / sizeof *commands; ++i)
        MDV_OUT("%s \t %s\n", commands[i].cmd, commands[i].descriprion);
}


static void mdv_redirect_output(char const *args)
{
    if (!args)
        result_file_path[0] = 0;
    else
    {
        strncpy(result_file_path, args, sizeof result_file_path);
        result_file_path[sizeof result_file_path - 1] = 0;

        FILE *f = fopen(result_file_path, "w");
        if (f) fclose(f);
    }
}


static void mdv_show_topology(char const *args)
{
    (void)args;

    mdv_topology *topology = 0;

    mdv_errno err = mdv_get_topology(client, &topology);

    if (err == MDV_OK && topology)
    {
        // Display topology
        MDV_INF("Usage: sfdp topology.dot -O -Tpng\n\n");

        MDV_OUT("graph topology {\n");

        mdv_vector *toponodes = mdv_topology_nodes(topology);
        mdv_vector *topolinks = mdv_topology_links(topology);

        mdv_vector_foreach(topolinks, mdv_topolink, link)
        {
            mdv_toponode const *src_node = mdv_vector_at(toponodes, link->node[0]);
            mdv_toponode const *dst_node = mdv_vector_at(toponodes, link->node[1]);
            MDV_OUT("  \"%s\" -- ", mdv_uuid_to_str(&src_node->uuid).ptr);
            MDV_OUT("\"%s\"\n", mdv_uuid_to_str(&dst_node->uuid).ptr);
        }

        MDV_OUT("}\n");

        mdv_vector_release(toponodes);
        mdv_vector_release(topolinks);

        mdv_topology_release(topology);
    }
    else
        MDV_INF("Topology request failed with error '%s' (%d)\n", mdv_strerror(err), err);
}


static void mdv_show_routes(char const *args)
{
    (void)args;

    mdv_hashmap *routes = mdv_get_routes(client);

    if (routes)
    {
        // Display routes
        MDV_OUT("graph routes {\n");

        mdv_hashmap_foreach(routes, mdv_uuid, uuid)
        {
            MDV_OUT("  \"%s\"\n", mdv_uuid_to_str(uuid).ptr);
        }

        MDV_OUT("}\n");

        mdv_hashmap_release(routes);
    }
    else
        MDV_INF("Routing table request failed\n");
}


static void mdv_show_tables(char const *args)
{
    (void)args;

    mdv_table *table = mdv_get_table(client, &MDV_SYSTBL_TABLES);

    if (table)
    {
        mdv_bitset *mask = mdv_bitset_create(mdv_table_description(table)->size, &mdv_default_allocator);

        if (mask)
        {
            mdv_bitset_fill(mask, true);

            mdv_table *result_table = 0;

            mdv_enumerator *enumerator = mdv_select(client, table, mask, "", &result_table);

            if (enumerator)
            {
                mdv_cout_table(result_table, enumerator);
                mdv_table_release(result_table);
                mdv_enumerator_release(enumerator);
            }
            else
                MDV_INF("Rows iteration failed\n");

            mdv_bitset_release(mask);
        }
        else
            MDV_INF("Rows iteration failed. No memory for fields mask.\n");

        mdv_table_release(table);
    }
}


static void mdv_test_scenario(char const *args)
{
    (void)args;

    // Table description

    mdv_field const fields[] =
    {
        { MDV_FLD_TYPE_CHAR,  0, mdv_str_static("Col1") },  // char *
        { MDV_FLD_TYPE_INT32, 2, mdv_str_static("Col2") },  // pair { int, int }
        { MDV_FLD_TYPE_BOOL,  1, mdv_str_static("Col3") }   // bool
    };

    mdv_table_desc desc =
    {
        .name = mdv_str_static("MyTable"),
        .size = 3,
        .fields = fields
    };

    // Table creation

    mdv_table *table = mdv_create_table(client, &desc);

    if (!table)
    {
        MDV_INF("Table '%s' creation failed\n", desc.name.ptr);
        return;
    }

    MDV_INF("New table '%s' with ID '%s' successfully created\n", desc.name.ptr, mdv_uuid_to_str(mdv_table_uuid(table)).ptr);

    // Rowset creation

    mdv_rowset *rowset = mdv_rowset_create(desc.size);

    if (!rowset)
    {
        MDV_INF("Rowset creation failed for table '%s'\n", desc.name.ptr);
        mdv_table_release(table);
        return;
    }

    int ints1[] = { 42, 43 };
    int ints2[] = { 44, 45 };
    bool bool_value_0 = true;
    bool bool_value_1 = false;

    mdv_data const row0[] =
    {
        { 5,             "hello" },
        { sizeof(ints1), ints1 },
        { 1,             &bool_value_0 }
    };

    mdv_data const row1[] =
    {
        { 5,             "world" },
        { sizeof(ints2), ints2 },
        { 1,             &bool_value_1 }
    };

    mdv_data const *rows[] = { row0, row1 };

    if (mdv_rowset_append(rowset, rows, 2) != 2)
    {
        MDV_INF("Rowset creation failed for table '%s'\n", desc.name.ptr);
        mdv_rowset_release(rowset);
        mdv_table_release(table);
        return;
    }

    // Data insertion into the table

    mdv_errno err = mdv_insert_rows(client, table, rowset);

    if (err == MDV_OK)
        MDV_INF("New row was successfully inserted\n");
     else
        MDV_INF("New row insertion failed with error '%s' (%d)\n", mdv_strerror(err), err);

    mdv_rowset_release(rowset);

    // Read data from the table

    mdv_bitset *mask = mdv_bitset_create(desc.size, &mdv_default_allocator);

    if (mask)
    {
        mdv_bitset_fill(mask, true);

        mdv_table *result_table = 0;

        mdv_enumerator *enumerator = mdv_select(client, table, mask, "", &result_table);

        if (enumerator)
        {
            mdv_cout_table(result_table, enumerator);
            mdv_table_release(result_table);
            mdv_enumerator_release(enumerator);
        }
        else
            MDV_INF("Rows iteration failed\n");

        mdv_bitset_release(mask);
    }
    else
        MDV_INF("Rows iteration failed. No memory for fields mask.\n");

    mdv_table_release(table);
}


static mdv_client * mdv_connect(char const *addr)
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
            .response_timeout   = 5
        },
        .threadpool =
        {
            .size = 4
        }
    };

    return mdv_client_connect(&config);
}


int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        MDV_INF("mdv: missing operand\n");
        mdv_usage();
        return 0;
    }

    struct option long_options[] =
    {
        { "help",   no_argument,    0, 0 },
        { 0,        0,              0, 0 }
    };

    mdv_logf_set_level(ZF_LOG_WARN);

    int option_index = 0;
    int op = 0;

    while((op = getopt_long(argc - 1, argv, "h", long_options, &option_index)) != -1)
    {
        switch(op)
        {
            case '?':
            case 'h':
                return mdv_help();
        }
    }

    linenoiseSetMultiLine(1);
    linenoiseHistoryLoad(MDV_HISTORY);
    linenoiseHistorySetMaxLen(MDV_HISTORY_LEN);
    linenoiseSetCompletionCallback(mdv_command_completion);

    client = mdv_connect(argv[argc - 1]);

    if (client)
    {
        char *line;

        while((line = linenoise("mdv> ")))
        {
            if (*line)
            {
                char const *args = strchr(line, ' ');
                size_t const cmd_len = args ? (size_t)(args - line) : strlen(line);
                while(args && *args && isspace(*args))
                    ++args;

                mdv_command const *cmd = mdv_command_find(line, cmd_len);

                if (cmd)
                {
                    if (!cmd->handler)
                        break;

                    cmd->handler(args);
                    linenoiseHistoryAdd(line);
                    linenoiseHistorySave(MDV_HISTORY);
                }
            }

            free(line);
        }

        mdv_client_close(client);
    }

    return 0;
}
