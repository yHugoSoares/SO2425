#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_DADOS 450
#define MAX_FIFO_NAME 256
#define MAX_DOCS 1000
#define REQUEST_PIPE "/tmp/request_pipe"
#define MAX_OP 3

#define FIFO_SERVER "/tmp/server_fifo"
#define FIFO_CLIENT "/tmp/client_fifo"
#define CACHE_FILE "cache.dat"
#define METADATA_FILE "metadata.dat"

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
    pid_t pid;
    char operacao[MAX_OP];
    char dados[MAX_DADOS];
    char resposta_fifo[MAX_FIFO_NAME];
} MensagemCliente;

int conta_linhas_com_palavra(const char *filepath, const char *keyword);

#endif
