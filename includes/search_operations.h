#ifndef SEARCH_OPERATIONS_H
#define SEARCH_OPERATIONS_H

#include "common.h"

// Search operations function prototypes
void handle_line_count(Request *req, Response *resp);
void handle_search(Request *req, Response *resp);
void handle_search_parallel(Request *req, Response *resp);
int document_contains_keyword(const char *doc_path, const char *keyword);
int count_lines_with_keyword(const char *doc_path, const char *keyword);

// External variables that need to be accessed
extern Document *documents;
extern int num_documents;

#endif /* SEARCH_OPERATIONS_H */

