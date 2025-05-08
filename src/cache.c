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

    return 0;
}

// Destroy a cache page
void cache_page_destroy(CachePage *page) {
    if (!page) return;

    if (page->dirty) {
        if (index_write_dirty_entry(page->entry, page->key) != 0) {
            fprintf(stderr, "Failed to write dirty entry for key %d\n", page->key);
        }
    }

    if (page->entry) {
        destroy_index_entry(page->entry);
    }

    free(page);
}

// Evict an entry from cache to metadata.dat
int cache_evict_entry() {
    if (!global_cache || global_cache->count == 0) {
        return -1;
    }

    // Select a random page to evict
    int idx = g_rand_int_range(g_rand_new(), 0, global_cache->count);
    CachePage *page = global_cache->pages[idx];

    // Write the entry back to metadata.dat
    FILE *file = fopen(METADATA_FILE, "ab");
    if (!file) {
        perror("Failed to open metadata file for eviction");
        return -1;
    }
    fwrite(page->entry, sizeof(IndexEntry), 1, file);
    fclose(file);

    // Remove from hash table
    g_hash_table_remove(global_cache->hash_table, &page->key);

    // Compact pages array
    global_cache->pages[idx] = global_cache->pages[--global_cache->count];
    global_cache->pages[global_cache->count] = NULL;

    cache_page_destroy(page);
    return 0;
}

// Add entry to cache
int cache_add_entry(int key, IndexEntry *entry, int dirty) {
    if (!global_cache) {
        fprintf(stderr, "Cache not initialized\n");
        return -1;
    }

    // Evict an entry if the cache is full
    if (global_cache->count >= global_cache->size) {
        if (cache_evict_entry() != 0) {
            fprintf(stderr, "Failed to evict entry from cache\n");
            return -1;
        }
    }

    CachePage *page = malloc(sizeof(CachePage));
    if (!page) {
        perror("Failed to allocate cache page");
        return -1;
    }

    page->key = key;
    page->dirty = dirty;
    page->entry = entry; // Cache takes ownership of the entry

    // Add to hash table
    g_hash_table_insert(global_cache->hash_table, &page->key, page);

    // Add to pages array
    global_cache->pages[global_cache->count++] = page;
    return 0;
}

// Load metadata from metadata.dat into cache
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
