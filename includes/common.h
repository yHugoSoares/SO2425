#ifndef COMMON_H
#define COMMON_H

#define MAX_DADOS 512
#define MAX_FIFO_NAME 256

typedef struct {
    char operacao;
    char dados[MAX_DADOS];
    char resposta_fifo[MAX_FIFO_NAME];
} MensagemCliente;

#endif
