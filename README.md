# Sistemas Operativos Project

## Authors
- Hugo Soares
- Francisco Martins
- Jorge Barbosa

## Project Overview
This project implements a client-server document indexing and search system as part of the Operating Systems course. The system allows users to:

- Index text documents with metadata (title, authors, year, path)
- Search documents by content and metadata
- Manage document entries through a client-server architecture
- Handle concurrent requests with caching optimizations

Key features:
- Named pipe (FIFO) communication between client and server
- Metadata persistence to disk
- LRU caching strategy for frequently accessed documents
- Concurrent search operations
- File-based document storage

## Installation
1. Clone the repository
2. Ensure required directories exist:
```bash
mkdir -p src include obj bin tmp
```
3. Run it:
```bash
./executables/dserver <args>
./executables/dclient <args>
```

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements
We would like to thank our professor and teaching assistants for their support and guidance throughout this project.
