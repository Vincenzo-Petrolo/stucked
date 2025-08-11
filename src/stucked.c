/*
 * stucked — worst-case stack analyzer (CLI-polished)
 *
 * This refactors only the command-line interface and main routine,
 * leaving the graph and dictionary data structures & implementations intact.
 *
 * Build: use the provided configure && make
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include "graph.c"   /* NOTE: intentionally include .c to respect original structure */
                    /* graph.c itself includes dictionary.c */

/* --------- Existing helpers kept from original main --------- */
size_t parse_line(char *string, char **key) {
    size_t idx = 0;
    size_t fn_name_length = 0;
    size_t dp_cnt = 0;
    size_t val = 0;

    while (dp_cnt < 3) {
        if (string[idx] == ':') dp_cnt++;
        idx++;
    }

    // Get the length of the function name
    while (string[idx + fn_name_length] != '\t') {
        fn_name_length++;
    }

    // Now create a new string
    *key = malloc(fn_name_length * sizeof(char) + 1);
    strncpy(*key, string + idx, fn_name_length);

    // Now read the value
    sscanf(string + idx + fn_name_length, "\t%zu", &val);

    return val;
}

struct dictionary *file_parse(struct dictionary *dict, const char *filename) {
    FILE *fp = fopen(filename, "r");
    char string[1000];

    if (!fp) return dict;

    while(fgets(string, sizeof(string), fp) != NULL) {
        char *key;
        size_t val;
        val = parse_line(string, &key);
        add(dict, key, val);
    }


    return dict;
}

/* --------- New helpers for CLI & output --------- */

#ifndef STUCKED_VERSION
#define STUCKED_VERSION "1.0.0"
#endif

static void print_usage(const char *prog) {
    printf(
        "Usage: %s [OPTIONS] <callgraph.txt> [stackmap1.txt ...]\n"
        "\n"
        "Analyze the worst-case stack usage along a path in the call graph.\n"
        "\n"
        "Positional arguments:\n"
        "  callgraph.txt         Call graph file understood by read_file()\n"
        "  stackmap*.txt         One or more files with function:stacksize mappings\n"
        "\n"
        "Options:\n"
        "  -p, --print-path      Also print the function chain for the worst case (default)\n"
        "  -q, --quiet           Only print the maximum stack usage value\n"
        "  -j, --json            Output JSON (includes max and path)\n"
        "  -v, --verbose         Verbose logging\n"
        "  -V, --version         Show version and exit\n"
        "  -h, --help            Show this help and exit\n",
        prog
    );
}

static void print_version(void) {
    printf("stucked %s\n", STUCKED_VERSION);
}

/* collect_node: callback for traverse_dfs to collect node names into an array */
struct name_list {
    const char **items;
    size_t count;
    size_t cap;
};

static void name_list_push(struct name_list *nl, const char *s) {
    if (!nl) return;
    if (nl->count == nl->cap) {
        size_t ncap = nl->cap ? nl->cap * 2 : 16;
        const char **nitems = (const char**)realloc(nl->items, ncap * sizeof(*nitems));
        if (!nitems) return; /* best-effort */
        nl->items = nitems;
        nl->cap = ncap;
    }
    nl->items[nl->count++] = s;
}

static void collect_node(struct dag *node, void *userdata) {
    struct name_list *nl = (struct name_list*)userdata;
    if (node && node->name) name_list_push(nl, node->name);
}

/* --------- Main --------- */

int main(int argc, char *argv[]) {
    int opt;
    int option_index = 0;
    bool verbose = false;
    bool quiet = false;
    bool json = false;
    bool print_path = true;

    static struct option long_opts[] = {
        { "help",       no_argument,       0, 'h' },
        { "version",    no_argument,       0, 'V' },
        { "verbose",    no_argument,       0, 'v' },
        { "quiet",      no_argument,       0, 'q' },
        { "json",       no_argument,       0, 'j' },
        { "print-path", no_argument,       0, 'p' },
        { 0,            0,                 0,  0  }
    };

    while ((opt = getopt_long(argc, argv, "hVvqjp", long_opts, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'V':
                print_version();
                return 0;
            case 'v':
                verbose = true;
                break;
            case 'q':
                quiet = true;
                break;
            case 'j':
                json = true;
                break;
            case 'p':
                print_path = true;
                break;
            default:
                print_usage(argv[0]);
                return 2;
        }
    }

    int positional = argc - optind;
    if (positional < 1) {
        fprintf(stderr, "error: missing callgraph file.\n\n");
        print_usage(argv[0]);
        return 2;
    }

    const char *graph_file = argv[optind++];
    int n_stackmaps = argc - optind;

    if (verbose) {
        fprintf(stderr, "graph file   : %s\n", graph_file);
        fprintf(stderr, "stackmaps    : %d file(s)\n", n_stackmaps);
    }

    /* Initialize dictionary and load stack maps (if any) */
    struct dictionary *dict = init(8);
    for (int i = 0; i < n_stackmaps; ++i) {
        const char *path = argv[optind + i];
        if (verbose) fprintf(stderr, "parsing stack map: %s\n", path);
        dict = file_parse(dict, path);
    }

    /* Build graph from file */
    if (verbose) fprintf(stderr, "reading call graph: %s\n", graph_file);
    struct dag *graph = read_file((char*)graph_file, dict);
    if (!graph) {
        fprintf(stderr, "error: failed to read call graph '%s'\n", graph_file);
        return 1;
    }

    /* Compute longest path */
    struct dag *lp = NULL;
    size_t max_stack = longest_path(graph, &lp);

    if (json) {
        /* Build a JSON array of names by DFS traversal of the path */
        struct name_list nl = {0};
        traverse_dfs(lp, collect_node, &nl);
        printf("{\"max_stack\": %zu, \"path\":[", max_stack);
        for (size_t i = 0; i < nl.count; ++i) {
            printf("%s\"%s\"", (i ? "," : ""), nl.items[i] ? nl.items[i] : "");
        }
        printf("]}\n");
        free(nl.items);
        return 0;
    }

    if (!quiet) {
        printf("The maximum stack usage is: %zu\n", max_stack);
        if (print_path) {
            traverse_dfs(lp, print_node, NULL);
        }
    } else {
        /* quiet: only the number */
        printf("%zu\n", max_stack);
    }

    return 0;
}
