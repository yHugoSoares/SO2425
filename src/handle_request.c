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
    char resposta[512];

    if (pedido.operacao == 'f') {
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

        return 0;  // termina o servidor
    }

    else if (pedido.operacao == 'a') {
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

    // Ignorar outras operações por enquanto
    return 1;
}