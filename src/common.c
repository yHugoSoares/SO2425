#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "common.h"
#include "cache.h"


void save_metadata(Metadata metadata) {
    int fd = open(METADATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666); // Use O_APPEND instead of O_TRUNC
    if (fd == -1) {
        perror("save_metadata");
        return;
    }

    // Write only the valid entries in the metadata
    ssize_t written = write(fd, metadata.entry, metadata.count * sizeof(Entry));
    if (written != metadata.count * sizeof(Entry)) {
        perror("Error writing metadata");
    }

    close(fd);
}


void load_metadata(Metadata metadata) {
    int fd = open(METADATA_FILE, O_RDONLY);
    if (fd == -1) {
        metadata.count = 0;
        metadata.last_id = 0;
        return;
    }

    // Read the file into the docs array
    ssize_t bytes_read = read(fd, metadata.entry, MAX_ENTRIES * sizeof(Entry));
    if (bytes_read < 0) {
        perror("Error reading metadata");
        metadata.count = 0;
    } else {
        // Calculate the number of valid entries
        metadata.count = bytes_read / sizeof(Entry);
    }

    close(fd);
}

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
