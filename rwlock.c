// #include "rwlock.h"
// #include "util.h"
// #include <string.h>
// #include <stdlib.h>

// void my_rwlock_init(my_rwlock_t *rwlock) {
//     pthread_mutex_init(&rwlock->lock, NULL);
//     pthread_cond_init(&rwlock->readers_cond, NULL);
//     pthread_cond_init(&rwlock->writers_cond, NULL);
//     rwlock->readers = 0;
//     rwlock->writers = 0;
//     rwlock->waiting_writers = 0;
// }

// void my_rwlock_destroy(my_rwlock_t *rwlock) {
//     pthread_mutex_destroy(&rwlock->lock);
//     pthread_cond_destroy(&rwlock->readers_cond);
//     pthread_cond_destroy(&rwlock->writers_cond);
// }

// void my_rwlock_rdlock(my_rwlock_t *rwlock) {
//     pthread_mutex_lock(&rwlock->lock);
//     while (rwlock->writers > 0 || rwlock->waiting_writers > 0) {
//         pthread_cond_wait(&rwlock->readers_cond, &rwlock->lock);
//     }
//     rwlock->readers++;
//     pthread_mutex_unlock(&rwlock->lock);
// }

// void my_rwlock_wrlock(my_rwlock_t *rwlock) {
//     pthread_mutex_lock(&rwlock->lock);
//     rwlock->waiting_writers++;
//     while (rwlock->readers > 0 || rwlock->writers > 0) {
//         pthread_cond_wait(&rwlock->writers_cond, &rwlock->lock);
//     }
//     rwlock->waiting_writers--;
//     rwlock->writers++;
//     pthread_mutex_unlock(&rwlock->lock);
// }

// void my_rwlock_unlock(my_rwlock_t *rwlock) {
//     pthread_mutex_lock(&rwlock->lock);
//     if (rwlock->writers > 0) {
//         rwlock->writers--;
//     } else {
//         rwlock->readers--;
//     }

//     if (rwlock->writers == 0 && rwlock->waiting_writers > 0) {
//         pthread_cond_signal(&rwlock->writers_cond);
//     } else if (rwlock->writers == 0 && rwlock->readers == 0) {
//         pthread_cond_broadcast(&rwlock->readers_cond);
//     }
//     pthread_mutex_unlock(&rwlock->lock);
// }

// my_rwlock_t *get_file_rwlock(const char *filename) {
//     pthread_mutex_lock(&list_mutex);

//     FileRWLock *current = file_rwlock_list;
//     while (current) {
//         if (strcmp(current->filename, filename) == 0) {
//             pthread_mutex_unlock(&list_mutex);
//             return &current->rwlock;
//         }
//         current = current->next;
//     }

//     FileRWLock *new_entry = malloc(sizeof(FileRWLock));
//     new_entry->filename = strdup(filename);
//     my_rwlock_init(&new_entry->rwlock);
//     new_entry->next = file_rwlock_list;
//     file_rwlock_list = new_entry;

//     pthread_mutex_unlock(&list_mutex);
//     return &new_entry->rwlock;
// }

// void free_file_rwlock_list() {
//     FileRWLock *current = file_rwlock_list;
//     while (current) {
//         FileRWLock *next = current->next;
//         free(current->filename);
//         my_rwlock_destroy(&current->rwlock);
//         free(current);
//         current = next;
//     }
// }