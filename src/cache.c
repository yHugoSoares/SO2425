#include <stdlib.h>
#include "cache.h"

LRUCache cache;

void init_cache(int capacity) {
    cache.head = NULL;
    cache.tail = NULL;
    cache.size = 0;
    cache.capacity = capacity;
}

void move_to_head(CacheNode *node) {
    if (node == cache.head) return;
    
    // Remove from current position
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    if (node == cache.tail) cache.tail = node->prev;
    
    // Add to head
    node->prev = NULL;
    node->next = cache.head;
    if (cache.head) cache.head->prev = node;
    cache.head = node;
    if (!cache.tail) cache.tail = node;
}

Document* get_from_cache(int doc_id) {
    CacheNode *node = cache.head;
    while (node) {
        if (node->doc.id == doc_id) {
            move_to_head(node);
            return &node->doc;
        }
        node = node->next;
    }
    return NULL;
}

void add_to_cache(Document doc) {
    // Check if already in cache
    if (get_from_cache(doc.id)) return;
    
    // Create new node
    CacheNode *new_node = malloc(sizeof(CacheNode));
    new_node->doc = doc;
    new_node->prev = NULL;
    new_node->next = cache.head;
    
    // Update links
    if (cache.head) cache.head->prev = new_node;
    cache.head = new_node;
    if (!cache.tail) cache.tail = new_node;
    
    // Check capacity
    if (cache.size == cache.capacity) {
        CacheNode *to_remove = cache.tail;
        cache.tail = to_remove->prev;
        if (cache.tail) cache.tail->next = NULL;
        free(to_remove);
    } else {
        cache.size++;
    }
}

void free_cache() {
    CacheNode *current = cache.head;
    while (current) {
        CacheNode *next = current->next;
        free(current);
        current = next;
    }
    cache.head = NULL;
    cache.tail = NULL;
    cache.size = 0;
}
