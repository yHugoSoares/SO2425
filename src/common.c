#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "common.h"

// Função auxiliar para contar linhas com palavra usando grep
int conta_linhas_com_palavra(const char *filepath, const char *keyword) {
    char comando[512];
    printf("Executando grep para contar linhas com a palavra '%s' no arquivo '%s'\n", 
           keyword, filepath);
    
    // Verifica se o arquivo existe antes de executar o grep
    struct stat buffer;
    if (stat(filepath, &buffer) != 0) {
        printf("Erro: arquivo não existe: %s (erro: %s)\n", filepath, strerror(errno));
        return -1;
    }
    
    snprintf(comando, sizeof(comando), "grep -c \"%s\" \"%s\" 2>/dev/null", keyword, filepath);
    printf("Comando: %s\n", comando);
    
    FILE *fp = popen(comando, "r");
    if (!fp) {
        perror("Erro ao executar grep");
        return -1;
    }

    int linhas = 0;
    if (fscanf(fp, "%d", &linhas) != 1) {
        printf("Erro ao ler resultado do grep\n");
        linhas = 0;
    }
    
    pclose(fp);
    printf("Encontradas %d linhas com a palavra '%s'\n", linhas, keyword);
    return linhas;
}
