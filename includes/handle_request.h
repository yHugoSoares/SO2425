#ifndef HANDLE_REQUEST_H
#define HANDLE_REQUEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include "common.h"

// Global variables
extern Metadata metadata;
extern char *document_folder;

// Function declarations
void save_metadata();
void load_metadata();
int handle_shutdown(MensagemCliente pedido);
int handle_add(MensagemCliente pedido);
int handle_consulta(MensagemCliente pedido);
int handle_remove(MensagemCliente pedido);
// int handle_lines_number(MensagemCliente pedido, const char *document_folder);
// int handle_search(MensagemCliente pedido, const char *document_folder);
int handle_request(MensagemCliente pedido, const char *document_folder);
int handle_shutdown(MensagemCliente pedido);
int handle_add(MensagemCliente pedido);
int handle_consulta(MensagemCliente pedido);
int handle_remove(MensagemCliente pedido);
int handle_lines_number(MensagemCliente pedido, const char *document_folder);
// int handle_search(MensagemCliente pedido, const char *document_folder);

#endif /* HANDLE_REQUEST_H */