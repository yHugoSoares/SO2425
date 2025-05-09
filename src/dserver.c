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
    mkfifo(REQUEST_PIPE, 0666);
    printf("Servidor iniciado. À escuta de pedidos...\n");

    int fd_request = open(REQUEST_PIPE, O_RDONLY, O_WRONLY);
    if (fd_request == -1) {
        perror("Erro ao abrir pipe de pedido");
        return 1;
    }

    Pedido pedido;
    int cache_size = atoi(argv[2]);
    cache_init(cache_size);

    // Load the index file into the cache
    if (index_load_file_to_cache() != 0) {
        fprintf(stderr, "Failed to load index file into cache\n");
    }

    int running = 1;

    while (running) {
        ssize_t bytes = read(fd_request, &pedido, sizeof(Pedido));
        if (bytes <= 0) {
            continue;
        }

        printf("Recebido pedido do cliente %d: operação '%c'\n", pedido.pid, pedido.operacao);

        if (!handle_request(pedido, METADATA_FILE)) {
            running = 0;
        }
    }

    close(fd_request);
    unlink(REQUEST_PIPE);

    return 0;
}


// receber ver o que esta ver a operacao fazer e enviar para o cliente add remove e consulte tudo feito pelo pai 
// s e l depois de saber que sao essas operacoes crio um filho que fica responsavel para usar outros filhos 
// para fazer esses pesquisas e enviar resposta 