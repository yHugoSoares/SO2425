#ifndef CACHE_H
#define CACHE_H

#include "index.h"

typedef struct cache_page {
    int dirty;
    int key;
    IndexEntry *entry;
} CachePage;

typedef struct cache {
    CachePage *pages;
    int size;
    int occupied;
} Cache;

int cache_add_new_entry(IndexEntry *entry);
int cache_add_index_entry(IndexEntry *entry, int key);
IndexEntry *cache_get_entry(int key);
int cache_delete_entry(int key);
int cache_remove_entry(int key);

#endif
