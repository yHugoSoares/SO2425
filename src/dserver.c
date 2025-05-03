#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "common.h"


// Função auxiliar para contar linhas com palavra usando grep
int conta_linhas_com_palavra(const char *filepath, const char *keyword) {
    char comando[512];
    snprintf(comando, sizeof(comando), "grep -c \"%s\" \"%s\" 2>/dev/null", keyword, filepath);
    FILE *fp = popen(comando, "r");
    if (!fp) return -1;

    int linhas = 0;
    fscanf(fp, "%d", &linhas);
    pclose(fp);
    return linhas;
}

int handle_request(MensagemCliente pedido, const char *document_folder) {
    char resposta[1024] = "";
    int found = 0;

    // SHUTDOWN -f
    if (strcmp(pedido.operacao, "-f") == 0) {
        snprintf(resposta, sizeof(resposta), "Servidor a terminar...");
        int fd_resp = open(pedido.resposta_fifo, O_WRONLY);
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
        return 0; // termina o servidor
    }

    // ADICIONAR DOCUMENTO -a
    if (strcmp(pedido.operacao, "-a") == 0) {
        Document doc;
        if (sscanf(pedido.dados, "%[^;];%[^;];%[^;];%s", doc.title, doc.authors, doc.year, doc.path) != 4) {
            snprintf(resposta, sizeof(resposta), "Erro: formato inválido.");
        } else {
            doc.id = ++metadata.last_id;
            metadata.docs[metadata.count++] = doc;
            save_metadata();
            snprintf(resposta, sizeof(resposta), "Document %d indexed", doc.id);
        }

    // REMOVER DOCUMENTO -d
    } else if (strcmp(pedido.operacao, "-d") == 0) {
        int id;
        if (sscanf(pedido.dados, "%d", &id) != 1) {
            snprintf(resposta, sizeof(resposta), "Erro: ID inválido.");
        } else {
            for (int i = 0; i < metadata.count; i++) {
                if (metadata.docs[i].id == id) {
                    metadata.docs[i] = metadata.docs[--metadata.count];
                    save_metadata();
                    snprintf(resposta, sizeof(resposta), "Index entry %d deleted", id);
                    found = 1;
                    break;
                }
            }
            if (!found)
                snprintf(resposta, sizeof(resposta), "ID %d não encontrado.", id);
        }

    // CONSULTAS: tratadas em processos filhos
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            // CONSULTAR DOCUMENTO -c
            if (strcmp(pedido.operacao, "-c") == 0) {
                int id;
                if (sscanf(pedido.dados, "%d", &id) == 1) {
                    for (int i = 0; i < metadata.count; i++) {
                        if (metadata.docs[i].id == id) {
                            snprintf(resposta, sizeof(resposta),
                                "Title: %s\nAuthors: %s\nYear: %s\nPath: %s",
                                metadata.docs[i].title, metadata.docs[i].authors,
                                metadata.docs[i].year, metadata.docs[i].path);
                            found = 1;
                            break;
                        }
                    }
                    if (!found)
                        snprintf(resposta, sizeof(resposta), "Documento com ID %d não encontrado.", id);
                }

            // CONTAR LINHAS COM PALAVRA -l
            } else if (strcmp(pedido.operacao, "-l") == 0) {
                int id;
                char palavra[64];
                if (sscanf(pedido.dados, "%d %s", &id, palavra) == 2) {
                    for (int i = 0; i < metadata.count; i++) {
                        if (metadata.docs[i].id == id) {
                            char caminho[256];
                            snprintf(caminho, sizeof(caminho), "%s/%s", document_folder, metadata.docs[i].path);
                            int count = conta_linhas_com_palavra(caminho, palavra);
                            snprintf(resposta, sizeof(resposta), "%d", count);
                            found = 1;
                            break;
                        }
                    }
                    if (!found)
                        snprintf(resposta, sizeof(resposta), "ID %d não encontrado.", id);
                }

            // PESQUISA COM PALAVRA -s
            } else if (strcmp(pedido.operacao, "-s") == 0) {
                char palavra[64];
                int max_processos = 1;
                sscanf(pedido.dados, "%s %d", palavra, &max_processos);

                int pipe_fds[2];
                pipe(pipe_fds);

                int filhos = 0;
                for (int i = 0; i < metadata.count; i++) {
                    if (filhos == max_processos) wait(NULL);

                    pid_t subpid = fork();
                    if (subpid == 0) {
                        char caminho[256];
                        snprintf(caminho, sizeof(caminho), "%s/%s", document_folder, metadata.docs[i].path);
                        int count = conta_linhas_com_palavra(caminho, palavra);
                        if (count > 0) {
                            dprintf(pipe_fds[1], "%d ", metadata.docs[i].id);
                        }
                        exit(0);
                    } else {
                        filhos++;
                    }
                }

                while (wait(NULL) > 0); // Esperar todos os filhos

                close(pipe_fds[1]);
                read(pipe_fds[0], resposta, sizeof(resposta));
                close(pipe_fds[0]);
            }

            // Enviar resposta
            int fd_resp = open(pedido.resposta_fifo, O_WRONLY);
            write(fd_resp, resposta, strlen(resposta) + 1);
            close(fd_resp);
            exit(0);
        } else {
            waitpid(pid, NULL, 0);
            return 1;
        }
    }

    // Resposta de operações do pai
    int fd_resp = open(pedido.resposta_fifo, O_WRONLY);
    write(fd_resp, resposta, strlen(resposta) + 1);
    close(fd_resp);
    return 1;
}

int main(int argc, char *argv[]) {
    // Criar o FIFO de pedidos (só precisa de ser feito uma vez)
    mkfifo(REQUEST_PIPE, 0666);

    printf("Servidor iniciado. À escuta de pedidos...\n");

    int fd_request = open(REQUEST_PIPE, O_RDONLY, O_WRONLY);
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

        printf("Recebido pedido do cliente %d: operação '%s'\n", pedido.pid, pedido.operacao);

        // Criar resposta (aqui só respondemos algo simples, para testar)
        char resposta[512];
        snprintf(resposta, sizeof(resposta), "Pedido recebido: operação '%s', dados: %s", pedido.operacao, pedido.dados);

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
