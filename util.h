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
#include <string.h>
// #include <pthread_rwlock.h>

// typedef struct my_rwlock_t my_rwlock_t;

// #include "rwlock.h"
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t  readers_cond;
    pthread_cond_t  writers_cond;
    int readers;
    int writers;
    int waiting_writers;
} my_rwlock_t;

typedef struct {
    int client_socket;
    int status;
    char *filename; // add const if breaking changes occur
    const char *url;
    CURL *curl;
    int fp;
    my_rwlock_t * lock;
} callback_data;


typedef struct FileRWLock {
    char *filename;
    my_rwlock_t rwlock;
    struct FileRWLock *next;
} FileRWLock;

// GLOBAL VARS

extern volatile sig_atomic_t flag;

extern int expire;

extern pthread_mutex_t list_mutex;

extern FileRWLock * file_rwlock_list;


// FUNCTIONS

int sendall(int s, char *buf, int *len);

int get_fsize(const char* file, struct stat st);

int check(int stat, char* message);

void connect_and_send(int * client_socket_fd);

void * thread_function();

size_t check_and_write(char *ptr, size_t size, size_t nmemb, void *userdata);

size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata);

char *get_hashed_filename(const char *path);

int file_exists(const char *filepath);

char *get_file_url(const char *path);

int is_host_blocked(const char *host);

// RW_LOCK

void free_file_rwlock_list();

my_rwlock_t *get_file_rwlock(const char *filename);

void my_rwlock_unlock(my_rwlock_t *rwlock);

void my_rwlock_wrlock(my_rwlock_t *rwlock);

void my_rwlock_rdlock(my_rwlock_t *rwlock);

void my_rwlock_destroy(my_rwlock_t *rwlock);

void my_rwlock_init(my_rwlock_t *rwlock);

#endif