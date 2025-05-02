#ifndef COMMON_H
#define COMMON_H

#define MAX_DADOS 512
#define MAX_FIFO_NAME 256
#define MAX_DOCS 1000

#define FIFO_SERVER "/tmp/server_fifo"
#define FIFO_CLIENT "/tmp/client_fifo"
#define CACHE_FILE "cache.dat"

typedef struct {
    int id;
    char title[200];
    char authors[200];
    char year[5];
    char path[64];
} Document;

typedef struct {
    Document docs[MAX_DOCS];
    int count;
    int last_id;
} Metadata;

typedef struct {
    char operacao;
    char dados[MAX_DADOS];
    char resposta_fifo[MAX_FIFO_NAME];
} MensagemCliente;

#endif
