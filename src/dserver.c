#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include "common.h"  

#define FIFO_SERVER "/tmp/server_fifo"

volatile sig_atomic_t running = 1;

// funcao que limpa o server de maneira segura
void sigint_handler(int sig) {
    running = 0;
}

int main(int argc, char *argv[]) {

    // mensagem de erro caso não execute bem 
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <document_folder> <cache_size>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *document_folder = argv[1];
    int cache_size = atoi(argv[2]);

    printf("Servidor iniciado!\n");
    printf("Pasta de documentos: %s\n", document_folder);
    printf("Tamanho da cache: %d\n", cache_size);

    // inicia FIFO
    if (mkfifo(FIFO_SERVER, 0666) == -1) {
        perror("mkfifo");
    }

    signal(SIGINT, sigint_handler);

    int fd_server;
    MensagemCliente pedido;

    while (running) {
        fd_server = open(FIFO_SERVER, O_RDONLY);
        if (fd_server == -1) {
            perror("open");
            break;
        }

        while (read(fd_server, &pedido, sizeof(MensagemCliente)) > 0) {
            printf("Recebido pedido: operação '%c'\n", pedido.operacao);

            if (pedido.operacao == 'f') {
                printf("Pedido de shutdown recebido. A terminar servidor.\n");
                running = 0;
                break;
            }

        }

        close(fd_server);
    }

    unlink(FIFO_SERVER);

    printf("Servidor terminado.\n");
    return 0;
}
