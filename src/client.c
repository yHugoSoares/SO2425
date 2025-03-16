#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PIPE_NAME "/tmp/dserver_pipe"
#define BUFFER_SIZE 512

void print_usage(const char *prog_name) {
    const char *usage =
        "Usage:\n"
        "  %s -a \"title\" \"authors\" \"year\" \"path\"\n"
        "  %s -c \"key\"\n"
        "  %s -d \"key\"\n"
        "  %s -l \"key\" \"keyword\"\n"
        "  %s -s \"keyword\"\n";
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, usage, prog_name, prog_name, prog_name, prog_name, prog_name);
    write(STDOUT_FILENO, buffer, strlen(buffer));
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return -1;
    }

    int fd;
    char buffer[BUFFER_SIZE];

    // Open the named pipe
    fd = open(PIPE_NAME, O_WRONLY);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    // Handle different commands
    if (strcmp(argv[1], "-a") == 0) {
        if (argc != 6) {
            print_usage(argv[0]);
            return -1;
        }
        snprintf(buffer, BUFFER_SIZE, "ADD|%s|%s|%s|%s", argv[2], argv[3], argv[4], argv[5]);
    } else if (strcmp(argv[1], "-c") == 0) {
        if (argc != 3) {
            print_usage(argv[0]);
            return -1;
        }
        snprintf(buffer, BUFFER_SIZE, "CHECK|%s", argv[2]);
    } else if (strcmp(argv[1], "-d") == 0) {
        if (argc != 3) {
            print_usage(argv[0]);
            return -1;
        }
        snprintf(buffer, BUFFER_SIZE, "DELETE|%s", argv[2]);
    } else if (strcmp(argv[1], "-l") == 0) {
        if (argc != 4) {
            print_usage(argv[0]);
            return -1;
        }
        snprintf(buffer, BUFFER_SIZE, "LINES|%s|%s", argv[2], argv[3]);
    } else if (strcmp(argv[1], "-s") == 0) {
        if (argc != 3) {
            print_usage(argv[0]);
            return -1;
        }
        snprintf(buffer, BUFFER_SIZE, "SEARCH|%s", argv[2]);
    } else {
        print_usage(argv[0]);
        return -1;
    }

    // Write the command to the pipe
    if (write(fd, buffer, strlen(buffer) + 1) == -1) {
        perror("write");
        close(fd);
        return -1;
    }

    close(fd);

    // Open the named pipe for reading the response
    fd = open(PIPE_NAME, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    // Read the response from the server
    ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read == -1) {
        perror("read");
        close(fd);
        return -1;
    }

    buffer[bytes_read] = '\0';
    write(STDOUT_FILENO, buffer, bytes_read);

    close(fd);
    return 1;
}