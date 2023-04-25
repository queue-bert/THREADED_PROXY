// #ifndef RWLOCK_H
// #define RWLOCK_H

// #include <pthread.h>
// #include "util.h"

// // typedef struct FileRWLock FileRWLock;


// typedef struct {
//     pthread_mutex_t lock;
//     pthread_cond_t  readers_cond;
//     pthread_cond_t  writers_cond;
//     int readers;
//     int writers;
//     int waiting_writers;
// } my_rwlock_t;

// // typedef struct my_rwlock_t {
// //     pthread_mutex_t mutex;
// //     pthread_cond_t read_cond;
// //     pthread_cond_t write_cond;
// //     int read_count;
// //     int write_count;
// //     int write_pending;
// //     FileRWLock *file_rwlock; // Add this line
// // } my_rwlock_t;



// typedef struct FileRWLock {
//     char *filename;
//     my_rwlock_t rwlock;
//     struct FileRWLock *next;
// } FileRWLock;


// pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
// FileRWLock *file_rwlock_list = NULL;



// void my_rwlock_init(my_rwlock_t *rwlock);
// void my_rwlock_destroy(my_rwlock_t *rwlock);
// void my_rwlock_rdlock(my_rwlock_t *rwlock);
// void my_rwlock_wrlock(my_rwlock_t *rwlock);
// void my_rwlock_unlock(my_rwlock_t *rwlock);
// my_rwlock_t *get_file_rwlock(const char *filename);
// void free_file_rwlock_list();

// #endif // RWLOCK_H