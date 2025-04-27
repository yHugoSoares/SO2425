//src/handle_request.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "common.h"
#include "cache.h"

Metadata metadata;
char *document_folder;


// NOT FULLY CORRECT
void save_metadata() {
    // Simple save without atomic guarantees
    int fd = open("metadata.dat", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("save_metadata");
        return;
    }
    write(fd, &metadata, sizeof(Metadata));
    close(fd);
}

void load_metadata() {
    int fd = open("metadata.dat", O_RDONLY);
    if (fd == -1) {
        metadata.count = 0;
        metadata.last_id = 0;
        return;
    }
    read(fd, &metadata, sizeof(Metadata));
    close(fd);
}

int handle_operation(int fd_server) {
    MensagemCliente pedido;
    
    while (read(fd_server, &pedido, sizeof(MensagemCliente)) > 0) {
        printf("Processing: %c\n", pedido.operacao);
        char response[MAX_DADOS] = {0};
        int client_fd;

        switch(pedido.operacao) {
            case 'a': { // ADD INDEXATION
                Document doc;
                char title[200], authors[200], year[5], path[64];
                
                if(sscanf(pedido.dados, "%[^;];%[^;];%[^;];%s", title, authors, year, path) != 4) {
                    strcpy(response, "Invalid format");
                    break;
                }

                // Basic ID generation
                doc.id = ++metadata.last_id;
                strncpy(doc.title, title, sizeof(doc.title));
                strncpy(doc.authors, authors, sizeof(doc.authors));
                strncpy(doc.year, year, sizeof(doc.year));
                strncpy(doc.path, path, sizeof(doc.path));

                metadata.docs[metadata.count++] = doc;
                save_metadata();
                snprintf(response, sizeof(response), "Added ID %d", doc.id);
                break;
            }
            
            case 'c': { // CHECK WITH KEY (STILL NOT WORKING)
                int doc_id;
                if(sscanf(pedido.dados, "%d", &doc_id) != 1) {
                    strcpy(response, "Invalid ID");
                    break;
                }

                for(int i = 0; i < metadata.count; i++) {
                    if(metadata.docs[i].id == doc_id) {
                        snprintf(response, sizeof(response),
                                "Title: %s\nPath: %s",
                                metadata.docs[i].title,
                                metadata.docs[i].path);
                        break;
                    }
                }
                if(!response[0]) strcpy(response, "Not found");
                break;
            }
            
            case 'd': { // DELETE WITH KEY
                int doc_id;
                if(sscanf(pedido.dados, "%d", &doc_id) != 1) {
                    strcpy(response, "Invalid ID");
                    break;
                }

                for(int i = 0; i < metadata.count; i++) {
                    if(metadata.docs[i].id == doc_id) {
                        metadata.docs[i] = metadata.docs[--metadata.count];
                        // save_metadata();
                        snprintf(response, sizeof(response), "Removed ID %d", doc_id);
                        break;
                    }
                }
                if(!response[0]) strcpy(response, "Not found");
                break;
            }

            case 'l': {
                // TODO
            }

            case 's': {
                // TODO
            }
            
            case 'f':
                return 0; // Terminate server
        }

        if((client_fd = open(pedido.resposta_fifo, O_WRONLY)) != -1) {
            write(client_fd, response, strlen(response)+1);
            close(client_fd);
        }
    }
    return 1;
}