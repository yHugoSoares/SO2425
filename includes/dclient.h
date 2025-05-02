#ifndef DCLIENT_H
#define DCLIENT_H

#define FIFO_SERVER "/tmp/server_fifo"
#define FIFO_CLIENT "/tmp/client_fifo"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "common.h"

int main (int argc, char *argv[]);

#endif 
