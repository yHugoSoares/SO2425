#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_FIFO_NAME 64
#define MAX_DOCS 1000
#define MAX_TITLE_SIZE 190
#define MAX_AUTHOR_SIZE 190
#define MAX_YEAR_SIZE 5
#define MAX_PATH_SIZE 64
#define MAX_KEYWORD_SIZE 32


#define REQUEST_PIPE "tmp/request_pipe"
#define FIFO_SERVER "tmp/server_fifo"
#define FIFO_CLIENT "tmp/client_fifo"
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
    char operacao;
    char title[MAX_TITLE_SIZE];
    char authors[MAX_AUTHOR_SIZE];
    char year[MAX_YEAR_SIZE];
    char path[MAX_PATH_SIZE];
    int key;
    int lines;
    char keyword[MAX_KEYWORD_SIZE];
    int n_procs;
} MensagemCliente;

int conta_linhas_com_palavra(const char *filepath, const char *keyword);

#endif
