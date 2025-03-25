#include <stdio.h>
#include <stdlib.h>
#include "../include/cache_management.h"

void update_cache(int doc_index) {
    // Update access statistics
    documents[doc_index].access_count++;
    documents[doc_index].last_access = global_time++;
    
    // If document is not in cache, try to add it
    if (!documents[doc_index].in_cache) {
        // Count documents in cache
        int in_cache_count = 0;
        for (int i = 0; i < num_documents; i++) {
            if (documents[i].in_cache) {
                in_cache_count++;
            }
        }
        
        if (in_cache_count < cache_size) {
            // We have room in the cache
            documents[doc_index].in_cache = 1;
        } else {
            // Cache is full, replace least recently used
            int lru_index = -1;
            int min_access_time = global_time;
            
            for (int i = 0; i < num_documents; i++) {
                if (documents[i].in_cache && documents[i].last_access < min_access_time) {
                    min_access_time = documents[i].last_access;
                    lru_index = i;
                }
            }
            
            if (lru_index >= 0) {
                documents[lru_index].in_cache = 0;
                documents[doc_index].in_cache = 1;
            }
        }
    }
}

void initialize_cache() {
    // Initialize cache with empty state
    printf("Initializing cache with size: %d\n", cache_size);
}

void adjust_cache_after_load() {
    // Adjust cache to respect cache_size
    int in_cache_count = 0;
    for (int i = 0; i < num_documents; i++) {
        if (documents[i].in_cache) {
            in_cache_count++;
        }
    }
    
    // If we have more documents in cache than allowed, remove some
    if (in_cache_count > cache_size) {
        // Sort by last_access (ascending)
        for (int i = 0; i < num_documents - 1; i++) {
            for (int j = 0; j < num_documents - i - 1; j++) {
                if (documents[j].in_cache && documents[j+1].in_cache && 
                    documents[j].last_access > documents[j+1].last_access) {
                    // Swap
                    Document temp = documents[j];
                    documents[j] = documents[j+1];
                    documents[j+1] = temp;
                }
            }
        }
        
        // Remove excess documents from cache
        for (int i = 0; i < num_documents && in_cache_count > cache_size; i++) {
            if (documents[i].in_cache) {
                documents[i].in_cache = 0;
                in_cache_count--;
            }
        }
    }
}

