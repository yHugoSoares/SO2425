#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "handle_request.h"
#include "common.h"

int main(int argc, char *argv[]) {
    mkfifo(REQUEST_PIPE, 0666);
    printf("Servidor iniciado. À escuta de pedidos...\n");

    int fd_request = open(REQUEST_PIPE, O_RDONLY, O_WRONLY);
    if (fd_request == -1) {
        perror("Erro ao abrir pipe de pedido");
        return 1;
    }

    MensagemCliente pedido;

    printf("%s %s %s %s", pedido.authors, pedido.title, pedido.year, pedido.path);
    int running = 1;

    while (running) {
        ssize_t bytes = read(fd_request, &pedido, sizeof(MensagemCliente));
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
