#ifndef METADATA_MANAGEMENT_H
#define METADATA_MANAGEMENT_H

#include "common.h"

// Metadata management function prototypes
void save_metadata();
void load_metadata();

// External variables that need to be accessed
extern Document *documents;
extern int num_documents;
extern int next_key;

#endif /* METADATA_MANAGEMENT_H */
