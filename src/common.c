// filepath: src/common.c
#include "common.h"
#include <stddef.h>


// Global variables
char document_folder[256];
int cache_size;
Document *documents = NULL;
int num_documents = 0;
int next_key = 1;
int server_running = 1;
int global_time = 0; // For LRU cache policy
