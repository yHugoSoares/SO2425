#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "metadata_management.h"
#include "cache_management.h"

extern char document_folder[256];
extern int cache_size;

void save_metadata() {
    printf("Saving metadata to disk...\n");
    
    // Open metadata file for writing
    int fd = open(METADATA_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error opening metadata file for writing");
        return;
    }
    
    // Write number of documents
    write(fd, &num_documents, sizeof(int));
    
    // Write next key
    write(fd, &next_key, sizeof(int));
    
    // Write document metadata
    for (int i = 0; i < num_documents; i++) {
        Document *doc = &documents[i];
        
        // Write key
        write(fd, &doc->key, sizeof(int));
        
        // Write year
        write(fd, &doc->year, sizeof(int));
        
        // Write cache info
        write(fd, &doc->in_cache, sizeof(int));
        write(fd, &doc->access_count, sizeof(int));
        write(fd, &doc->last_access, sizeof(int));
        
        // Write title length and title
        int len = strlen(doc->title) + 1;
        write(fd, &len, sizeof(int));
        write(fd, doc->title, len);
        
        // Write authors length and authors
        len = strlen(doc->authors) + 1;
        write(fd, &len, sizeof(int));
        write(fd, doc->authors, len);
        
        // Write path length and path
        len = strlen(doc->path) + 1;
        write(fd, &len, sizeof(int));
        write(fd, doc->path, len);
    }
    
    close(fd);
    printf("Metadata saved successfully\n");
}

void load_metadata() {
    printf("Loading metadata from disk...\n");
    
    // Check if metadata file exists
    struct stat st;
    if (stat(METADATA_FILE, &st) == -1) {
        printf("No metadata file found, starting with empty database\n");
        return;
    }
    
    // Open metadata file for reading
    int fd = open(METADATA_FILE, O_RDONLY);
    if (fd == -1) {
        perror("Error opening metadata file for reading");
        return;
    }
    
    // Read number of documents
    read(fd, &num_documents, sizeof(int));
    
    // Read next key
    read(fd, &next_key, sizeof(int));
    
    // Allocate memory for documents
    documents = malloc(num_documents * sizeof(Document));
    if (!documents) {
        perror("Memory allocation error");
        close(fd);
        return;
    }
    
    // Read document metadata
    for (int i = 0; i < num_documents; i++) {
        Document *doc = &documents[i];
        
        // Read key
        read(fd, &doc->key, sizeof(int));
        
        // Read year
        read(fd, &doc->year, sizeof(int));
        
        // Read cache info
        read(fd, &doc->in_cache, sizeof(int));
        read(fd, &doc->access_count, sizeof(int));
        read(fd, &doc->last_access, sizeof(int));
        
        // Read title
        int len;
        read(fd, &len, sizeof(int));
        doc->title = malloc(len);
        read(fd, doc->title, len);
        
        // Read authors
        read(fd, &len, sizeof(int));
        doc->authors = malloc(len);
        read(fd, doc->authors, len);
        
        // Read path
        read(fd, &len, sizeof(int));
        doc->path = malloc(len);
        read(fd, doc->path, len);
    }
    
    close(fd);
    
    // Adjust cache to respect cache_size
    adjust_cache_after_load();
    
    printf("Loaded %d documents from metadata file\n", num_documents);
}

