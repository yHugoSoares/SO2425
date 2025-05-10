#include "cache.h"
#include "common.h"

Cache *global_cache = NULL;

void cache_init(int size) {
    global_cache = malloc(sizeof(Cache));
    if (!global_cache) {
        fprintf(stderr, "Failed to allocate memory for cache\n");
        return -1;
    }

    global_cache->size = size;
    global_cache->count = 0;
    global_cache->pages = malloc(size * sizeof(CachePage *));
    if (!global_cache->pages) {
        fprintf(stderr, "Failed to allocate memory for cache pages\n");
        free(global_cache);
        return -1;
    }

    for (int i = 0; i < size; i++) {
        global_cache->pages[i] = NULL;
    }

    return 0;
}

void cache_destroy() {
    for (int i = 0; i < global_cache->size; i++) {
        if (global_cache->pages[i]) {
            free(global_cache->pages[i]);
        }
    }
    free(global_cache->pages);
    free(global_cache);
}

int cache_add_entry(int key, Entry entry) {
    if (global_cache->count >= global_cache->size) {
        fprintf(stderr, "Cache is full. Cannot add new entry.\n");
        return -1;
    }

    CachePage *new_page = malloc(sizeof(CachePage));
    if (!new_page) {
        fprintf(stderr, "Failed to allocate memory for new cache page\n");
        return -1;
    }

    new_page->entry = malloc(sizeof(Entry));
    if (!new_page->entry) {
        fprintf(stderr, "Failed to allocate memory for new cache entry\n");
        free(new_page);
        return -1;
    }

    new_page->key = key;
    strcpy(new_page->entry->title, entry.title);
    strcpy(new_page->entry->authors, entry.authors);
    strcpy(new_page->entry->year, entry.year);
    strcpy(new_page->entry->path, entry.path);

    global_cache->pages[global_cache->count] = new_page;
    global_cache->count++;

    return 0;
}
