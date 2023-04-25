#include "headers.h"

void process_header_buffer(struct HeaderData *header_data, const char *buffer, size_t buffer_size) {
    size_t pos = 0;
    while (pos < buffer_size) {
        const char *separator = strchr(buffer + pos, ':');
        if (separator) {
            const char *line_end = strchr(separator, '\n');
            if (!line_end) {
                line_end = buffer + buffer_size;
            }

            struct Header *header = (struct Header *)malloc(sizeof(struct Header));
            header->key = strndup(buffer + pos, separator - (buffer + pos));
            header->value = strndup(separator + 1, line_end - separator - 2);
            header->next = header_data->headers;
            header_data->headers = header;
            header_data->size++;

            pos = line_end - buffer + 1;
        } else {
            break;
        }
    }
}

char *get_header_value(struct HeaderData *header_data, const char *key) {
    struct Header *header = header_data->headers;
    while (header != NULL) {
        if (strcasecmp(header->key, key) == 0) {
            return header->value;
        }
        header = header->next;
    }
    return NULL;
}

void free_header_data(struct HeaderData *header_data) {
    struct Header *header = header_data->headers;
    while (header != NULL) {
        struct Header *next_header = header->next;
        free(header->key);
        free(header->value);
        free(header);
        header = next_header;
    }
}
