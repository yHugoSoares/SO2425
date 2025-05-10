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

#define MAX_PROCESS 10
Metadata metadata;
extern Cache *global_cache;

// Saves the metadata to a file


int handle_shutdown(Pedido pedido) {
    char resposta[] = "Servidor a terminar...";

    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/pipe_%d", pedido.pid);

    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } else {
        perror("Erro ao abrir FIFO de resposta");
    }
    save_metadata(metadata);

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
        snprintf(resposta, sizeof(resposta), "Documento indexado com sucesso.");
    }

    // Send response to client
    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/pipe_%d", pedido.pid);
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
    int found = 0;

    for (int i = 0; i < metadata.count; i++) {
        if (metadata.entry[i].key == pedido.key) {
            snprintf(resposta, sizeof(resposta),
                     "Title: %s\nAuthors: %s\nYear: %s\nPath: %s",
                     metadata.entry[i].title,
                     metadata.entry[i].authors,
                     metadata.entry[i].year,
                     metadata.entry[i].path);
            found = 1;
            break;
        }
    }

    if (!found) {
        snprintf(resposta, sizeof(resposta), "Documento com ID %d não encontrado.", pedido.key);
    }

    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/pipe_%d", pedido.pid);
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
    int found = 0;

    for (int i = 0; i < metadata.count; i++) {
        if (metadata.entry[i].key == pedido.key) {
            for (int j = i; j < metadata.count - 1; j++) {
                metadata.entry[j] = metadata.entry[j + 1];
            }
            metadata.count--;
            save_metadata(metadata);
            found = 1;
            break;
        }
    }

    if (found) {
        snprintf(resposta, sizeof(resposta), "Document %d removed.", pedido.key);
    } else {
        snprintf(resposta, sizeof(resposta), "Document %d not found", pedido.key);
    }

    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/pipe_%d", pedido.pid);
    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } else {
        perror("Erro ao abrir FIFO de resposta");
    }

    return 1;
}


int handle_lines_number(Pedido pedido, const char *document_folder){
    char resposta[256];
    //int lines_number = 0;
    int found = 0;// função para retornar o numero de linhas que contêm uma dada palavra
    for (int i = 0; i< metadata.count;i++){
        int resp;
        if(metadata.entry[i].key == pedido.key){
            if (metadata.entry[i].path[0] != '\0'){
                resp= conta_linhas_com_palavra(metadata.entry[i].path,pedido.keyword);
                found=1;
            }
        }
        if (found == 0){
            snprintf(resposta, sizeof(resposta), "Document %d not found.", pedido.key);
        }
        else{
            snprintf(resposta, sizeof(resposta), "Document %d found in %d lines.", pedido.key, resp);
        }
        break;
    }
    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/pipe_%d",pedido.pid);
    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } 
    else {
        perror("Erro ao abrir FIFO de resposta");
    }
    return 1;
}


int handle_search(Pedido pedido, const char *document_folder) {
    char resposta[1024] = "";  // Buffer for the response
    int found = 0;

    // Iterate through all documents in metadata
    for (int i = 0; i < metadata.count; i++) {
        Entry *entry = &metadata.entry[i];

        // Construct the full file path
        char full_path[256];
        snprintf(full_path, sizeof(full_path), "%s/%s", document_folder, entry->path);

        // Use grep to search for the keyword in the file
        pid_t pid = fork();
        if (pid == -1) {
            perror("Erro ao criar processo");
            return 0;
        }

        if (pid == 0) {
            // Child process
            execlp("grep", "grep", "-q", pedido.keyword, full_path, NULL);

            // If execlp fails
            perror("Erro ao executar grep");
            _exit(EXIT_FAILURE);
        } else {
            // Parent process
            int status;
            waitpid(pid, &status, 0);  // Wait for the specific child process to finish

            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                // If grep found the keyword, add the file path to the response
                found++;
                strncat(resposta, entry->path, sizeof(resposta) - strlen(resposta) - 2);
                strncat(resposta, "\n", sizeof(resposta) - strlen(resposta) - 1);
            }
        }
    }

    // If no files were found, set an appropriate response
    if (found == 0) {
        strncpy(resposta, "Nenhum documento possui a palavra pedida", sizeof(resposta) - 1);
        resposta[sizeof(resposta) - 1] = '\0';
    }

    // Send the response to the client
    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/pipe_%d", pedido.pid);
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
            fprintf(stderr, "Operação desconhecida.");
            return 1;
    }
}
