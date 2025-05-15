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
extern Cache *global_cache;

// Função para verificar se um arquivo existe
int file_exists(const char *path) {
    struct stat buffer;
    int result = stat(path, &buffer);
    if (result == 0) {
        printf("Arquivo encontrado: %s\n", path);
        return 1; // Arquivo existe
    } else {
        printf("Arquivo NÃO encontrado: %s (erro: %s)\n", path, strerror(errno));
        return 0; // Arquivo não existe
    }
}

// Função para salvar toda a cache no arquivo de metadados
void save_cache_to_metadata() {
    if (!global_cache) return;
    
    printf("Salvando cache no arquivo de metadados...\n");
    
    // Abre o arquivo para escrita (trunca o arquivo existente)
    FILE *file = fopen(METADATA_FILE, "wb");
    if (!file) {
        perror("Erro ao abrir arquivo de metadados para escrita");
        return;
    }
    
    // Percorre todas as páginas da cache
    for (int i = 0; i < global_cache->count; i++) {
        if (global_cache->pages[i] && global_cache->pages[i]->entry) {
            // Escreve a entrada no arquivo
            fwrite(global_cache->pages[i]->entry, sizeof(Entry), 1, file);
            printf("Salvando entrada %d no arquivo\n", global_cache->pages[i]->key);
        }
    }
    
    fclose(file);
    printf("Cache salva com sucesso!\n");
}

int handle_shutdown(Pedido pedido) {
    char resposta[] = "Server is shutting down...";

    // Salva a cache no arquivo de metadados antes de desligar
    save_cache_to_metadata();

    char fifo_resposta[MAX_FIFO_NAME];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pedido.pid);

    int fd_resp = open(fifo_resposta, O_WRONLY);
    if (fd_resp != -1) {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    } else {
        perror("Erro ao abrir FIFO de resposta");
    }
    
    return 0;  
}

int handle_add(Pedido pedido) {
    char resposta[256];

    int key = index_get_next_key();  // ESTE é o valor real do ID atribuído
    printf("Adicionando documento com key=%d, path=%s\n", key, pedido.path);

    Entry *entry = create_index_entry(pedido.title, pedido.authors, pedido.year, pedido.path, 0);
    if (!entry) {
        snprintf(resposta, sizeof(resposta), "Erro: falha ao criar entrada de metadados.");
    } else if (cache_add_entry(key, entry, 1) != 0) {
        snprintf(resposta, sizeof(resposta), "Erro: falha ao adicionar entrada ao cache.");
        destroy_index_entry(entry);
    } else {
        snprintf(resposta, sizeof(resposta), "Document %d indexed.", key);
    }

    // Enviar resposta
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

int handle_consulta(Pedido pedido) {
    char resposta[512];
    Entry *entry = cache_get_entry(pedido.key);

    if (!entry || entry->delete_flag) {
        snprintf(resposta, sizeof(resposta), "Document %d not found.", pedido.key);
    } else {
        snprintf(resposta, sizeof(resposta),
                 "Title: %s\nAuthors: %s\nYear: %s\nPath: %s",
                 entry->Title, entry->authors, entry->year, entry->path);
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

int handle_remove(Pedido pedido) {
    char resposta[256];

    if (!global_cache) {
        snprintf(resposta, sizeof(resposta), "Cache not initialized.");
    } else {
        // Buscar a página da cache pela key
        CachePage *page = g_hash_table_lookup(global_cache->hash_table, &pedido.key);

        if (!page || !page->entry || page->entry->delete_flag) {
            snprintf(resposta, sizeof(resposta), "Document %d not found.", pedido.key);
        } else {
            page->entry->delete_flag = 1;
            page->dirty = 1; // Marca a página como suja
            snprintf(resposta, sizeof(resposta), "Document %d marked as deleted.", pedido.key);
        }
    }

    // Enviar resposta
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

int handle_lines_number(Pedido pedido, const char *document_folder) {
    char resposta[512];
    printf("Buscando documento com key=%d\n", pedido.key);
    
    // Primeiro tenta buscar do cache
    Entry *entry = cache_get_entry(pedido.key);
    
    if (!entry) {
        printf("Documento não encontrado no cache, tentando buscar do arquivo\n");
        // Se não estiver no cache, tenta buscar do arquivo
        entry = index_get_entry(pedido.key);
    }

    if (entry && !entry->delete_flag && entry->path[0] != '\0') {
        char full_path[512];
        
        // Verifica se o caminho já é absoluto
        if (entry->path[0] == '/') {
            strncpy(full_path, entry->path, sizeof(full_path) - 1);
            full_path[sizeof(full_path) - 1] = '\0';
        } else {
            // Constrói o caminho completo usando o document_folder
            snprintf(full_path, sizeof(full_path), "%s/%s", document_folder, entry->path);
        }
        
        printf("Caminho completo do arquivo: %s\n", full_path);
        printf("Verificando se o arquivo existe...\n");
        
        // Verifica se o arquivo existe
        if (!file_exists(full_path)) {
            // Tenta encontrar o arquivo apenas com o nome, sem o caminho
            char *filename = strrchr(entry->path, '/');
            if (filename) {
                filename++; // Pula a barra
            } else {
                filename = entry->path; // Não tem barra, usa o caminho completo
            }
            
            // Constrói o caminho usando apenas o nome do arquivo
            snprintf(full_path, sizeof(full_path), "%s/%s", document_folder, filename);
            printf("Tentando caminho alternativo: %s\n", full_path);
            
            if (!file_exists(full_path)) {
                snprintf(resposta, sizeof(resposta), "Ficheiro não encontrado: %.450s", entry->path);
            } else {
                printf("Arquivo encontrado com caminho alternativo, contando linhas com a palavra '%s'\n", pedido.keyword);
                int linhas = conta_linhas_com_palavra(full_path, pedido.keyword);
                snprintf(resposta, sizeof(resposta), "Documento %d: '%s' aparece em %d linha(s).",
                         pedido.key, pedido.keyword, linhas);
            }
        } else {
            printf("Arquivo encontrado, contando linhas com a palavra '%s'\n", pedido.keyword);
            int linhas = conta_linhas_com_palavra(full_path, pedido.keyword);
            snprintf(resposta, sizeof(resposta), "Documento %d: '%s' aparece em %d linha(s).",
                     pedido.key, pedido.keyword, linhas);
        }
    } else {
        if (!entry) {
            printf("Documento %d não encontrado\n", pedido.key);
        } else if (entry->delete_flag) {
            printf("Documento %d está marcado como excluído\n", pedido.key);
        } else if (entry->path[0] == '\0') {
            printf("Documento %d tem caminho vazio\n", pedido.key);
        }
        
        snprintf(resposta, sizeof(resposta), "Documento %d não encontrado.", pedido.key);
    }

    // Enviar resposta
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

int handle_search(Pedido pedido, const char *document_folder) {
    char resposta[1024] = "";  // Increased buffer size for multiple file paths
    int found = 0;
    int active_processes = 0;

    printf("Iniciando busca por documentos com a palavra '%s'\n", pedido.keyword);
    printf("Diretório de documentos: %s\n", document_folder);
    
    // Iterate over all cache entries
    for (int i = 0; i < global_cache->count; i++) {
        if (i >= global_cache->size || !global_cache->pages[i]) continue;
        
        Entry *entry = global_cache->pages[i]->entry;
        if (!entry || entry->delete_flag) continue;
        
        char full_path[512];
        
        // Verifica se o caminho já é absoluto
        if (entry->path[0] == '/') {
            strncpy(full_path, entry->path, sizeof(full_path) - 1);
            full_path[sizeof(full_path) - 1] = '\0';
        } else {
            // Constrói o caminho completo usando o document_folder
            snprintf(full_path, sizeof(full_path), "%s/%s", document_folder, entry->path);
        }
        
        printf("Verificando arquivo: %s\n", full_path);
        
        // Verifica se o arquivo existe
        if (!file_exists(full_path)) {
            // Tenta encontrar o arquivo apenas com o nome, sem o caminho
            char *filename = strrchr(entry->path, '/');
            if (filename) {
                filename++; // Pula a barra
            } else {
                filename = entry->path; // Não tem barra, usa o caminho completo
            }
            
            // Constrói o caminho usando apenas o nome do arquivo
            snprintf(full_path, sizeof(full_path), "%s/%s", document_folder, filename);
            printf("Tentando caminho alternativo: %s\n", full_path);
            
            if (!file_exists(full_path)) {
                printf("Arquivo não encontrado, pulando...\n");
                continue;
            }
        }
        
        // Limit the number of active child processes to pedido.n_procs
        if (active_processes >= pedido.n_procs) {
            int status;
            wait(&status);  // Wait for a child process to finish
            active_processes--;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("Erro ao criar processo");
            return 0;
        }

        if (pid == 0) {
            // Child process
            // Execute grep to search for the keyword in the file
            printf("Processo filho executando grep em: %s\n", full_path);
            execlp("grep", "grep", "-q", pedido.keyword, full_path, NULL);

            // If execlp fails
            perror("Erro ao executar grep");
            _exit(EXIT_FAILURE);
        } else {
            // Parent process
            active_processes++;

            int status;
            waitpid(pid, &status, 0);  // Wait for the specific child process to finish
            active_processes--;

            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                // If grep found the keyword, add the file path to the response
                found++;
                printf("Palavra '%s' encontrada no arquivo: %s\n", pedido.keyword, full_path);
                strncat(resposta, entry->path, sizeof(resposta) - strlen(resposta) - 2);
                strncat(resposta, "\n", sizeof(resposta) - strlen(resposta) - 1);
            } else {
                printf("Palavra '%s' NÃO encontrada no arquivo: %s\n", pedido.keyword, full_path);
            }
        }
    }

    // Wait for any remaining child processes
    while (active_processes > 0) {
        wait(NULL);
        active_processes--;
    }

    // If no files were found, set an appropriate response
    if (found == 0) {
        printf("Nenhum documento possui a palavra '%s'\n", pedido.keyword);
        strncpy(resposta, "Nenhum documento possui a palavra pedida", sizeof(resposta) - 1);
        resposta[sizeof(resposta) - 1] = '\0';
    }

    // Send the response to the client
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

int handle_request(Pedido pedido, const char *document_folder) {    
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
            return handle_lines_number(pedido, document_folder);
        case 's':
            return handle_search(pedido, document_folder);
        default:
            fprintf(stderr, "Unknown operation.");
            return 1;
    }
}
