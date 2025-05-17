#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index.h"
#include "common.h"

// Carrega todos os documentos válidos do metadata.dat para um vetor
int load_metadata_to_vector(Entry *array, int max_entries) {
    int fd = open(METADATA_FILE, O_RDONLY);
    if (fd == -1) {
        perror("Erro ao abrir metadata.dat");
        return -1;
    }

    int count = 0;
    Entry temp;

    while (read(fd, &temp, sizeof(Entry)) == sizeof(Entry)) {
        if (temp.delete_flag == 0) {
            if (count >= max_entries) {
                fprintf(stderr, "Limite de entradas atingido (%d)\n", max_entries);
                break;
            }
            array[count++] = temp;
        }
    }

    close(fd);
    return count;  // Retorna o número de entradas carregadas
}
