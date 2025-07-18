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

// Load from file to cache
int cache_load_from_file(const char *filename) {
    if (!global_cache) {
        fprintf(stderr, "Cache not initialized\n");
        return -1;
    }

    // Open the file for reading
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open file for reading");
        return -1;
    }

    // Read entries from the file and add them to the cache
    Entry entry;
    while (read(fd, &entry, sizeof(Entry)) == sizeof(Entry)) {
        // Skip deleted entries
        if (entry.delete_flag) {
            continue;
        }

        // Allocate memory for the entry
        Entry *entry_copy = malloc(sizeof(Entry));
        if (!entry_copy) {
            perror("Failed to allocate memory for entry");
            close(fd);
            return -1;
        }
        memcpy(entry_copy, &entry, sizeof(Entry));

        // Add the entry to the cache
        if (cache_add_entry(global_cache->count, entry_copy, 0) != 0) {
            fprintf(stderr, "Failed to add entry to cache\n");
            free(entry_copy);
        }
    }

    close(fd);
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
    int fd = open(METADATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Failed to open metadata file for eviction");
        return -1;
    }
    write(fd, page->entry, sizeof(Entry));
    close(fd);

    // Remove from hash table
    g_hash_table_remove(global_cache->hash_table, &page->key);

    // Compact pages array
    global_cache->pages[idx] = global_cache->pages[--global_cache->count];
    global_cache->pages[global_cache->count] = NULL;

    cache_page_destroy(page);
    return 0;
}

int cache_delete_entry(int key) {
    if (!global_cache) {
        fprintf(stderr, "Cache not initialized\n");
        return -1;
    }

    // Find the page in the hash table
    CachePage *page = g_hash_table_lookup(global_cache->hash_table, &key);
    if (!page) {
        fprintf(stderr, "Entry with key %d not found in cache\n", key);
        return -1;
    }

    // Remove the page from the hash table
    g_hash_table_remove(global_cache->hash_table, &key);

    // Find and remove the page from the pages array
    for (int i = 0; i < global_cache->count; i++) {
        if (global_cache->pages[i] == page) {
            // Shift the remaining pages to fill the gap
            for (int j = i; j < global_cache->count - 1; j++) {
                global_cache->pages[j] = global_cache->pages[j + 1];
            }
            global_cache->pages[--global_cache->count] = NULL;
            break;
        }
    }

    // Destroy the cache page
    cache_page_destroy(page);

    return 0;
}


// Add entry to cache
int cache_add_entry(int key, Entry *entry, int dirty) {
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

// Get entry from index (cache)
Entry *cache_get_entry(int key) {
    if (!global_cache) {
        fprintf(stderr, "Cache not initialized\n");
        return NULL;
    }

    CachePage *page = g_hash_table_lookup(global_cache->hash_table, &key);
    if (page) {
        return page->entry;
    }

    return NULL;
}

// Load metadata from metadata.dat into cache
int cache_load_metadata() {
    if (!global_cache) {
        fprintf(stderr, "Cache not initialized\n");
        return -1;
    }

    // Open metadata file for reading
    int fd = open(METADATA_FILE, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open metadata file for reading");
        return -1;
    }

    // Read metadata entries and add them to the cache
    Entry entry;
    while (read(fd, &entry, sizeof(Entry)) == sizeof(Entry)) {
        Entry *entry_copy = malloc(sizeof(Entry));
        if (!entry_copy) {
            perror("Failed to allocate memory for metadata entry");
            close(fd);
            return -1;
        }
        memcpy(entry_copy, &entry, sizeof(Entry));
        cache_add_entry(entry_copy->delete_flag, entry_copy, 0);
    }

    close(fd);
    return 0;
}
