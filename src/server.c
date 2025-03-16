#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PIPE_NAME "/tmp/dserver_pipe"
#define BUFFER_SIZE 512

void handle_add(const char *title, const char *authors, const char *year, const char *path) {
    // Implement the logic to handle the add command
    printf("Adding: %s, %s, %s, %s\n", title, authors, year, path);
}

void handle_check(const char *key) {
    // Implement the logic to handle the check command
    printf("Checking: %s\n", key);
}

void handle_delete(const char *key) {
    // Implement the logic to handle the delete command
    printf("Deleting: %s\n", key);
}

void handle_lines(const char *key, const char *keyword) {
    // Implement the logic to handle the lines command
    printf("Lines: %s, %s\n", key, keyword);
}

void handle_search(const char *keyword) {
    // Implement the logic to handle the search command
    printf("Searching: %s\n", keyword);
}

int main() {
    int fd;
    char buffer[BUFFER_SIZE];

    // Create the named pipe
    if (mkfifo(PIPE_NAME, 0666) == -1) {
        perror("mkfifo");
        return EXIT;
    }

    while (1) {
        // Open the named pipe for reading
        fd = open(PIPE_NAME, O_RDONLY);
        if (fd == -1) {
            perror("open");
            return EXIT;
        }

        // Read the command from the pipe
        ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read == -1) {
            perror("read");
            close(fd);
            return EXIT;
        }

        buffer[bytes_read] = '\0';

        // Parse the command and handle it
        char *command = strtok(buffer, "|");
        if (strcmp(command, "ADD") == 0) {
            char *title = strtok(NULL, "|");
            char *authors = strtok(NULL, "|");
            char *year = strtok(NULL, "|");
            char *path = strtok(NULL, "|");
            handle_add(title, authors, year, path);
        } else if (strcmp(command, "CHECK") == 0) {
            char *key = strtok(NULL, "|");
            handle_check(key);
        } else if (strcmp(command, "DELETE") == 0) {
            char *key = strtok(NULL, "|");
            handle_delete(key);
        } else if (strcmp(command, "LINES") == 0) {
            char *key = strtok(NULL, "|");
            char *keyword = strtok(NULL, "|");
            handle_lines(key, keyword);
        } else if (strcmp(command, "SEARCH") == 0) {
            char *keyword = strtok(NULL, "|");
            handle_search(keyword);
        }

        close(fd);
    }

    return EXIT_SUCCESS;
}