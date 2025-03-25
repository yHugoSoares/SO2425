#ifndef DOCUMENT_OPERATIONS_H
#define DOCUMENT_OPERATIONS_H

#include "common.h"

// Document operations function prototypes
void handle_add_document(Request *req, Response *resp);
void handle_query_document(Request *req, Response *resp);
void handle_delete_document(Request *req, Response *resp);
int find_document_by_key(int key);
char* get_full_path(const char *relative_path);

// External variables that need to be accessed
extern Document *documents;
extern int num_documents;
extern int next_key;
extern char document_folder[256];
extern int global_time;

#endif /* DOCUMENT_OPERATIONS_H */

