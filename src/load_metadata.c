#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index.h"
#include "common.h"

// Carrega todos os documentos válidos do metadata.dat para um vetor
int load_metadata_to_vector(Entry *array, int max_entries) {
    FILE *file = fopen(METADATA_FILE, "rb");
    if (!file) {
        perror("Erro ao abrir metadata.dat");
        return -1;
    }

    int count = 0;
    Entry temp;

    while (fread(&temp, sizeof(Entry), 1, file) == 1) {
        if (temp.delete_flag == 0) {
            if (count >= max_entries) {
                fprintf(stderr, "Limite de entradas atingido (%d)\n", max_entries);
                break;
            }
            array[count++] = temp;
        }
    }

    fclose(file);
    return count;  // Retorna o número de entradas carregadas
}
