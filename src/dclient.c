#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "common.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <operacao> [dados...]\n", argv[0]);
        return 1;
    }

    pid_t pid = getpid();

    // Criar o FIFO de resposta específico deste cliente
    char fifo_resposta[64];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pid);
    mkfifo(fifo_resposta, 0666);

    // Preparar mensagem
    MensagemCliente pedido;
    pedido.pid = pid;

    strncpy(pedido.operacao, argv[1], MAX_OP - 1);
    pedido.operacao[MAX_OP - 1] = '\0'; 
    
    if (argc > 2) {
        snprintf(pedido.dados, MAX_DADOS, "%s", argv[2]);  // simplificado para um argumento
    } else {
        pedido.dados[0] = '\0';
    }

    // Abrir FIFO de pedido (comunicação com servidor)
    int fd_request = open(REQUEST_PIPE, O_WRONLY);
    if (fd_request == -1) {
        perror("Erro ao abrir pipe de pedido");
        unlink(fifo_resposta);
        return 1;
    }

    write(fd_request, &pedido, sizeof(MensagemCliente));
    close(fd_request);

    // Esperar resposta no FIFO pessoal
    int fd_resposta = open(fifo_resposta, O_RDONLY);
    if (fd_resposta == -1) {
        perror("Erro ao abrir pipe de resposta");
        unlink(fifo_resposta);
        return 1;
    }

    char buffer[512];
    int n = read(fd_resposta, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Resposta do servidor: %s\n", buffer);
    }

    close(fd_resposta);
    unlink(fifo_resposta);

    return 0;
}
