#ifndef CACHE_H
#define CACHE_H

#include <glib.h>

#include "index.h"

typedef struct cache_page {
    int dirty;
    int key;
    Entry *entry;
} CachePage;

typedef struct cache {
    CachePage **pages;
    int size;
    int occupied;
    int count;
    GHashTable *hash_table;
    GRand *rng;
} Cache;

int cache_init(int cache_size);
int cache_load_from_file(const char *filename);
int cache_load_metadata();
int cache_add_entry(int key, Entry *entry, int dirty);
int cache_add_new_entry(Entry *entry);
int cache_add_index_entry(Entry *entry, int key);
Entry *cache_get_entry(int key);
int cache_delete_entry(int key);
int cache_remove_entry(int key);

#endif
