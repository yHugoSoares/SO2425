#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "search_operations.h"
#include "document_operations.h"
#include "cache_management.h"

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

