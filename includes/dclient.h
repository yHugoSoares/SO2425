#ifndef DCLIENT_H
#define DCLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "common.h"

void prepararPedido(int argc, char *argv[], MensagemCliente *pedido);

int main (int argc, char *argv[]);

#endif /* CLIENT_H */
