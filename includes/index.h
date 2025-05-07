#ifndef INDEX_H
#define INDEX_H

#define INDEX_FILE "index"

typedef struct index_entry {
    int delete_flag;
    char Title[200];
    char authors[200];
    char year[5];
    char path[64];
} IndexEntry;

IndexEntry *create_index_entry(char *title, char *authors, char *year, char *path, int delete_flag);
int destroy_index_entry(IndexEntry *entry);
int index_add_entry(IndexEntry *entry);
int index_write_dirty_entry(IndexEntry *entry, int key);
int index_delete_entry(int key);
IndexEntry *index_get_entry(int key); // Change return type to IndexEntry *
int index_get_next_key();

#endif // INDEX_H