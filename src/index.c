#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "index.h"

int Next_key = 0;

int index_get_next_key() {
    return Next_key++;
}

IndexEntry *create_index_entry(char *title, char *authors, char *year, char *path, int delete_flag) {
    IndexEntry *entry = malloc(sizeof(struct index_entry));
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

int destroy_index_entry(IndexEntry *entry) {
    if (entry == NULL) {
        return -1;
    }
    free(entry);
    return 0;
}

int index_add_entry(IndexEntry *entry) {

    // Open the index file for writing
    int index_fd = open(INDEX_FILE, O_APPEND | O_CREAT, 0600);
    if (index_fd == -1) {
        perror("Failed to open index file");
        destroy_index_entry(entry);
        return -1;
    }

    // Write the entry to the file
    if (write(index_fd, entry, sizeof(struct index_entry)) <= 0) {
        perror("Failed to write index entry to file");
        close(index_fd);
        destroy_index_entry(entry);
        return -1;
    }

    destroy_index_entry(entry);
    return Next_key++;
}

int index_write_dirty_entry(IndexEntry *entry, int key) {

    // Open the index file for writing
    int index_fd = open(INDEX_FILE, O_WRONLY | O_CREAT, 0600);
    if (index_fd == -1) {
        perror("Failed to open index file");
        destroy_index_entry(entry);
        return -1;
    }

    // Calculate the offset for the entry
    off_t offset = key * sizeof(struct index_entry);
    if (lseek(index_fd, offset, SEEK_SET) == -1) {
        perror("Failed to seek to index entry");
        close(index_fd);
        return -1;
    }

    // Write the entry to the file
    if (write(index_fd, entry, sizeof(struct index_entry)) <= 0) {
        perror("Failed to write index entry to file");
        close(index_fd);
        destroy_index_entry(entry);
        return -1;
    }

    return 0;
}

IndexEntry *index_get_entry(int key) {

    // Check if key is valid
    if (key < 0 || key >= Next_key) {
        return NULL;
    }

    // Open the index file for reading
    int index_fd = open(INDEX_FILE, O_RDONLY);
    if (index_fd == -1) {
        perror("Failed to open index file");
        return NULL;
    }

    // Calculate the offset for the entry
    off_t offset = key * sizeof(struct index_entry);
    if (lseek(index_fd, offset, SEEK_SET) == -1) {
        perror("Failed to seek to index entry");
        close(index_fd);
        return NULL;
    }

    // Read the entry from the file
    IndexEntry *entry = malloc(sizeof(struct index_entry));
    if (entry == NULL) {
        perror("Failed to allocate memory for index entry");
        close(index_fd);
        return NULL;
    }

    ssize_t bytesRead = read(index_fd, entry, sizeof(struct index_entry));

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

int index_delete_entry(int key){

    // Get the index entry
    IndexEntry *entry = index_get_entry(key);

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
    int index_fd = open(INDEX_FILE, O_WRONLY);
    if (index_fd == -1) {
        perror("Failed to open index file");
        free(entry);
        return -1;
    }

    // Calculate the offset for the entry
    off_t offset = key * sizeof(struct index_entry);
    if (lseek(index_fd, offset, SEEK_SET) == -1) {
        perror("Failed to seek to index entry");
        free(entry);
        close(index_fd);
        return -1;
    }

    // Write the updated entry back to the file
    if (write(index_fd, entry, sizeof(struct index_entry)) <= 0) {
        perror("Failed to write index entry to file");
        free(entry);
        close(index_fd);
        return -1;
    }

    destroy_index_entry(entry);
    close(index_fd);
    return 0;
}














int IndexGetKey(){
    //Verificar se o arquivo Index existe
    int IndexFile = open(INDEX_FILE, O_RDONLY | O_CREAT| O_APPEND, 0600);
    if(IndexFile == -1){
        //Erro ao abrir o arquivo
        perror("Erro ao abrir o arquivo de índice");
        return -1;
    }

    // -- Calcular Offset do documento -- //
    off_t offset = lseek(IndexFile, 0, SEEK_END);
    if (offset == -1) {
        perror("Erro ao obter o offset do documento");
        close(IndexFile);
        return -1;
    }

    close(IndexFile);
    return offset / sizeof(struct indexPackage); // Retorna a chave do próximo documento
}



int IndexAddManager(IndexPack argument,int key){

    //Verificar se o arquivo Index existe 
    int IndexFile = open(INDEX_FILE, O_WRONLY | O_CREAT, 0600); 
    
    if(IndexFile == -1){
        //Erro ao abrir o arquivo
        perror("Erro ao abrir o arquivo de índice");
        return -1;
    }
    
    // -- Calcular Offset do documento -- //
    off_t offset = key * sizeof(struct indexPackage);
    off_t offsetSeek = lseek(IndexFile, offset, SEEK_SET);
    if (offsetSeek == -1) {
        close(IndexFile);
        return -1;
    }


    size_t bytesWritten = write(IndexFile, argument, sizeof(struct indexPackage));

    if(bytesWritten == -1){
        //Erro ao escrever no arquivo
        perror("Erro ao escrever no arquivo de índice");
        return -1;
    }

    close(IndexFile);
    return key; // Escrita bem sucedida
}





IndexPack IndexConsultManager(int key){
    
    //Verificar se o arquivo Index existe
    int IndexFile = open(INDEX_FILE, O_RDONLY , 0600);
    if(IndexFile == -1){
        //Erro ao abrir o arquivo
        perror("Erro ao abrir o arquivo de índice");
        return NULL;
    }

    // -- Calcular Offset do documento -- //
    off_t offset = key * sizeof(struct indexPackage);
    off_t offsetSeek = lseek(IndexFile, offset, SEEK_SET);
    if (offsetSeek == -1) {
        perror("Erro ao obter o offset do documento");
        close(IndexFile);
        return NULL;
    }

    void* pack = malloc(sizeof(struct indexPackage));
    if(pack == NULL) {
        perror("Failed to allocate memory for the pack");
    }
    ssize_t bytesRead = read(IndexFile, pack, sizeof(struct indexPackage));
    if(bytesRead == -1){
        //Erro ao ler o arquivo
        perror("Erro ao ler o arquivo de índice");
        free(pack);
        close(IndexFile);
        return NULL;
    }

    close(IndexFile);

    return (IndexPack) pack;
}


int IndexDeleteManager(int key,IndexPack BlankPackage){

    //Verificar se o arquivo Index existe
    int IndexFile = open(INDEX_FILE, O_WRONLY, 0600);
    if(IndexFile == -1){
        //Erro ao abrir o arquivo
        perror("Erro ao abrir o arquivo de índice");
        return -1;
    }

    // -- Calcular Offset do documento -- //
    off_t offset = key * sizeof(struct indexPackage);
    off_t offsetSeek = lseek(IndexFile, offset, SEEK_SET);
    if (offsetSeek == -1) {
        perror("Erro ao obter o offset do documento");
        close(IndexFile);
        return -1;
    }

    //Remover o documento
    if(write(IndexFile, BlankPackage, sizeof(struct indexPackage)) < 0) {
        perror("Failed to write empty block");
        return -1;
    }
    close(IndexFile);

    return 0;

}