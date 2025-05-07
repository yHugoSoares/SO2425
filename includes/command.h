/**
 * @file command.h
 * @brief Header file for document indexing and management commands.
 *
 * This file contains the function declarations and data structures used for
 * managing document indexing, including adding, removing, and handling metadata.
 */

#ifndef COMMAND_H
#define COMMAND_H

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "index.h"
#include "cache.h"

int add_metadata(char *title, char *authors, char *year, char *path, int cache_flag);
IndexEntry consult_metadata(int key, int cache_flag);
int delete_metadata(int key, int cache_flag);

#endif // COMMAND_H