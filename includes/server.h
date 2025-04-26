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


// Global variables
extern char document_folder[256];
extern int cache_size;
extern int num_documents;
extern int next_key;
extern int server_running;
extern int global_time;

#endif /* SERVER_H */
