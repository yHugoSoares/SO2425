#ifndef CACHE_H
#define CACHE_H

#include "common.h"

typedef struct CacheNode {
    Document doc;
    struct CacheNode *prev;
    struct CacheNode *next;
} CacheNode;

typedef struct {
    CacheNode *head;
    CacheNode *tail;
    int size;
    int capacity;
} LRUCache;

void init_cache(int capacity);
Document* get_from_cache(int doc_id);
void add_to_cache(Document doc);
void free_cache();

#endif