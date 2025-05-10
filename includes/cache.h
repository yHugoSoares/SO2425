#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int key; // Chave do documento
    char title[100]; // Título do documento
    char authors[100]; // Autores do documento
    char year[5]; // Ano do documento
    char path[64]; // Caminho do documento
} Entry;

typedef struct {
    Entry *entry; // Entrada do cache
    int key; // Chave do cache
} CachePage;

typedef struct {
    int size; // Tamanho do cache
    int count; // Contador de entradas no cache
    CachePage **pages; // Páginas do cache
} Cache;

void cache_init(int size);
void cache_destroy();
int cache_add_entry(int key, Entry entry);


#endif // CACHE_H