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

Metadata metadata;


// // NOT FULLY CORRECT
void save_metadata() {
    int fd = open(METADATA_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("save_metadata");
        return;
    }

    ssize_t written = write(fd, &metadata, sizeof(metadata));
    if (written != sizeof(metadata)) {
        perror("Error writing metadata");
    }

    close(fd);
}

void load_metadata() {
    int fd = open(METADATA_FILE, O_RDONLY);
    if (fd == -1) {
        metadata.count = 0;
        metadata.last_id = 0;
        return;
    }
    read(fd, &metadata, sizeof(Metadata));
    close(fd);
}

int handle_request(MensagemCliente pedido, const char *document_folder) {
    char resposta[1024] = "";
    // int found = 0;

    switch (pedido.operacao) {
        case 'f': {  // SHUTDOWN
            snprintf(resposta, sizeof(resposta), "Servidor a terminar...");

            // Construir FIFO de resposta
            char fifo_resposta[MAX_FIFO_NAME];
            snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pedido.pid);

            int fd_resp = open(fifo_resposta, O_WRONLY);
            if (fd_resp != -1) {
                write(fd_resp, resposta, strlen(resposta) + 1);
                close(fd_resp);
            } else {
                perror("Erro ao abrir FIFO de resposta");
            }
            return 0;  // Terminar o servidor
        }

        case 'a': {  // ADD DOCUMENT
            if (metadata.count >= MAX_DOCS) {
                snprintf(resposta, sizeof(resposta), "Erro: limite de documentos atingido.");
            } else {
                Document doc;
                doc.id = ++metadata.last_id;
                strncpy(doc.title, pedido.title, MAX_TITLE_SIZE - 1);
                strncpy(doc.authors, pedido.authors, MAX_AUTHOR_SIZE - 1);
                strncpy(doc.year, pedido.year, MAX_YEAR_SIZE - 1);
                strncpy(doc.path, pedido.path, MAX_PATH_SIZE - 1);
    
                metadata.docs[metadata.count++] = doc;
                save_metadata();
    
                snprintf(resposta, sizeof(resposta), "Documento %d indexado com sucesso.", doc.id);
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

        // case 'd': {  // DELETE DOCUMENT
        //     int id;
        //     if (sscanf(pedido.dados, "%d", &id) != 1) {
        //         snprintf(resposta, sizeof(resposta), "Erro: ID inválido.");
        //     } else {
        //         for (int i = 0; i < metadata.count; i++) {
        //             if (metadata.docs[i].id == id) {
        //                 metadata.docs[i] = metadata.docs[--metadata.count];
        //                 save_metadata();
        //                 snprintf(resposta, sizeof(resposta), "Index entry %d deleted", id);
        //                 found = 1;
        //                 break;
        //             }
        //         }
        //         if (!found) {
        //             snprintf(resposta, sizeof(resposta), "ID %d não encontrado.", id);
        //         }
        //     }
        //     break;
        // }

        // default: {  // QUERIES HANDLED IN CHILD PROCESS
        //     pid_t pid = fork();
        //     if (pid == 0) {  // Child process
        //         switch (pedido.operacao[1]) {
        //             case 'c': {  // CONSULT DOCUMENT
        //                 int id;
        //                 if (sscanf(pedido.dados, "%d", &id) == 1) {
        //                     for (int i = 0; i < metadata.count; i++) {
        //                         if (metadata.docs[i].id == id) {
        //                             snprintf(resposta, sizeof(resposta),
        //                                 "Title: %s\nAuthors: %s\nYear: %s\nPath: %s",
        //                                 metadata.docs[i].title, metadata.docs[i].authors,
        //                                 metadata.docs[i].year, metadata.docs[i].path);
        //                             found = 1;
        //                             break;
        //                         }
        //                     }
        //                     if (!found) {
        //                         snprintf(resposta, sizeof(resposta), "Documento com ID %d não encontrado.", id);
        //                     }
        //                 }
        //                 break;
        //             }

        //             case 'l': {  // COUNT LINES
        //                 int id;
        //                 char palavra[64];
        //                 if (sscanf(pedido.dados, "%d %s", &id, palavra) == 2) {
        //                     for (int i = 0; i < metadata.count; i++) {
        //                         if (metadata.docs[i].id == id) {
        //                             char caminho[256];
        //                             snprintf(caminho, sizeof(caminho), "%s/%s", 
        //                                    document_folder, metadata.docs[i].path);
        //                             int count = conta_linhas_com_palavra(caminho, palavra);
        //                             snprintf(resposta, sizeof(resposta), "%d", count);
        //                             found = 1;
        //                             break;
        //                         }
        //                     }
        //                     if (!found) {
        //                         snprintf(resposta, sizeof(resposta), "ID %d não encontrado.", id);
        //                     }
        //                 }
        //                 break;
        //             }

        //             case 's': {  // SEARCH
        //                 char palavra[64];
        //                 int max_processos = 1;
        //                 sscanf(pedido.dados, "%s %d", palavra, &max_processos);

        //                 int pipe_fds[2];
        //                 pipe(pipe_fds);

        //                 int filhos = 0;
        //                 for (int i = 0; i < metadata.count; i++) {
        //                     if (filhos == max_processos) wait(NULL);

        //                     if (fork() == 0) {
        //                         char caminho[256];
        //                         snprintf(caminho, sizeof(caminho), "%s/%s", 
        //                                document_folder, metadata.docs[i].path);
        //                         int count = conta_linhas_com_palavra(caminho, palavra);
        //                         if (count > 0) {
        //                             dprintf(pipe_fds[1], "%d ", metadata.docs[i].id);
        //                         }
        //                         exit(0);
        //                     }
        //                     filhos++;
        //                 }

        //                 while (wait(NULL) > 0);
        //                 close(pipe_fds[1]);
        //                 read(pipe_fds[0], resposta, sizeof(resposta));
        //                 close(pipe_fds[0]);
        //                 break;
        //             }

        //             default: {
        //                 snprintf(resposta, sizeof(resposta), "Operação inválida: %s", pedido.operacao);
        //                 break;
        //             }
        //         }

        //         // Send response
        //         int fd_resp = open(pedido.resposta_fifo, O_WRONLY);
        //         write(fd_resp, resposta, strlen(resposta) + 1);
        //         close(fd_resp);
        //         exit(0);
        //     } 
        //     else {  // Parent process
        //         waitpid(pid, NULL, 0);
        //         return 1;
        //     }
        // }
    }

    // Send response for parent-processed operations
    // int fd_resp = open(pedido.resposta_fifo, O_WRONLY);
    // write(fd_resp, resposta, strlen(resposta) + 1);
    // close(fd_resp);
    return 1;
}
