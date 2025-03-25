#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "common.h"

// Client function prototypes
void send_request_and_get_response(Request *req, Response *resp);
void add_document(const char *title, const char *authors, const char *year, const char *path);
void query_document(const char *key);
void delete_document(const char *key);
void count_lines(const char *key, const char *keyword);
void search_documents(const char *keyword, const char *nr_processes);
void shutdown_server();

#endif /* CLIENT_H */

