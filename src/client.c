#include "client.h"

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-a|-c|-d|-l|-s|-f] [args...]\n", argv[0]);
        return 1;
    }

    // Parse operation
    const char *op = argv[1];

    if (strcmp(op, "-a") == 0) {
        
        if (argc != 6) {
            fprintf(stderr, "Usage: %s -a \"title\" \"authors\" \"year\" \"path\"\n", argv[0]);
            return 1;
        }
        add_document(argv[2], argv[3], argv[4], argv[5]);
    } else if (strcmp(op, "-c") == 0) {
        // Query document
        if (argc != 3) {
            fprintf(stderr, "Usage: %s -c \"key\"\n", argv[0]);
            return 1;
        }
        query_document(argv[2]);
    } else if (strcmp(op, "-d") == 0) {
        // Delete document
        if (argc != 3) {
            fprintf(stderr, "Usage: %s -d \"key\"\n", argv[0]);
            return 1;
        }
        delete_document(argv[2]);
    } else if (strcmp(op, "-l") == 0) {
        // Count lines with keyword
        if (argc != 4) {
            fprintf(stderr, "Usage: %s -l \"key\" \"keyword\"\n", argv[0]);
            return 1;
        }
        count_lines(argv[2], argv[3]);
    } else if (strcmp(op, "-s") == 0) {
        // Search documents with keyword
        if (argc == 3) {
            search_documents(argv[2], "1"); // Default to 1 process
        } else if (argc == 4) {
            search_documents(argv[2], argv[3]);
        } else {
            fprintf(stderr, "Usage: %s -s \"keyword\" [nr_processes]\n", argv[0]);
            return 1;
        }
    } else if (strcmp(op, "-f") == 0) {
        // Shutdown server
        if (argc != 2) {
            fprintf(stderr, "Usage: %s -f\n", argv[0]);
            return 1;
        }
        shutdown_server();
    } else {
        fprintf(stderr, "Invalid operation: %s\n", op);
        fprintf(stderr, "Usage: %s [-a|-c|-d|-l|-s|-f] [args...]\n", argv[0]);
        return 1;
    }

    return 0;
}

void send_request_and_get_response(Request *req, Response *resp) {
    int request_fd, response_fd;

    // Open request pipe for writing
    request_fd = open(REQUEST_PIPE, O_WRONLY);
    if (request_fd == -1) {
        perror("Error opening request pipe");
        exit(1);
    }

    // Send request
    write(request_fd, req, sizeof(Request));
    close(request_fd);

    // Open response pipe for reading
    response_fd = open(RESPONSE_PIPE, O_RDONLY);
    if (response_fd == -1) {
        perror("Error opening response pipe");
        exit(1);
    }

    // Read response
    read(response_fd, resp, sizeof(Response));
    close(response_fd);
}

void add_document(const char *title, const char *authors, const char *year, const char *path) {
    Request req;
    Response resp;

    // Prepare request
    memset(&req, 0, sizeof(Request));
    req.operation = OP_ADD;
    strncpy(req.title, title, sizeof(req.title) - 1);
    strncpy(req.authors, authors, sizeof(req.authors) - 1);
    strncpy(req.year, year, sizeof(req.year) - 1);
    strncpy(req.path, path, sizeof(req.path) - 1);

    // Send request and get response
    send_request_and_get_response(&req, &resp);

    // Print response
    if (resp.status == STATUS_OK) {
        printf("Document %d indexed\n", resp.doc_key);
    } else {
        fprintf(stderr, "Error: %s\n", resp.message);
    }
}

void query_document(const char *key) {
    Request req;
    Response resp;

    // Prepare request
    memset(&req, 0, sizeof(Request));
    req.operation = OP_QUERY;
    strncpy(req.key, key, sizeof(req.key) - 1);

    // Send request and get response
    send_request_and_get_response(&req, &resp);

    // Print response
    if (resp.status == STATUS_OK) {
        printf("%s\n", resp.message);
    } else {
        fprintf(stderr, "Error: %s\n", resp.message);
    }
}

void delete_document(const char *key) {
    Request req;
    Response resp;

    // Prepare request
    memset(&req, 0, sizeof(Request));
    req.operation = OP_DELETE;
    strncpy(req.key, key, sizeof(req.key) - 1);

    // Send request and get response
    send_request_and_get_response(&req, &resp);

    // Print response
    if (resp.status == STATUS_OK) {
        printf("%s\n", resp.message);
    } else {
        fprintf(stderr, "Error: %s\n", resp.message);
    }
}

void count_lines(const char *key, const char *keyword) {
    Request req;
    Response resp;

    // Prepare request
    memset(&req, 0, sizeof(Request));
    req.operation = OP_LINE_COUNT;
    strncpy(req.key, key, sizeof(req.key) - 1);
    strncpy(req.keyword, keyword, sizeof(req.keyword) - 1);

    // Send request and get response
    send_request_and_get_response(&req, &resp);

    // Print response
    if (resp.status == STATUS_OK) {
        printf("%s\n", resp.message);
    } else {
        fprintf(stderr, "Error: %s\n", resp.message);
    }
}

void search_documents(const char *keyword, const char *nr_processes) {
    Request req;
    Response resp;

    // Prepare request
    memset(&req, 0, sizeof(Request));
    
    // If nr_processes > 1, use parallel search
    int processes = atoi(nr_processes);
    if (processes > 1) {
        req.operation = OP_SEARCH_PARALLEL;
        req.nr_processes = processes;
    } else {
        req.operation = OP_SEARCH;
    }
    
    strncpy(req.keyword, keyword, sizeof(req.keyword) - 1);

    // Send request and get response
    send_request_and_get_response(&req, &resp);

    // Print response
    if (resp.status == STATUS_OK) {
        printf("%s\n", resp.message);
    } else {
        fprintf(stderr, "Error: %s\n", resp.message);
    }
}

void shutdown_server() {
    Request req;
    Response resp;

    // Prepare request
    memset(&req, 0, sizeof(Request));
    req.operation = OP_SHUTDOWN;

    // Send request and get response
    send_request_and_get_response(&req, &resp);

    // Print response
    if (resp.status == STATUS_OK) {
        printf("%s\n", resp.message);
    } else {
        fprintf(stderr, "Error: %s\n", resp.message);
    }
}

