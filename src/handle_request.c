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

int handle_shutdown(MensagemCliente pedido) {
    char resposta[] = "Servidor a terminar...";

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

int handle_add(MensagemCliente pedido) {
    char resposta[256];

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

        snprintf(resposta, sizeof(resposta), "Document %d indexed", doc.id);
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

int handle_consulta(MensagemCliente pedido) {
    char resposta[512];
    int found = 0;

    for (int i = 0; i < metadata.count; i++) {
        if (metadata.docs[i].id == pedido.key) {
            snprintf(resposta, sizeof(resposta),
                     "Title: %s\nAuthors: %s\nYear: %s\nPath: %s",
                     metadata.docs[i].title,
                     metadata.docs[i].authors,
                     metadata.docs[i].year,
                     metadata.docs[i].path);
            found = 1;
            break;
        }
    }

    if (!found) {
        snprintf(resposta, sizeof(resposta), "Documento com ID %d não encontrado.", pedido.key);
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

int handle_remove(MensagemCliente pedido) {
    char resposta[256];
    int found = 0;

    for (int i = 0; i < metadata.count; i++) {
        if (metadata.docs[i].id == pedido.key) {
            for (int j = i; j < metadata.count - 1; j++) {
                metadata.docs[j] = metadata.docs[j + 1];
            }
            metadata.count--;
            save_metadata();
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




int handle_request(MensagemCliente pedido, const char *document_folder) {
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
            return handle_lines_number(pedido, document_folder); // por fazer 
        case 's':
            return handle_search(pedido, document_folder); // por fazer
        default:
            fprintf(stderr, "Operação desconhecida.");
            return 1;
    }
}
