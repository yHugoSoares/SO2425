#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "common.h"
#include "cache.h"
#include "index.h"

#define MAX_PROCESS 10
Metadata metadata;
extern Cache *global_cache;

void get_from_cache_to_metadata() {
    if (!global_cache) {
        fprintf(stderr, "Cache not initialized\n");
        return;
    }

    // Reset metadata
    metadata.count = 0;

    // Iterate through all cache pages
    for (int i = 0; i < global_cache->count; i++) {
        CachePage *page = global_cache->pages[i];
        if (!page || !page->entry) {
            continue;
        }

        // Populate metadata with cache entry
        Document *doc = &metadata.docs[metadata.count++];
        doc->id = page->key;
        strncpy(doc->title, page->entry->Title, sizeof(doc->title) - 1);
        doc->title[sizeof(doc->title) - 1] = '\0';
        strncpy(doc->authors, page->entry->authors, sizeof(doc->authors) - 1);
        doc->authors[sizeof(doc->authors) - 1] = '\0';
        strncpy(doc->year, page->entry->year, sizeof(doc->year) - 1);
        doc->year[sizeof(doc->year) - 1] = '\0';
        strncpy(doc->path, page->entry->path, sizeof(doc->path) - 1);
        doc->path[sizeof(doc->path) - 1] = '\0';

        // Ensure metadata does not exceed its maximum capacity
        if (metadata.count >= MAX_DOCS) {
            fprintf(stderr, "Metadata capacity reached. Some entries may not be transferred from cache.\n");
            break;
        }
    }
}

// NOT FULLY CORRECT
void save_metadata() {
    int fd = open(METADATA_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("save_metadata");
        return;
    }

    get_from_cache_to_metadata();

    ssize_t written = write(fd, &metadata, sizeof(metadata));
    if (written != sizeof(metadata)) {
        perror("Error writing metadata");
    }

    close(fd);
}

void load_metadata() {
    int fd = open(METADATA_FILE, O_RDONLY);
    if (fd == -1) {
        metadata.count = 0;
        metadata.last_id = 0;
        return;
    }
    read(fd, &metadata, sizeof(Metadata));
    close(fd);
}

int handle_shutdown(MensagemCliente pedido) {
    char resposta[] = "Servidor a terminar...";

    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pedido.pid);

    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } else {
        perror("Erro ao abrir FIFO de resposta");
    }
    save_metadata();

    return 0;  
}

int handle_add(MensagemCliente pedido) {
    char resposta[256];

    // Create a new metadata entry
    IndexEntry *entry = create_index_entry(pedido.title, pedido.authors, pedido.year, pedido.path, 0);
    if (!entry) {
        snprintf(resposta, sizeof(resposta), "Erro: falha ao criar entrada de metadados.");
    } else if (cache_add_entry(index_get_next_key(), entry, 1) != 0) {
        snprintf(resposta, sizeof(resposta), "Erro: falha ao adicionar entrada ao cache.");
        destroy_index_entry(entry);
    } else {
        snprintf(resposta, sizeof(resposta), "Documento indexado com sucesso.");
    }

    // Send response to client
    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pedido.pid);
    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } else {
        perror("Erro ao abrir FIFO de resposta");
    }

    return 1;
}

int handle_consulta(MensagemCliente pedido) {
    char resposta[512];
    int found = 0;

    for (int i = 0; i < metadata.count; i++) {
        if (metadata.docs[i].id == pedido.key) {
            snprintf(resposta, sizeof(resposta),
                     "Title: %s\nAuthors: %s\nYear: %s\nPath: %s",
                     metadata.docs[i].title,
                     metadata.docs[i].authors,
                     metadata.docs[i].year,
                     metadata.docs[i].path);
            found = 1;
            break;
        }
    }

    if (!found) {
        snprintf(resposta, sizeof(resposta), "Documento com ID %d não encontrado.", pedido.key);
    }

    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pedido.pid);
    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } else {
        perror("Erro ao abrir FIFO de resposta");
    }

    return 1;
}

int handle_remove(MensagemCliente pedido) {
    char resposta[256];
    int found = 0;

    for (int i = 0; i < metadata.count; i++) {
        if (metadata.docs[i].id == pedido.key) {
            for (int j = i; j < metadata.count - 1; j++) {
                metadata.docs[j] = metadata.docs[j + 1];
            }
            metadata.count--;
            save_metadata();
            found = 1;
            break;
        }
    }

    if (found) {
        snprintf(resposta, sizeof(resposta), "Document %d removed.", pedido.key);
    } else {
        snprintf(resposta, sizeof(resposta), "Document %d not found", pedido.key);
    }

    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pedido.pid);
    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } else {
        perror("Erro ao abrir FIFO de resposta");
    }

    return 1;
}


int handle_lines_number(MensagemCliente pedido, const char *document_folder){
    char resposta[256];
    //int lines_number = 0;
    int found = 0;// função para retornar o numero de linhas que contêm uma dada palavra
    for (int i = 0; i< metadata.count;i++){
        int resp;
        if(metadata.docs[i].id== pedido.key){
            if (metadata.docs[i].path[0] != '\0'){
                resp= conta_linhas_com_palavra(metadata.docs[i].path,pedido.keyword);
                found=1;
            }
        }
        if (found == 0){
            snprintf(resposta, sizeof(resposta), "Document %d not found.", pedido.key);
        }
        else{
            snprintf(resposta, sizeof(resposta), "Document %d found in %d lines.", pedido.key, resp);
        }
        break;
    }
    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d",pedido.pid);
    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } 
    else {
        perror("Erro ao abrir FIFO de resposta");
    }
    return 1;
}


int handle_search(MensagemCliente pedido, const char *document_folder ){
// função que devolve os ficheiros onde uma determinada palavra esta presente
char resposta[256];
int found = 0;
int ativos=0;
for (int i = 0; i< metadata.count;i++){
    int fd[2];
    if(pipe(fd)==-1){
        perror("Erro ao criar pipe");
        return 0;
    }
    if(ativos>= MAX_PROCESS){
        int status;
        wait(&status);
        ativos--;
    }
    pid_t pid = fork();
    if(pid==-1){
        perror("Erro ao criar processo");
        close(fd[0]);
        close(fd[1]);
        return 0;
    }
    if(pid==0){
        ativos++;
        close(fd[0]);
        char file[256];
        snprintf(file, sizeof(file), "%s/%s", document_folder, metadata.docs[i].path);
        int res= execlp("grep","grep","-q",pedido.keyword,file,NULL);
        if(res==-1){
            perror("Erro ao executar grep");
            close(fd[1]);
            exit(1);
        }
        _exit(EXIT_FAILURE);
        }
        else{
            close(fd[1]);
            char buffer[256];
            int bytes_read = read(fd[0], buffer, sizeof(buffer)-1);
            if(bytes_read==-1){
                perror("Erro ao ler pipe");
                return 0;
            }
            int status;
            waitpid(pid, &status, 0);
            ativos--;
            if(WIFEXITED(status)&& WEXITSTATUS(status)==0){
                found++;
                strcat(resposta,metadata.docs[i].path);
                strcat(resposta,"\n");
            }
        }
        
    }
    while(ativos>0){
        wait(NULL);
        ativos--;
    }

    if(found==0){
        stpcpy(resposta,"Nenhum documento possui a palavra pedida");
    }
    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d",pedido.pid);
    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } 
    else {
        perror("Erro ao abrir FIFO de resposta");
    }
    return 1;
}


int handle_request(MensagemCliente pedido, const char *document_folder) {
    switch (pedido.operacao) {
        case 'f':
            return handle_shutdown(pedido);
        case 'a':
            return handle_add(pedido);
        case 'c':
            return handle_consulta(pedido);
        case 'd':
            return handle_remove(pedido);
        case 'l':
            return handle_lines_number(pedido, document_folder); // por fazer os testes e comprovar
        case 's':
            return handle_search(pedido, document_folder); // por fazer os testes e comprovar devido a não conseguir anexar os ficheiros
        default:
            fprintf(stderr, "Operação desconhecida.");
            return 1;
    }
}
