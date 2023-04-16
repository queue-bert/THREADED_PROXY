#ifndef UTIL_H
#define UTIL_H

#define ACK 10
#define SOCKETERROR -1
#define PACKET BUFSIZE + HEADER
#define BUFFER BUFSIZE * 2
#define BACKLOG 64
#define POOL_THREADS 20
#define DOCUMENT_ROOT "./www"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include "queue.h"
#include <curl/curl.h>
#include <stddef.h>

extern volatile sig_atomic_t flag;

// status, curl, filename, fp, url, socket

typedef struct {
    int client_socket;
    int status;
    const char *filename;
    const char *url;
    CURL *curl;
    int fp;
} callback_data;

int sendall(int s, char *buf, int *len);

int get_fsize(const char* file, struct stat st);

int check(int stat, char* message);

void connect_and_send(int * client_socket_fd);

void * thread_function();

const char* get_mime_type(const char* file_path);

int valid_path(char * req_uri, char * filename);

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);

size_t check_and_write(char *ptr, size_t size, size_t nmemb, void *userdata);

size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata);

char *get_hashed_filename(const char *path);

int file_exists(const char *filepath);

char *get_file_url(const char *path);

int is_host_blocked(const char *host);

#endif