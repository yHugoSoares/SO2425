// filepath: includes/common.h
#ifndef COMMON_H
#define COMMON_H

// Constants for communication
#define REQUEST_PIPE "/tmp/dserver_request_pipe"
#define RESPONSE_PIPE "/tmp/dserver_response_pipe"
#define METADATA_FILE "/tmp/dserver_metadata.dat"

// Operation codes
#define OP_ADD 1
#define OP_QUERY 2
#define OP_DELETE 3
#define OP_LINE_COUNT 4
#define OP_SEARCH 5
#define OP_SEARCH_PARALLEL 6
#define OP_SHUTDOWN 7

// Status codes
#define STATUS_OK 0
#define STATUS_ERROR 1

// Document structure
typedef struct {
    int key;
    char *title;
    char *authors;
    int year;
    char *path;
    int in_cache; // Flag to indicate if document is in cache
    int access_count; // For cache replacement policy
    int last_access; // For LRU cache policy
} Document;

// Request structure
typedef struct {
    int operation;
    char title[200];
    char authors[200];
    char year[5];
    char path[64];
    char key[20];
    char keyword[100];
    int nr_processes;
} Request;

// Response structure
typedef struct {
    int status;
    char message[512];
    int doc_key;
} Response;

// Global variables (declared as extern)
extern char document_folder[256];
extern int cache_size;
extern Document *documents;
extern int num_documents;
extern int next_key;
extern int global_time;

#endif /* COMMON_H */