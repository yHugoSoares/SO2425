#ifndef CACHE_MANAGEMENT_H
#define CACHE_MANAGEMENT_H

#include "common.h"

// Cache management function prototypes
void update_cache(int doc_index);
void initialize_cache();
void adjust_cache_after_load();

// External variables that need to be accessed
extern Document *documents;
extern int num_documents;
extern int cache_size;
extern int global_time;

#endif /* CACHE_MANAGEMENT_H */
