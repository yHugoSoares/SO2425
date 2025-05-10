#ifndef COMMON_H
#define COMMON_H

#include "cache.h"

#define MAX_TITLE_SIZE 190
#define MAX_AUTHOR_SIZE 190
#define MAX_YEAR_SIZE 5
#define MAX_PATH_SIZE 64
#define MAX_KEYWORD_SIZE 100
#define MAX_ENTRIES 256
#define MAX_FIFO_NAME 128

#define METADATA_FILE "/metadata.dat"
#define PIPE "/tmp/pipe"


typedef struct {
    int pid; // PID do cliente
    int dirty; // Flag para indicar se o cache está sujo
    char operacao; // Operação a realizar
    int key; // Chave do documento
    char keyword[100]; // Palavra-chave para pesquisa
    char n_procs; // Número de processos para pesquisa
    char title[MAX_TITLE_SIZE]; // Título do documento
    char authors[MAX_AUTHOR_SIZE]; // Autores do documento
    char year[MAX_YEAR_SIZE]; // Ano do documento
    char path[MAX_PATH_SIZE]; // Caminho do documento
} Pedido;

typedef struct {
    Entry entry[MAX_ENTRIES] ; // Array de entradas
    int count; // Contador de documentos
    int dirty; // Flag para indicar se o índice está sujo
    int last_id; // Último ID usado
} Metadata;


void save_metadata(Metadata metadata);
void load_metadata(Metadata metadata);
int conta_linhas_com_palavra(const char *filepath, const char *keyword);

#endif // COMMON_H