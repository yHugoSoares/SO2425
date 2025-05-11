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
#define MAX_PROCESS 10
extern Cache *global_cache;

// Function declarations
void save_metadata();
void load_metadata();
int handle_shutdown(Pedido pedido);
int handle_add(Pedido pedido);
int handle_consulta(Pedido pedido);
int handle_remove(Pedido pedido);
// int handle_lines_number(Pedido pedido, const char *document_folder);
// int handle_search(Pedido pedido, const char *document_folder);
int handle_request(Pedido pedido, const char *document_folder);
int handle_shutdown(Pedido pedido);
int handle_add(Pedido pedido);
int handle_consulta(Pedido pedido);
int handle_remove(Pedido pedido);
int handle_lines_number(Pedido pedido, const char *document_folder);
// int handle_search(Pedido pedido, const char *document_folder);

#endif /* HANDLE_REQUEST_H */