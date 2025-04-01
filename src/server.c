#include "server.h"


// Signal handler for graceful shutdown
void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("Received signal %d, shutting down...\n", sig);
        server_running = 0;
    }
}

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s document_folder cache_size\n", argv[0]);
        return 1;
    }

    // Parse arguments
    strncpy(document_folder, argv[1], sizeof(document_folder) - 1);
    cache_size = atoi(argv[2]);

    if (cache_size <= 0) {
        fprintf(stderr, "Cache size must be a positive integer\n");
        return 1;
    }

    // Set up signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Initialize server
    initialize_server();

    // Load metadata from disk
    load_metadata();

    // Handle client requests
    handle_client_requests();

    // Cleanup before exit
    cleanup_server();

    return 0;
}

void initialize_server() {
    printf("Initializing server with document folder: %s, cache size: %d\n", document_folder, cache_size);

    // Create request pipe if it doesn't exist
    struct stat st;
    if (stat(REQUEST_PIPE, &st) == -1) {
        if (mkfifo(REQUEST_PIPE, 0666) == -1) {
            perror("Error creating request pipe");
            exit(1);
        }
    }

    // Create response pipe if it doesn't exist
    if (stat(RESPONSE_PIPE, &st) == -1) {
        if (mkfifo(RESPONSE_PIPE, 0666) == -1) {
            perror("Error creating response pipe");
            exit(1);
        }
    }

    printf("Server initialized successfully\n");
}

void cleanup_server() {
    printf("Cleaning up server resources...\n");

    // Save metadata to disk
    save_metadata();

    // Free document metadata
    for (int i = 0; i < num_documents; i++) {
        free(documents[i].title);
        free(documents[i].authors);
        free(documents[i].path);
    }
    free(documents);

    // Remove named pipes
    unlink(REQUEST_PIPE);
    unlink(RESPONSE_PIPE);

    printf("Server shutdown complete\n");
}

void handle_client_requests() {
    int request_fd, response_fd;
    Request req;
    Response resp;

    printf("Server is running. Waiting for client requests...\n");

    while (server_running) {
        // Open request pipe for reading
        request_fd = open(REQUEST_PIPE, O_RDONLY);
        if (request_fd == -1) {
            if (errno == EINTR) {
                // Interrupted by signal, check if we should continue
                if (!server_running) break;
                continue;
            }
            perror("Error opening request pipe");
            continue;
        }

        // Read request
        ssize_t bytes_read = read(request_fd, &req, sizeof(Request));
        close(request_fd);

        if (bytes_read <= 0) {
            if (errno == EINTR) {
                // Interrupted by signal, check if we should continue
                if (!server_running) break;
                continue;
            }
            perror("Error reading from request pipe");
            continue;
        }

        // Process request
        memset(&resp, 0, sizeof(Response));
        process_request(&req, &resp);

        // Open response pipe for writing
        response_fd = open(RESPONSE_PIPE, O_WRONLY);
        if (response_fd == -1) {
            perror("Error opening response pipe");
            continue;
        }

        // Send response
        write(response_fd, &resp, sizeof(Response));
        close(response_fd);

        // Check if server should shut down
        if (req.operation == OP_SHUTDOWN) {
            server_running = 0;
        }
    }
}

void process_request(Request *req, Response *resp) {
    printf("Processing request with operation: %d\n", req->operation);

    switch (req->operation) {
        case OP_ADD:
            handle_add_document(req, resp);
            break;
        case OP_QUERY:
            handle_query_document(req, resp);
            break;
        case OP_DELETE:
            handle_delete_document(req, resp);
            break;
        case OP_LINE_COUNT:
            handle_line_count(req, resp);
            break;
        case OP_SEARCH:
            handle_search(req, resp);
            break;
        case OP_SEARCH_PARALLEL:
            handle_search_parallel(req, resp);
            break;
        case OP_SHUTDOWN:
            handle_shutdown(resp);
            break;
        default:
            resp->status = STATUS_ERROR;
            strncpy(resp->message, "Invalid operation", sizeof(resp->message) - 1);
            break;
    }
}

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

void handle_line_count(Request *req, Response *resp) {
    int key = atoi(req->key);
    printf("Counting lines with keyword '%s' in document %d\n", req->keyword, key);

    int doc_index = find_document_by_key(key);
    if (doc_index >= 0) {
        // Update cache
        update_cache(doc_index);
        
        // Get full path to document
        char *full_path = get_full_path(documents[doc_index].path);
        
        // Count lines with the keyword
        int count = count_lines_with_keyword(full_path, req->keyword);
        free(full_path);
        
        // Prepare response
        resp->status = STATUS_OK;
        snprintf(resp->message, sizeof(resp->message), "%d", count);
        return;
    }

    // Document not found
    resp->status = STATUS_ERROR;
    snprintf(resp->message, sizeof(resp->message), "Document with key %d not found", key);
}

int count_lines_with_keyword(const char *doc_path, const char *keyword) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    
    if (pid == 0) {
        // Child process
        close(pipefd[0]); // Close read end
        
        // Redirect stdout to pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        // Execute grep | wc -l
        int grep_to_wc[2];
        if (pipe(grep_to_wc) == -1) {
            perror("pipe");
            exit(1);
        }
        
        pid_t grep_pid = fork();
        if (grep_pid == -1) {
            perror("fork");
            exit(1);
        }
        
        if (grep_pid == 0) {
            // Grep process
            close(grep_to_wc[0]);
            dup2(grep_to_wc[1], STDOUT_FILENO);
            close(grep_to_wc[1]);
            
            execlp("grep", "grep", "-i", keyword, doc_path, NULL);
            perror("execlp grep");
            exit(1);
        }
        
        // Wc process
        close(grep_to_wc[1]);
        dup2(grep_to_wc[0], STDIN_FILENO);
        close(grep_to_wc[0]);
        
        execlp("wc", "wc", "-l", NULL);
        perror("execlp wc");
        exit(1);
    }
    
    // Parent process
    close(pipefd[1]); // Close write end
    
    // Read result
    char buffer[32];
    ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
    close(pipefd[0]);
    
    // Wait for child to finish
    waitpid(pid, NULL, 0);
    
    if (bytes_read <= 0) {
        return 0;
    }
    
    buffer[bytes_read] = '\0';
    return atoi(buffer);
}

void handle_search(Request *req, Response *resp) {
    printf("Searching for documents containing keyword: %s\n", req->keyword);
    
    int *results = NULL;
    int result_count = 0;
    
    // Sequential search
    for (int i = 0; i < num_documents; i++) {
        char *full_path = get_full_path(documents[i].path);
        if (document_contains_keyword(full_path, req->keyword)) {
            // Add document key to results
            results = realloc(results, (result_count + 1) * sizeof(int));
            results[result_count++] = documents[i].key;
            
            // Update cache
            update_cache(i);
        }
        free(full_path);
    }
    
    // Format results
    char result_str[512] = "[";
    for (int i = 0; i < result_count; i++) {
        char key_str[16];
        snprintf(key_str, sizeof(key_str), "%d", results[i]);
        
        if (i > 0) {
            strcat(result_str, ", ");
        }
        strcat(result_str, key_str);
    }
    strcat(result_str, "]");
    
    // Prepare response
    resp->status = STATUS_OK;
    strncpy(resp->message, result_str, sizeof(resp->message) - 1);
    
    // Free results
    free(results);
}

void handle_search_parallel(Request *req, Response *resp) {
    printf("Parallel searching for documents containing keyword: %s with %d processes\n", 
           req->keyword, req->nr_processes);
    
    int nr_processes = req->nr_processes;
    
    // Limit number of processes to number of documents
    if (nr_processes > num_documents) {
        nr_processes = num_documents;
    }
    
    // Create pipes for communication with child processes
    int **pipes = malloc(nr_processes * sizeof(int*));
    for (int i = 0; i < nr_processes; i++) {
        pipes[i] = malloc(2 * sizeof(int));
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            
            // Clean up already created pipes
            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
                free(pipes[j]);
            }
            free(pipes);
            
            resp->status = STATUS_ERROR;
            strncpy(resp->message, "Failed to create pipes", sizeof(resp->message) - 1);
            return;
        }
    }
    
    // Distribute documents among processes
    int docs_per_process = num_documents / nr_processes;
    int remaining_docs = num_documents % nr_processes;
    
    // Fork child processes
    for (int i = 0; i < nr_processes; i++) {
        pid_t pid = fork();
        
        if (pid == -1) {
            perror("fork");
            
            // Clean up pipes
            for (int j = 0; j < nr_processes; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
                free(pipes[j]);
            }
            free(pipes);
            
            resp->status = STATUS_ERROR;
            strncpy(resp->message, "Failed to fork process", sizeof(resp->message) - 1);
            return;
        }
        
        if (pid == 0) {
            // Child process
            
            // Close all read ends and other processes' write ends
            for (int j = 0; j < nr_processes; j++) {
                if (j != i) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                else {
                    close(pipes[j][0]);
                }
            }
            
            // Calculate range of documents for this process
            int start = i * docs_per_process + (i < remaining_docs ? i : remaining_docs);
            int end = start + docs_per_process + (i < remaining_docs ? 1 : 0);
            
            // Search for keyword in assigned documents
            int *proc_results = NULL;
            int proc_result_count = 0;
            
            for (int j = start; j < end; j++) {
                char *full_path = get_full_path(documents[j].path);
                if (document_contains_keyword(full_path, req->keyword)) {
                    // Add document key to results
                    proc_results = realloc(proc_results, (proc_result_count + 1) * sizeof(int));
                    proc_results[proc_result_count++] = documents[j].key;
                }
                free(full_path);
            }
            
            // Write result count and results to pipe
            write(pipes[i][1], &proc_result_count, sizeof(int));
            if (proc_result_count > 0) {
                write(pipes[i][1], proc_results, proc_result_count * sizeof(int));
                free(proc_results);
            }
            
            close(pipes[i][1]);
            exit(0);
        }
        
        // Parent process continues
    }
    
    // Parent process: close all write ends
    for (int i = 0; i < nr_processes; i++) {
        close(pipes[i][1]);
    }
    
    // Collect results from child processes
    int *all_results = NULL;
    int total_results = 0;
    
    for (int i = 0; i < nr_processes; i++) {
        int proc_result_count;
        read(pipes[i][0], &proc_result_count, sizeof(int));
        
        if (proc_result_count > 0) {
            int *proc_results = malloc(proc_result_count * sizeof(int));
            read(pipes[i][0], proc_results, proc_result_count * sizeof(int));
            
            // Add to all results
            all_results = realloc(all_results, (total_results + proc_result_count) * sizeof(int));
            memcpy(all_results + total_results, proc_results, proc_result_count * sizeof(int));
            total_results += proc_result_count;
            
            free(proc_results);
        }
        
        close(pipes[i][0]);
        
        // Wait for child process to finish
        wait(NULL);
    }
    
    // Free pipes
    for (int i = 0; i < nr_processes; i++) {
        free(pipes[i]);
    }
    free(pipes);
    
    // Update cache for found documents
    for (int i = 0; i < total_results; i++) {
        int doc_index = find_document_by_key(all_results[i]);
        if (doc_index >= 0) {
            update_cache(doc_index);
        }
    }
    
    // Format results
    char result_str[512] = "[";
    for (int i = 0; i < total_results; i++) {
        char key_str[16];
        snprintf(key_str, sizeof(key_str), "%d", all_results[i]);
        
        if (i > 0) {
            strcat(result_str, ", ");
        }
        strcat(result_str, key_str);
    }
    strcat(result_str, "]");
    
    // Prepare response
    resp->status = STATUS_OK;
    strncpy(resp->message, result_str, sizeof(resp->message) - 1);
    
    // Free results
    free(all_results);
}

int document_contains_keyword(const char *doc_path, const char *keyword) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 0;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return 0;
    }
    
    if (pid == 0) {
        // Child process
        close(pipefd[0]); // Close read end
        
        // Redirect stdout to pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        // Execute grep -q (quiet mode, exit with status 0 if match found)
        execlp("grep", "grep", "-q", "-i", keyword, doc_path, NULL);
        
        // If execlp fails
        perror("execlp");
        exit(1);
    }
    
    // Parent process
    close(pipefd[1]); // Close write end
    
    // Wait for grep to finish
    int status;
    waitpid(pid, &status, 0);
    
    close(pipefd[0]);
    
    // Check if grep found the keyword (exit status 0 means found)
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

void handle_shutdown(Response *resp) {
    printf("Shutting down server...\n");
    
    // Save metadata to disk
    save_metadata();
    
    // Prepare response
    resp->status = STATUS_OK;
    strncpy(resp->message, "Server is shutting down", sizeof(resp->message) - 1);
    
    // Server will exit after sending response
}

void save_metadata() {
    printf("Saving metadata to disk...\n");
    
    // Open metadata file for writing
    int fd = open(METADATA_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error opening metadata file for writing");
        return;
    }
    
    // Write number of documents
    write(fd, &num_documents, sizeof(int));
    
    // Write next key
    write(fd, &next_key, sizeof(int));
    
    // Write document metadata
    for (int i = 0; i < num_documents; i++) {
        Document *doc = &documents[i];
        
        // Write key
        write(fd, &doc->key, sizeof(int));
        
        // Write year
        write(fd, &doc->year, sizeof(int));
        
        // Write cache info
        write(fd, &doc->in_cache, sizeof(int));
        write(fd, &doc->access_count, sizeof(int));
        write(fd, &doc->last_access, sizeof(int));
        
        // Write title length and title
        int len = strlen(doc->title) + 1;
        write(fd, &len, sizeof(int));
        write(fd, doc->title, len);
        
        // Write authors length and authors
        len = strlen(doc->authors) + 1;
        write(fd, &len, sizeof(int));
        write(fd, doc->authors, len);
        
        // Write path length and path
        len = strlen(doc->path) + 1;
        write(fd, &len, sizeof(int));
        write(fd, doc->path, len);
    }
    
    close(fd);
    printf("Metadata saved successfully\n");
}

void load_metadata() {
    printf("Loading metadata from disk...\n");
    
    // Check if metadata file exists
    struct stat st;
    if (stat(METADATA_FILE, &st) == -1) {
        printf("No metadata file found, starting with empty database\n");
        return;
    }
    
    // Open metadata file for reading
    int fd = open(METADATA_FILE, O_RDONLY);
    if (fd == -1) {
        perror("Error opening metadata file for reading");
        return;
    }
    
    // Read number of documents
    read(fd, &num_documents, sizeof(int));
    
    // Read next key
    read(fd, &next_key, sizeof(int));
    
    // Allocate memory for documents
    documents = malloc(num_documents * sizeof(Document));
    if (!documents) {
        perror("Memory allocation error");
        close(fd);
        return;
    }
    
    // Read document metadata
    for (int i = 0; i < num_documents; i++) {
        Document *doc = &documents[i];
        
        // Read key
        read(fd, &doc->key, sizeof(int));
        
        // Read year
        read(fd, &doc->year, sizeof(int));
        
        // Read cache info
        read(fd, &doc->in_cache, sizeof(int));
        read(fd, &doc->access_count, sizeof(int));
        read(fd, &doc->last_access, sizeof(int));
        
        // Read title
        int len;
        read(fd, &len, sizeof(int));
        doc->title = malloc(len);
        read(fd, doc->title, len);
        
        // Read authors
        read(fd, &len, sizeof(int));
        doc->authors = malloc(len);
        read(fd, doc->authors, len);
        
        // Read path
        read(fd, &len, sizeof(int));
        doc->path = malloc(len);
        read(fd, doc->path, len);
    }
    
    close(fd);
    
    // Adjust cache to respect cache_size
    int in_cache_count = 0;
    for (int i = 0; i < num_documents; i++) {
        if (documents[i].in_cache) {
            in_cache_count++;
        }
    }
    
    // If we have more documents in cache than allowed, remove some
    if (in_cache_count > cache_size) {
        // Sort by last_access (ascending)
        for (int i = 0; i < num_documents - 1; i++) {
            for (int j = 0; j < num_documents - i - 1; j++) {
                if (documents[j].in_cache && documents[j+1].in_cache && 
                    documents[j].last_access > documents[j+1].last_access) {
                    // Swap
                    Document temp = documents[j];
                    documents[j] = documents[j+1];
                    documents[j+1] = temp;
                }
            }
        }
        
        // Remove excess documents from cache
        for (int i = 0; i < num_documents && in_cache_count > cache_size; i++) {
            if (documents[i].in_cache) {
                documents[i].in_cache = 0;
                in_cache_count--;
            }
        }
    }
    
    printf("Loaded %d documents from metadata file\n", num_documents);
}

void update_cache(int doc_index) {
    // Update access statistics
    documents[doc_index].access_count++;
    documents[doc_index].last_access = global_time++;
    
    // If document is not in cache, try to add it
    if (!documents[doc_index].in_cache) {
        // Count documents in cache
        int in_cache_count = 0;
        for (int i = 0; i < num_documents; i++) {
            if (documents[i].in_cache) {
                in_cache_count++;
            }
        }
        
        if (in_cache_count < cache_size) {
            // We have room in the cache
            documents[doc_index].in_cache = 1;
        } else {
            // Cache is full, replace least recently used
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
                documents[doc_index].in_cache = 1;
            }
        }
    }
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

