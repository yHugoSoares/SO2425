#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "common.h"
#include "cache.h"
#include "index.h"

#define MAX_PROCESS 10
extern Cache *global_cache;

int handle_shutdown(Pedido pedido) {
    char resposta[] = "Server is shuting down...";

    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pedido.pid);

    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } else {
        perror("Erro ao abrir FIFO de resposta");
    }
    

    return 0;  
}

int handle_add(Pedido pedido) {
    char resposta[256];

    // Create a new metadata entry
    Entry *entry = create_index_entry(pedido.title, pedido.authors, pedido.year, pedido.path, 0);
    if (!entry) {
        snprintf(resposta, sizeof(resposta), "Erro: falha ao criar entrada de metadados.");
    } else if (cache_add_entry(index_get_next_key(), entry, 1) != 0) {
        snprintf(resposta, sizeof(resposta), "Erro: falha ao adicionar entrada ao cache.");
        destroy_index_entry(entry);
    } else {
        snprintf(resposta, sizeof(resposta), "Document %d indexed." , pedido.key);
    }

    // Send response to client
    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pedido.pid);
    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } else {
        perror("Erro ao abrir FIFO de resposta");
    }

    return 1;
}

int handle_consulta(Pedido pedido) {
    char resposta[512];
    Entry *entry = cache_get_entry(pedido.key);

    if (!entry || entry->delete_flag) {
        snprintf(resposta, sizeof(resposta), "Document %d not found.", pedido.key);
    } else {
        snprintf(resposta, sizeof(resposta),
                 "Title: %s\nAuthors: %s\nYear: %s\nPath: %s",
                 entry->Title, entry->authors, entry->year, entry->path);
    }

    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pedido.pid);
    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } else {
        perror("Erro ao abrir FIFO de resposta");
    }

    return 1;
}

int handle_remove(Pedido pedido) {
    char resposta[256];
    
    if (cache_delete_entry(pedido.key) == 0) {
        snprintf(resposta, sizeof(resposta), "Index entry %d deleted.", pedido.key);
    } else {
        snprintf(resposta, sizeof(resposta), "Document %d not found.", pedido.key);
    }

    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pedido.pid);
    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } else {
        perror("Erro ao abrir FIFO de resposta");
    }

    return 1;
}

int handle_lines_number(Pedido pedido, const char *document_folder) {
    char resposta[256];
    int found = 0;

    // Retrieve the entry from the cache
    Entry *entry = cache_get_entry(pedido.key);
    if (entry && entry->path[0] != '\0') {
        int resp = conta_linhas_com_palavra(entry->path, pedido.keyword);
        snprintf(resposta, sizeof(resposta), "Document %d found in %d lines.", pedido.key, resp);
        found = 1;
    }

    if (!found) {
        snprintf(resposta, sizeof(resposta), "Document %d not found.", pedido.key);
    }

    // Send response to client
    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pedido.pid);
    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } else {
        perror("Erro ao abrir FIFO de resposta");
    }

    return 1;
}


int handle_search(Pedido pedido, const char *document_folder) {
    char resposta[1024] = "";  // Increased buffer size for multiple file paths
    int found = 0;
    int active_processes = 0;

    // Iterate over all cache entries
    for (int i = 0; i < global_cache->size; i++) {
        Entry *entry = cache_get_entry(i);
        if (!entry || entry->delete_flag) {
            continue;
        }

        // Limit the number of active child processes to pedido.n_procs
        if (active_processes >= pedido.n_procs) {
            int status;
            wait(&status);  // Wait for a child process to finish
            active_processes--;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("Erro ao criar processo");
            return 0;
        }

        if (pid == 0) {
            // Child process
            char file[256];
            snprintf(file, sizeof(file), "%s", entry->path);

            // Execute grep to search for the keyword in the file
            execlp("grep", "grep", "-q", pedido.keyword, file, NULL);

            // If execlp fails
            perror("Erro ao executar grep");
            _exit(EXIT_FAILURE);
        } else {
            // Parent process
            active_processes++;

            int status;
            waitpid(pid, &status, 0);  // Wait for the specific child process to finish
            active_processes--;

            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                // If grep found the keyword, add the file path to the response
                found++;
                strncat(resposta, entry->path, sizeof(resposta) - strlen(resposta) - 2);
                strncat(resposta, "\n", sizeof(resposta) - strlen(resposta) - 1);
            }
        }
    }

    // Wait for any remaining child processes
    while (active_processes > 0) {
        wait(NULL);
        active_processes--;
    }

    // If no files were found, set an appropriate response
    if (found == 0) {
        strncpy(resposta, "Nenhum documento possui a palavra pedida", sizeof(resposta) - 1);
        resposta[sizeof(resposta) - 1] = '\0';
    }

    // Send the response to the client
    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pedido.pid);
    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } else {
        perror("Erro ao abrir FIFO de resposta");
    }

    return 1;
}


int handle_request(Pedido pedido, const char *document_folder) {
    cache_load_from_file(METADATA_FILE);
    
    switch (pedido.operacao) {
        case 'f':
            return handle_shutdown(pedido);
        case 'a':
            return handle_add(pedido);
        case 'c':
            return handle_consulta(pedido);
        case 'd':
            return handle_remove(pedido);
        case 'l':
            return handle_lines_number(pedido, document_folder); // por fazer os testes e comprovar
        case 's':
            return handle_search(pedido, document_folder); // por fazer os testes e comprovar devido a não conseguir anexar os ficheiros
        default:
            fprintf(stderr, "Unknown operation.");
            return 1;
    }
}
