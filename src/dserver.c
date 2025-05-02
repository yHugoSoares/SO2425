#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "common.h"

int main(int argc, char *argv[]) {
    // Criar o FIFO de pedidos (só precisa de ser feito uma vez)
    mkfifo(REQUEST_PIPE, 0666);

    printf("Servidor iniciado. À escuta de pedidos...\n");

    int fd_request = open(REQUEST_PIPE, O_RDONLY);
    if (fd_request == -1) {
        perror("Erro ao abrir pipe de pedido");
        return 1;
    }

    MensagemCliente pedido;
    while (1) {
        ssize_t bytes = read(fd_request, &pedido, sizeof(MensagemCliente));
        if (bytes <= 0) {
            continue;
        }

        printf("Recebido pedido do cliente %d: operação '%c'\n", pedido.pid, pedido.operacao);

        // Criar resposta (aqui só respondemos algo simples, para testar)
        char resposta[512];
        snprintf(resposta, sizeof(resposta),
                 "Pedido recebido: operação '%c', dados: %s",
                 pedido.operacao, pedido.dados);

        // Criar caminho do FIFO de resposta
        char fifo_resposta[64];
        snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pedido.pid);

        // Enviar resposta ao cliente
        int fd_resposta = open(fifo_resposta, O_WRONLY);
        if (fd_resposta != -1) {
            write(fd_resposta, resposta, strlen(resposta) + 1);
            close(fd_resposta);
        } else {
            perror("Erro ao abrir pipe de resposta");
        }
    }

    close(fd_request);
    unlink(REQUEST_PIPE);

    return 0;
}
