/**
 * This is an implementation of a graph data structure in C written on a hot
 * Sunday in August.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> 
#include "dictionary.c"


struct dag {
    ///////////
    // Data
    ///////////
    size_t val; // Value carried by this node
    char *name; // Name of the node

    ///////////
    // Metadata
    ///////////
    struct dag **neighbors; // Array of pointers to neighbors
    size_t num_neighbors; // Number of neighbors on this node.
};

struct node_searcher {
    char *name;
    struct dag *node;
};

// This function creates a node to be inserted in a DAG.
struct dag* create_node(size_t val, char *name) {
    struct dag* new_node = malloc(1 * sizeof(struct dag));

    // Data
    new_node->name = strdup(name);
    new_node->val = val;
    
    // Metadata
    new_node->neighbors = NULL;
    new_node->num_neighbors = 0;

    // Shalla, do not check malloc fail
    return new_node;
}

// Creates a connect between a source node and a destination node. It returns
// the source node
struct dag* connect(struct dag* src, struct dag* dst) {
    if (!dst) return src;

    src->num_neighbors++;
    src->neighbors = realloc(src->neighbors, src->num_neighbors *  sizeof(struct dag*));

    src->neighbors[src->num_neighbors - 1] = dst;

    return src;
}

// This function recursively traverse the DAG
void traverse_dfs(struct dag* src, void (*fn)(struct dag*, void *), void *params) {
    fn(src, params);

    for (int i = 0; i < src->num_neighbors; i++) {
        traverse_dfs(src->neighbors[i], fn, params);
    }

    return;
}

// This function prints a node
void print_node(struct dag *node, void *) {
    if (!node) return;

    printf("[%s] %zu\n", node->name, node->val);
}

// This function match a node and stores the matched node pointer into matched
void match_node(struct dag *node, void *matched) {

    struct node_searcher *payload = matched;

    if (strncmp(node->name, payload->name, strlen(node->name)) == 0) {
        payload->node = node;
    }

    return;
}


// This function iterates over the graph and finds the longest path that
// generates the largest sum
size_t longest_path(struct dag* src, struct dag **longest_chain) {
    size_t max_sum = 0;
    struct dag *n = NULL;

    for (int i = 0; i < src->num_neighbors; i++) {
        struct dag *node;
        size_t sum = longest_path(src->neighbors[i], &node);
        if ( sum > max_sum) {
            max_sum = sum;
            n = node;
        }
    }

    // Create copy of self and connect it to max sum neighboor
    struct dag *tmp = create_node(0, "");
    tmp->name = src->name; tmp->val = src->val;
    tmp = connect(tmp, n);

    // Pass new chain to top
    *longest_chain = tmp;
    
    return src->val + max_sum;
}

struct dag *search_node(struct dag *root, char *name) {
    struct node_searcher s;

    s.name = strdup(name);
    s.node = NULL;

    traverse_dfs(root, match_node, &s);

    return s.node;
}

// This function reads the function name from a string formatted as
// rt_hal_vpu_init() <rt_hal_vpu_err_t rt_hal_vpu_init (void) at
// rt_hal_vpu.c:34>: Extracting only the initial function name and removing the
// trailing spaces. It returns the number of trailing spaces and in fn_name it
// returns the parsed fn_name
size_t parse_fn_name(char *string, char **fn_name) {
    size_t spaces = 0;
    size_t fn_name_length = 0;


    // Consume whitespaces
    while (string[spaces] == ' ') {
        spaces++;
    }

    // Count the length for the string
    while (string[spaces + fn_name_length] != '(') {
        fn_name_length++;
    }

    // Now that I know the length of the string I can allocate it, +1 for \0
    *fn_name = malloc(fn_name_length + 1);

    // Copy the content
    strncpy(*fn_name, string+spaces, fn_name_length);

    // Add \0
    (*fn_name)[fn_name_length] = 0;

    return spaces;
}


void populate_dag(FILE *fp, struct dag *parent, int spaces) {
    char buf[500];

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        // printf("[DEBUG] Read %s of length %d\n", buf, strlen(buf));
        // Parse the string
        char *fn_name;
        int curr_spaces = parse_fn_name(buf, &fn_name);

        // I've found a child of this parent.
        if (curr_spaces > spaces) {
            struct dag *node = create_node(0, fn_name);
            parent = connect(parent, node);

            // printf("[DEBUG] Found a child for: "); print_node(parent, NULL);
            // printf("[DEBUG] The child in question: "); print_node(node, NULL);

            // Recursively call for the child
            populate_dag(fp, node, curr_spaces);
        } else {
            // Done recursing, give back control to the parent restoring the fp
            fseek(fp, -strlen(buf), SEEK_CUR);

            // printf("[DEBUG] Done recursing, going back of %d\n", strlen(buf));
            // printf("[DEBUG] For node: "); print_node(parent, NULL);

            return;
        }
    }


}

void update_w(struct dag *node, void *param) {
    struct dictionary *dict = (struct dictionary *) param;

    if (exist(dict, node->name)) {
        node->val = get(dict, node->name);
    }
}

// This function reads a file and parses it as a DAG, it can create duplicated
// nodes. I don't care.
struct dag *read_file(char *filename, struct dictionary *dict) {
    struct dag *root = NULL;
    FILE *fp = NULL;
    char buf[500];


    fp = fopen(filename, "r");

    if (!fp) {
        return root;
    }

    // Read the first line
    fgets(buf, sizeof(buf), fp);

    char *root_name;

    parse_fn_name(buf, &root_name);

    // Create the root node
    root = create_node(0, root_name);

    // Populate DAG
    populate_dag(fp, root, 0);


    // Traverse the DAG and update the weights
    traverse_dfs(root, update_w, dict);

    return root;
}