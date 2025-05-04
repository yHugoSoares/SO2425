#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

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
