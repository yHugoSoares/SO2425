#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <string.h>
#include <time.h>

#include "index.h"
#include "cache.h"
#include "common.h"


// Global cache instance
Cache *global_cache = NULL;

// Add this forward declaration at the top of the file
void cache_page_destroy(CachePage *page);

// Initialize the cache
int cache_init(int cache_size) {
    if (cache_size <= 0) {
        fprintf(stderr, "Cache size must be greater than 0\n");
        return -1;
    }

    if (global_cache != NULL) {
        fprintf(stderr, "Cache already initialized\n");
        return -1;
    }

    global_cache = malloc(sizeof(Cache));
    if (!global_cache) {
        perror("Failed to allocate cache");
        return -1;
    }

    global_cache->pages = calloc(cache_size, sizeof(CachePage*));
    if (!global_cache->pages) {
        perror("Failed to allocate cache pages");
        free(global_cache);
        return -1;
    }

    global_cache->hash_table = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, 
        (GDestroyNotify)cache_page_destroy);
        
    global_cache->size = cache_size;
    global_cache->count = 0;
    global_cache->rng = g_rand_new_with_seed(time(NULL));

    return 0;
}

int cache_load_metadata() {
    if (!global_cache) {
        fprintf(stderr, "Cache not initialized\n");
        return -1;
    }

    // Open metadata file for reading
    FILE *file = fopen(METADATA_FILE, "rb");
    if (!file) {
        perror("Failed to open metadata file for reading");
        return -1;
    }

    // Read metadata entries and add them to the cache
    IndexEntry entry;
    while (fread(&entry, sizeof(IndexEntry), 1, file) == 1) {
        IndexEntry *entry_copy = malloc(sizeof(IndexEntry));
        if (!entry_copy) {
            perror("Failed to allocate memory for metadata entry");
            fclose(file);
            return -1;
        }
        memcpy(entry_copy, &entry, sizeof(IndexEntry));
        cache_add_entry(entry_copy->delete_flag, entry_copy, 0);
    }

    fclose(file);
    return 0;
}

// Destroy a cache page
void cache_page_destroy(CachePage *page) {
    if (!page) return;
    
    if (page->dirty) {
        index_write_dirty_entry(page->entry, page->key);
    }
    
    if (page->entry) {
        destroy_index_entry(page->entry);
    }
    
    free(page);
}

// Get a random page index to evict
int cache_get_eviction_index() {
    return g_rand_int_range(global_cache->rng, 0, global_cache->count);
}

// Add entry to cache
int cache_add_entry(int key, IndexEntry *entry, int dirty) {
    if (!global_cache || global_cache->count >= global_cache->size) {
        fprintf(stderr, "Cache full or not initialized\n");
        return -1;
    }

    // Check if key already exists
    if (g_hash_table_contains(global_cache->hash_table, &key)) {
        fprintf(stderr, "Key %d already exists in cache\n", key);
        return -1;
    }

    CachePage *page = malloc(sizeof(CachePage));
    if (!page) {
        perror("Failed to allocate cache page");
        return -1;
    }

    page->key = key;
    page->dirty = dirty;
    page->entry = entry;

    // Add to hash table
    g_hash_table_insert(global_cache->hash_table, &page->key, page);

    // Add to pages array
    global_cache->pages[global_cache->count++] = page;
    return 0;
}

// Get entry from cache
IndexEntry *cache_get_entry(int key) {
    if (!global_cache) {
        fprintf(stderr, "Cache not initialized\n");
        return NULL;
    }

    CachePage *page = g_hash_table_lookup(global_cache->hash_table, &key);
    if (page) {
        return page->entry;
    }

    // Cache miss - load from index
    IndexEntry *entry = index_get_entry(key);
    if (!entry) {
        return NULL;
    }

    // Add to cache if possible
    if (global_cache->count < global_cache->size) {
        cache_add_entry(key, entry, 0);
    }
    
    return entry;
}

// Evict an entry from cache
int cache_evict_entry() {
    if (!global_cache || global_cache->count == 0) {
        return -1;
    }

    int idx = cache_get_eviction_index();
    CachePage *page = global_cache->pages[idx];

    // Remove from hash table
    g_hash_table_remove(global_cache->hash_table, &page->key);

    // Compact pages array
    global_cache->pages[idx] = global_cache->pages[--global_cache->count];
    global_cache->pages[global_cache->count] = NULL;

    cache_page_destroy(page);
    return 0;
}

// Delete an entry
int cache_delete_entry(int key) {
    if (!global_cache) {
        return -1;
    }

    CachePage *page = g_hash_table_lookup(global_cache->hash_table, &key);
    if (page) {
        page->entry->delete_flag = 1;
        page->dirty = 1;
        return 0;
    }

    return index_delete_entry(key);
}

// Cleanup cache
void cache_cleanup() {
    if (!global_cache) return;

    g_hash_table_destroy(global_cache->hash_table);
    g_rand_free(global_cache->rng);
    
    for (int i = 0; i < global_cache->count; i++) {
        cache_page_destroy(global_cache->pages[i]);
    }
    
    free(global_cache->pages);
    free(global_cache);
    global_cache = NULL;
}
