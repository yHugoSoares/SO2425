// src/main_client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "common.h"

#define FIFO_SERVER "/tmp/server_fifo"

void prepararPedido(int argc, char *argv[], MensagemCliente *pedido) {
    // adicionar 
    if (strcmp(argv[1], "-a") == 0) { 
        if (argc != 6) {
            fprintf(stderr, "Uso: %s -a \"title\" \"authors\" \"year\" \"path\"\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        pedido->operacao = 'a';
        snprintf(pedido->dados, MAX_DADOS, "%s;%s;%s;%s", argv[2], argv[3], argv[4], argv[5]);
    }
    // consultar
    else if (strcmp(argv[1], "-c") == 0) { 
        if (argc != 3) {
            fprintf(stderr, "Uso: %s -c \"key\"\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        pedido->operacao = 'c';
        snprintf(pedido->dados, MAX_DADOS, "%s", argv[2]);
    }
    // remove
    else if (strcmp(argv[1], "-d") == 0) { 
        if (argc != 3) {
            fprintf(stderr, "Uso: %s -d \"key\"\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        pedido->operacao = 'd';
        snprintf(pedido->dados, MAX_DADOS, "%s", argv[2]);
    }
    // conta linhas
    else if (strcmp(argv[1], "-l") == 0) { 
        if (argc != 4) {
            fprintf(stderr, "Uso: %s -l \"key\" \"keyword\"\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        pedido->operacao = 'l';
        snprintf(pedido->dados, MAX_DADOS, "%s;%s", argv[2], argv[3]);
    }
    // pesquisa 
    else if (strcmp(argv[1], "-s") == 0) { 
        if (argc == 3) {
            pedido->operacao = 's';
            snprintf(pedido->dados, MAX_DADOS, "%s", argv[2]);
        }
        else if (argc == 4) {
            pedido->operacao = 's';
            snprintf(pedido->dados, MAX_DADOS, "%s;%s", argv[2], argv[3]);
        }
        else {
            fprintf(stderr, "Uso: %s -s \"keyword\" [nr_processes]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    // termina
    else if (strcmp(argv[1], "-f") == 0) { 
        if (argc != 2) {
            fprintf(stderr, "Uso: %s -f\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        pedido->operacao = 'f';
        strcpy(pedido->dados, "");
    }
    else {
        fprintf(stderr, "Operação inválida: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    strcpy(pedido->resposta_fifo, "");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <operacao> [dados...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd_server = open(FIFO_SERVER, O_WRONLY);
    if (fd_server == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    MensagemCliente pedido;
    memset(&pedido, 0, sizeof(MensagemCliente));

    prepararPedido(argc, argv, &pedido);

    if (write(fd_server, &pedido, sizeof(MensagemCliente)) == -1) {
        perror("write");
        close(fd_server);
        exit(EXIT_FAILURE);
    }

    close(fd_server);

    printf("Pedido '%c' enviado ao servidor.\n", pedido.operacao);

    return 0;
}

