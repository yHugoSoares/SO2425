#ifndef DSERVER_H
#define DSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include "common.h"

void sigint_handler(int sig);

int main(int argc, char *argv[]);

#endif /* SERVER_H */
