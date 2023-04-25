#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Header {
    char *key;
    char *value;
    struct Header *next;
};

struct HeaderData {
    struct Header *headers;
    size_t size;
};

void process_header_buffer(struct HeaderData *header_data, const char *buffer, size_t buffer_size);
char *get_header_value(struct HeaderData *header_data, const char *key);
void free_header_data(struct HeaderData *header_data);

#endif
