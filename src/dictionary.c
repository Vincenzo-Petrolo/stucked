// This is a simple C implementation for a dictionary.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct key_value_pair {
    char *key;
    size_t value;
    struct key_value_pair *next;
    unsigned long hash;
};

struct dictionary {
    size_t capacity;
    size_t occupied;
    struct key_value_pair **buckets;
};


/////////////////
// PROTOTYPES  //
/////////////////

unsigned long hash(const char *str);
static struct dictionary *resize(struct dictionary *dict);
struct dictionary *init(size_t capacity);
struct dictionary *add(struct dictionary *dict, char *key, size_t value);
size_t exist(struct dictionary *dict, const char *key);
size_t get(struct dictionary *dict, const char *key);
void print(struct dictionary *dict);


// This is the djb2 hash function
unsigned long hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

// This function resizes the dictionary doubling the capacity when buckets start
// to become too much filled.
static struct dictionary *resize(struct dictionary *dict) {
    struct dictionary *new_dict = init(dict->capacity*2);

    // Start replaying for all the keys
    for (size_t i = 0; i < dict->capacity; i++) {

        struct key_value_pair *node = dict->buckets[i];

        // Skip if no linked list
        if (!node) continue;

        // Iterate over the linked list
        do {
            new_dict = add(new_dict, node->key, node->value);
            node = node->next;
        } while (node != NULL);
    }

    return new_dict;
}

// This function initializes the dictionary to given capacity. The more the
// capacity, the lesser the collisions.
struct dictionary *init(size_t capacity) {
    struct dictionary *obj = malloc(sizeof(struct dictionary));

    obj->buckets = malloc(sizeof(struct key_value_pair *) * capacity);

    // Initialize to empty linked lists
    for (size_t i = 0; i < capacity; i++) {
        obj->buckets[i] = NULL;
    }

    obj->capacity = capacity;
    obj->occupied = 0;

    return obj;
}

// This function adds a new element to the dictionary. If the key is already
// existing, it updates the value. If aliasing occurs, it appends the new value
// to the linked list.
struct dictionary *add(struct dictionary *dict, char *key, size_t value) {
    unsigned long hashed_key = hash(key);
    size_t idx = hashed_key % dict->capacity;
    struct key_value_pair *new_node;
    struct key_value_pair *last_node;

    struct key_value_pair *node = dict->buckets[idx];

    // In case the bucket is empty. Allocate a new one.
    if (!node) {
        node = malloc(sizeof(struct key_value_pair));
        node->key = strdup(key);
        node->value = value;
        node->next = NULL;
        node->hash = hashed_key;

        dict->buckets[idx] = node;
        dict->occupied++;
        
        // Check if almost full, then resize and return the new dictionary.
        float occupancy = (float)dict->occupied / (float) dict->capacity;
    
        if (occupancy > 0.75) {
           return resize(dict);
        }

        return dict;
    }

    do {
        // Iterate the linked list looking for a match.
        if (strcmp(node->key, key) == 0) {
            // Matched the key, update value and return
            node->value = value;
            return dict;
        }
        
        last_node = node;
        node = node->next;
    } while (node != NULL);

    // If I reach here, create a new node and append it.
    new_node = malloc(sizeof(struct key_value_pair));
    new_node->key = strdup(key);
    new_node->value = value;
    new_node->next = NULL;
    new_node->hash = hashed_key;

    // Update as last node
    last_node->next = new_node;


    return dict;
}

// Check for existence of a given key in dictionary. Returns 0 if not found.
size_t exist(struct dictionary *dict, const char *key) {
    unsigned long hashed_key = hash(key);
    size_t idx = hashed_key % dict->capacity;
    struct key_value_pair *node = NULL;

    // Not even the bucket exists
    if (!dict->buckets[idx]) {
        return 0;
    }

    node = dict->buckets[idx];

    do {
        // Iterate the linked list looking for a match.
        if (strcmp(node->key, key) == 0) {
            // Matched the key, update value and return
            return 1;
        }
        
        node = node->next;
    } while (node != NULL);
    

    return 0;
}

// This function returns the value given a key. It must be checked for
// existence.
size_t get(struct dictionary *dict, const char *key) {
    unsigned long hashed_key = hash(key);
    size_t idx = hashed_key % dict->capacity;
    struct key_value_pair *node = NULL;

    node = dict->buckets[idx];

    do {
        // Iterate the linked list looking for a match.
        if (strcmp(node->key, key) == 0) {
            // Matched the key, update value and return
            return node->value;
        }
        
        node = node->next;
    } while (node != NULL);
}

// This function prints the content of the dictionary.
void print(struct dictionary *dict) {

    // Start replaying for all the keys
    for (size_t i = 0; i < dict->capacity; i++) {

        struct key_value_pair *node = dict->buckets[i];

        // Skip if no linked list
        if (!node) continue;

        // Iterate over the linked list
        do {
            printf("[%s]->%zu\n", node->key, node->value);
            node = node->next;
        } while (node != NULL);
    }
}