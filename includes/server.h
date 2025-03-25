#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include "common.h"
#include "document_operations.h"
#include "cache_management.h"
#include "search_operations.h"
#include "metadata_management.h"

// Server function prototypes
void initialize_server();
void cleanup_server();
void handle_client_requests();
void process_request(Request *req, Response *resp);
void handle_shutdown(Response *resp);
void handle_signal(int sig);

// Global variables
extern char document_folder[256];
extern int cache_size;
extern Document *documents;
extern int num_documents;
extern int next_key;
extern int server_running;
extern int global_time;

#endif /* SERVER_H */
