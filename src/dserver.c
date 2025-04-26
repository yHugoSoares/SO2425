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
char document_folder[256];
int cache_size;
int num_documents = 0;
int next_key = 1;
int server_running = 1;
int global_time = 0; // For LRU cache policy

int main(int argc, char *argv[]) {
    
    return 0;
}
