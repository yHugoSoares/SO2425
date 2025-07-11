#ifndef INDEX_H
#define INDEX_H

#define INDEX_FILE "metadata.dat"

typedef struct index_entry {
    int delete_flag;
    char Title[200];
    char authors[200];
    char year[5];
    char path[64];
} Entry;

Entry *create_index_entry(char *title, char *authors, char *year, char *path, int delete_flag);
int destroy_index_entry(Entry *entry);
int index_load_file_to_cache();
int index_add_entry(Entry *entry);
int index_write_dirty_entry(Entry *entry, int key);
int index_delete_entry(int key);
Entry *index_get_entry(int key); // Change return type to Entry *
int index_get_next_key();

#endif // INDEX_H