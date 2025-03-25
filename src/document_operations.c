#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "document_operations.h"
#include "cache_management.h"
#include "metadata_management.h"

void handle_add_document(Request *req, Response *resp) {
    printf("Adding document: %s\n", req->title);

    // Check if document path exists
    char *full_path = get_full_path(req->path);
    
    int file_fd = open(full_path, O_RDONLY);
    if (file_fd == -1) {
        resp->status = STATUS_ERROR;
        snprintf(resp->message, sizeof(resp->message), "Document file not found: %s", full_path);
        free(full_path);
        return;
    }
    close(file_fd);
    free(full_path);

    // Allocate memory for new document
    documents = realloc(documents, (num_documents + 1) * sizeof(Document));
    if (!documents) {
        resp->status = STATUS_ERROR;
        strncpy(resp->message, "Memory allocation error", sizeof(resp->message) - 1);
        return;
    }

    // Initialize new document
    Document *doc = &documents[num_documents];
    doc->key = next_key++;
    doc->title = strdup(req->title);
    doc->authors = strdup(req->authors);
    doc->year = atoi(req->year);
    doc->path = strdup(req->path);
    doc->in_cache = 1; // New documents are added to cache
    doc->access_count = 1;
    doc->last_access = global_time++;

    num_documents++;

    // Manage cache if needed
    if (cache_size < num_documents) {
        // Find least recently used document to remove from cache
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
        }
    }

    // Save metadata to disk
    save_metadata();

    // Prepare response
    resp->status = STATUS_OK;
    snprintf(resp->message, sizeof(resp->message), "Document %d indexed", doc->key);
    resp->doc_key = doc->key;
}

void handle_query_document(Request *req, Response *resp) {
    int key = atoi(req->key);
    printf("Querying document with key: %d\n", key);

    int doc_index = find_document_by_key(key);
    if (doc_index >= 0) {
        // Document found, update cache
        update_cache(doc_index);
        
        // Prepare response
        resp->status = STATUS_OK;
        snprintf(resp->message, sizeof(resp->message),
                 "Title: %s\nAuthors: %s\nYear: %d\nPath: %s",
                 documents[doc_index].title, documents[doc_index].authors, 
                 documents[doc_index].year, documents[doc_index].path);
        return;
    }

    // Document not found
    resp->status = STATUS_ERROR;
    snprintf(resp->message, sizeof(resp->message), "Document with key %d not found", key);
}

void handle_delete_document(Request *req, Response *resp) {
    int key = atoi(req->key);
    printf("Deleting document with key: %d\n", key);

    int doc_index = find_document_by_key(key);
    if (doc_index >= 0) {
        // Free document resources
        free(documents[doc_index].title);
        free(documents[doc_index].authors);
        free(documents[doc_index].path);

        // Remove document by shifting remaining documents
        for (int j = doc_index; j < num_documents - 1; j++) {
            documents[j] = documents[j + 1];
        }
        num_documents--;

        // Resize documents array
        documents = realloc(documents, num_documents * sizeof(Document));

        // Save metadata to disk
        save_metadata();

        // Prepare response
        resp->status = STATUS_OK;
        snprintf(resp->message, sizeof(resp->message), "Index entry %d deleted", key);
        return;
    }

    // Document not found
    resp->status = STATUS_ERROR;
    snprintf(resp->message, sizeof(resp->message), "Document with key %d not found", key);
}

int find_document_by_key(int key) {
    for (int i = 0; i < num_documents; i++) {
        if (documents[i].key == key) {
            return i;
        }
    }
    return -1;
}

char* get_full_path(const char *relative_path) {
    char *full_path = malloc(strlen(document_folder) + strlen(relative_path) + 2);
    sprintf(full_path, "%s/%s", document_folder, relative_path);
    return full_path;
}
