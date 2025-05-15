#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "index.h"
#include "cache.h"
#include "common.h"  // Adicionando a inclusão do common.h que contém a definição de METADATA_FILE

int Next_key = 0;

int index_get_next_key() {
    return Next_key++;
}

Entry *create_index_entry(char *title, char *authors, char *year, char *path, int delete_flag) {
    Entry *entry = malloc(sizeof(Entry));
    if (entry == NULL) {
        perror("Failed to allocate memory for index entry");
        return NULL;
    }
    entry->delete_flag = 0;
    strcpy(entry->Title, title);
    strcpy(entry->authors, authors);
    strcpy(entry->year, year);
    strcpy(entry->path, path);
    return entry;
}

int destroy_index_entry(Entry *entry) {
    if (entry == NULL) {
        return -1;
    }
    free(entry);
    return 0;
}

int index_load_file_to_cache() {
    // Inicializa Next_key como 0 por padrão
    Next_key = 0;
    
    // Verificar se o arquivo existe
    int index_fd = open(METADATA_FILE, O_RDONLY);
    if (index_fd == -1) {
        printf("Arquivo de metadados não encontrado. Iniciando com Next_key = 0\n");
        return 0;  // Retorna sucesso, já que começar com Next_key = 0 é válido
    }

    // Verifica se o arquivo está vazio
    off_t file_size = lseek(index_fd, 0, SEEK_END);
    if (file_size == 0) {
        printf("Arquivo de metadados vazio. Iniciando com Next_key = 0\n");
        close(index_fd);
        return 0;  // Arquivo vazio, Next_key permanece 0
    }
    
    printf("Carregando entradas do arquivo de metadados para a cache...\n");
    
    // Volta ao início do arquivo
    lseek(index_fd, 0, SEEK_SET);

    Entry entry;
    int max_key = -1;  // Para rastrear o maior índice encontrado
    int count = 0;
    
    // Lê todas as entradas do arquivo
    while (read(index_fd, &entry, sizeof(Entry)) == sizeof(Entry)) {
        count++;
        
        // Pula entradas marcadas como excluídas
        if (entry.delete_flag) {
            continue;
        }

        // Cria uma cópia da entrada para o cache
        Entry *entry_copy = malloc(sizeof(Entry));
        if (!entry_copy) {
            perror("Failed to allocate memory for index entry");
            continue;
        }
        memcpy(entry_copy, &entry, sizeof(Entry));

        // Adiciona a entrada ao cache com o índice atual
        int key = Next_key++;
        if (cache_add_entry(key, entry_copy, 0) != 0) {
            fprintf(stderr, "Failed to add entry with key %d to cache\n", key);
            free(entry_copy);
        } else {
            printf("Entrada carregada para a cache: key=%d, path=%s\n", key, entry_copy->path);
            
            // Atualiza o maior índice encontrado
            if (key > max_key) {
                max_key = key;
            }
        }
    }

    // Garante que Next_key seja pelo menos o maior índice + 1
    if (Next_key <= max_key) {
        Next_key = max_key + 1;
    }
    
    printf("Carregadas %d entradas do arquivo de metadados\n", count);
    printf("Next_key inicializado como %d\n", Next_key);
    
    close(index_fd);
    return 0;
}

int index_add_entry(Entry *entry) {
    // Open the index file for writing
    int index_fd = open(METADATA_FILE, O_APPEND | O_CREAT | O_WRONLY, 0600);
    if (index_fd == -1) {
        perror("Failed to open index file");
        return -1;
    }

    // Write the entry to the file
    if (write(index_fd, entry, sizeof(Entry)) <= 0) {
        perror("Failed to write index entry to file");
        close(index_fd);
        return -1;
    }

    close(index_fd);

    // Do not destroy the entry here; the caller is responsible for managing its memory
    return Next_key++;
}

int index_write_dirty_entry(Entry *entry, int key) {
    if (!entry) {
        fprintf(stderr, "Erro: entrada nula passada para index_write_dirty_entry\n");
        return -1;
    }
    // Open the index file for reading and writing
    int index_fd = open(METADATA_FILE, O_RDWR | O_CREAT, 0600);
    if (index_fd == -1) {
        perror("Failed to open index file");
        return -1;
    }

    // Calculate the offset for the entry
    off_t offset = key * sizeof(Entry);
    if (lseek(index_fd, offset, SEEK_SET) == -1) {
        perror("Failed to seek to index entry");
        close(index_fd);
        return -1;
    }

    // Write the entry to the file
    if (write(index_fd, entry, sizeof(Entry)) <= 0) {
        perror("Failed to write index entry to file");
        close(index_fd);
        return -1;
    }

    close(index_fd);
    return 0;
}

Entry *index_get_entry(int key) {
    // Check if key is valid
    if (key < 0 || key >= Next_key) {
        return NULL;
    }

    // Open the index file for reading
    int index_fd = open(METADATA_FILE, O_RDONLY);
    if (index_fd == -1) {
        perror("Failed to open index file");
        return NULL;
    }

    // Calculate the offset for the entry
    off_t offset = key * sizeof(Entry);
    if (lseek(index_fd, offset, SEEK_SET) == -1) {
        perror("Failed to seek to index entry");
        close(index_fd);
        return NULL;
    }

    // Read the entry from the file
    Entry *entry = malloc(sizeof(Entry));
    if (entry == NULL) {
        perror("Failed to allocate memory for index entry");
        close(index_fd);
        return NULL;
    }

    ssize_t bytesRead = read(index_fd, entry, sizeof(Entry));

    if (bytesRead == 0) {
        free(entry);
        entry = NULL;
    } else if (bytesRead < 0) {
        perror("Failed to read index entry from file");
        free(entry);
        entry = NULL;
    }

    close(index_fd);
    return entry;
}

int index_delete_entry(int key) {
    // Get the index entry
    Entry *entry = index_get_entry(key);

    // Check if the entry was found
    if (entry == NULL) {
        return -1;
    }

    // Check if the entry is already marked as deleted
    if (entry->delete_flag == 1) {
        free(entry);
        return -1;
    }

    // Mark the entry as deleted
    entry->delete_flag = 1;

    // Open the index file for writing
    int index_fd = open(METADATA_FILE, O_WRONLY);
    if (index_fd == -1) {
        perror("Failed to open index file");
        free(entry);
        return -1;
    }

    // Calculate the offset for the entry
    off_t offset = key * sizeof(Entry);
    if (lseek(index_fd, offset, SEEK_SET) == -1) {
        perror("Failed to seek to index entry");
        free(entry);
        close(index_fd);
        return -1;
    }

    // Write the updated entry back to the file
    if (write(index_fd, entry, sizeof(Entry)) <= 0) {
        perror("Failed to write index entry to file");
        free(entry);
        close(index_fd);
        return -1;
    }

    destroy_index_entry(entry);
    close(index_fd);
    return 0;
}
