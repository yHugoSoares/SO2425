#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "common.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <operacao> [args...]\n", argv[0]);
        return 1;
    }

    pid_t pid = getpid();
    Pedido pedido;
    memset(&pedido, 0, sizeof(Pedido));
    pedido.pid = pid;
    pedido.operacao = argv[1][1];  

    // Preencher os campos dependendo da operação
    switch (pedido.operacao) {
        case 'f': // Finalizar
            pedido.operacao = 'f'; // Finalizar
            break;
        case 'a': // Adicionar documento
            if (argc != 6) {
                fprintf(stderr, "Uso: %s -a \"title\" \"authors\" \"year\" \"path\"\n", argv[0]);
                return 1;
            }
            strncpy(pedido.title, argv[2], MAX_TITLE_SIZE - 1);
            pedido.title[MAX_TITLE_SIZE - 1] = '\0';
            
            strncpy(pedido.authors, argv[3], MAX_AUTHOR_SIZE - 1);
            pedido.authors[MAX_AUTHOR_SIZE - 1] = '\0';
            
            strncpy(pedido.year, argv[4], MAX_YEAR_SIZE - 1);
            pedido.year[MAX_YEAR_SIZE - 1] = '\0';
            
            strncpy(pedido.path, argv[5], MAX_PATH_SIZE - 1);
            pedido.path[MAX_PATH_SIZE - 1] = '\0';
                        break;

        case 'c': // Listar documentos
            if (argc != 3) {
                fprintf(stderr, "Uso: %s -c <document_key>\n", argv[0]);
                return 1;
            }
            pedido.key = atoi(argv[2]);
            break;

        case 'd': // Remover documento
            if (argc != 3) {
                fprintf(stderr, "Uso: %s -d <document_key>\n", argv[0]);
                return 1;
            }
            pedido.key = atoi(argv[2]);
            break;

        case 'l': // Contar linhas
            if(argc != 4){
                fprintf(stderr, "Uso: %s -l <document_key> <keyword>\n",argv[0]);
                return 1;
            }
            pedido.key = atoi(argv[2]);
            strncpy(pedido.keyword, argv[3], MAX_KEYWORD_SIZE - 1);
            pedido.keyword[MAX_KEYWORD_SIZE - 1] = '\0';
            break;

        case 's': // Procurar por palavra-chave
            if (argc != 4) {
                fprintf(stderr, "Uso: %s -s <keyword> <nr_processos>\n", argv[0]);
                return 1;
            }
            strncpy(pedido.keyword, argv[2], MAX_KEYWORD_SIZE - 1);
            pedido.keyword[MAX_KEYWORD_SIZE - 1] = '\0';
            pedido.n_procs = atoi(argv[3]);
            
            break;
        
        default:
            fprintf(stderr, "Operação inválida: %s\n", argv[1]);
            return 1;
    }

    // Abrir FIFO de pedido (comunicação com servidor)
    int fd_request = open(REQUEST_PIPE, O_WRONLY);
    if (fd_request == -1) {
        perror("Erro ao abrir pipe de pedido");
        return 1;
    }

    // Criar FIFO de resposta após abrir request
    char fifo_resposta[64];
    snprintf(fifo_resposta, sizeof(fifo_resposta), "/tmp/response_pipe_%d", pid);
    mkfifo(fifo_resposta, 0666);

    // Enviar pedido
    write(fd_request, &pedido, sizeof(Pedido));
    close(fd_request);

    // Ler resposta
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
        printf("Resposta do servidor:\n%s", buffer);
        printf("\n");
    }

    close(fd_resposta);
    unlink(fifo_resposta);

    return 0;
}
