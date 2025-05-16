#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include "cache.h"
#include "handle_request.h"
#include "common.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <document_folder> <cache_size>\n", argv[0]);
        return 1;
    }

    // Verificar se a pasta de documentos existe
    struct stat st;
    if (stat(argv[1], &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Erro: '%s' não é um diretório válido ou não existe\n", argv[1]);
        return 1;
    }
    
    // printf("Pasta de documentos: %s\n", argv[1]);
    
    // // Listar alguns arquivos no diretório para verificar
    // printf("Verificando arquivos no diretório...\n");
    // char command[512];
    // snprintf(command, sizeof(command), "ls -la %s | head -5", argv[1]);
    // system(command);

    // Criar FIFO se não existir
    unlink(REQUEST_PIPE); // Remove o FIFO se já existir
    if (mkfifo(REQUEST_PIPE, 0666) != 0) {
        perror("Erro ao criar FIFO");
        return 1;
    }
    
    printf("Servidor iniciado. À escuta de pedidos...\n");

    // Abrir o FIFO de pedidos apenas para leitura
    int fd_request = open(REQUEST_PIPE, O_RDONLY);
    if (fd_request == -1) {
        perror("Erro ao abrir pipe de pedido");
        return 1;
    }

    // Inicializar cache
    int cache_size = atoi(argv[2]);
    if (cache_init(cache_size) != 0) {
        fprintf(stderr, "Falha ao inicializar cache\n");
        return 1;
    }
    
    printf("Cache inicializada com tamanho %d\n", cache_size);

    // Carregar index para a cache
    if (index_load_file_to_cache() != 0) {
        fprintf(stderr, "Falha ao carregar ficheiro index para a cache\n");
    }

    Pedido pedido;
    int running = 1;

    while (running) {
        ssize_t bytes = read(fd_request, &pedido, sizeof(Pedido));
        if (bytes <= 0) {
            continue;
        }

        printf("Recebido pedido do cliente %d: operação '%c'\n", pedido.pid, pedido.operacao);
        
        // Se for operação 'a', mostrar o caminho do arquivo
        if (pedido.operacao == 'a') {
            printf("Adicionando arquivo: %s\n", pedido.path);
            printf("Caminho completo: %s/%s\n", argv[1], pedido.path);
        }

        // Operações demoradas tratadas por filhos
        if (pedido.operacao == 's' || pedido.operacao == 'l') {
            pid_t pid = fork();
            if (pid == -1) {
                perror("Erro ao criar processo filho");
                continue;
            }

            if (pid == 0) {
                // Filho trata a operação e termina
                handle_request(pedido, argv[1]);  // document_folder
                exit(0);
            }

            // Pai continua
            continue;
        }

        // Operações rápidas tratadas pelo pai
        if (pedido.operacao == 'f') {
            // Se for operação de finalização, salva a cache e termina
            if (!handle_request(pedido, argv[1])) {
                running = 0;
            }
        } else {
            // Outras operações
            handle_request(pedido, argv[1]);
        }
    }

    close(fd_request);
    unlink(REQUEST_PIPE);
    return 0;
}
