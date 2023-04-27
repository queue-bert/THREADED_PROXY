#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "util.h"
#include "queue.h"
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/sendfile.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <arpa/inet.h>
#include <time.h>


volatile sig_atomic_t flag = 0;
int expire = 0;
pthread_mutex_t list_mutex =  PTHREAD_MUTEX_INITIALIZER;
FileRWLock * file_rwlock_list = NULL;

int sendall(int s, char *buf, int *len)
{
    int total = 0;
    int bytesleft = *len;
    int n;

    while(total < *len)
    {
        printf("writing to %d\n", s);
        if((n = send(s, buf+total, bytesleft, 0)) <= 0)
        {
            printf("error writing to %d\n", s);
            break;
        }
        else
        {
            total += n;
            bytesleft -= n;
        }
    }

    *len = total;

    return n==-1?-1:0;
}

int get_fsize(const char* file, struct stat st)
{
    if(stat(file, &st) == 0)
    {
        return (off_t)st.st_size;
    }
    else
    {
        perror("error determining the filesize");
        return 1;
    }
}

int check(int stat, char* message)
{
    if (stat == SOCKETERROR)
    {
        perror(message);
        return 1;
        //exit(1);
    }
    else
    {
        return 0;
    }
}


size_t check_and_write(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    // status, curl, filename, fp, url, socket
    // printf("\n\nIN THE WRITE CALLBACK\n\n");
    size_t realsize = size * nmemb;
    callback_data *data = (callback_data *)userdata;


    // if(data->filename == NULL)
    // {
    //     if(data->fp == -1)
    //     {
    //         char * hash = get_hashed_filename(data->url);
    //         char filename[256];
    //         snprintf(filename, sizeof(filename), "./cache/%s", hash);
    //         // printf("FILENAME: %s\n", filename);

    //         data->fp = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    //         if(data->fp == -1)
    //         {
    //             fprintf(stderr, "Failed to open the file\n");
    //             exit(1);
    //         }
    //         // UNLOCK WRITING OUTSIDE OF FUNCTION
    //         free(hash);
    //     }
    //     write(data->fp, ptr, realsize);
    // }
    // else
    // {
    //     if(data->fp == -1)
    //     {
    //         // printf("DELETING");
    //         data->fp = open(data->filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    //         if (data->fp == -1) {
    //             fprintf(stderr, "Failed to open the file\n");
    //             exit(1);
    //         }
    //         close(data->fp);

    //         // Reopen the file in append mode
    //         data->fp = open(data->filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    //         if (data->fp == -1)
    //         {
    //             fprintf(stderr, "Failed to open the file\n");
    //             exit(1);
    //         }
    //     }
    //     write(data->fp, ptr, realsize);
    // }
    
    write(data->fp, ptr, realsize);
    sendall(data->client_socket, ptr, (int *)&realsize);

    return realsize;
}



void my_rwlock_init(my_rwlock_t *rwlock) {
    pthread_mutex_init(&rwlock->lock, NULL);
    pthread_cond_init(&rwlock->readers_cond, NULL);
    pthread_cond_init(&rwlock->writers_cond, NULL);
    rwlock->readers = 0;
    rwlock->writers = 0;
    rwlock->waiting_writers = 0;
}

void my_rwlock_destroy(my_rwlock_t *rwlock) {
    pthread_mutex_destroy(&rwlock->lock);
    pthread_cond_destroy(&rwlock->readers_cond);
    pthread_cond_destroy(&rwlock->writers_cond);
}

void my_rwlock_rdlock(my_rwlock_t *rwlock) {
    // printf("READ LOCKING\n");
    pthread_mutex_lock(&rwlock->lock);
    while (rwlock->writers > 0 || rwlock->waiting_writers > 0) {
        pthread_cond_wait(&rwlock->readers_cond, &rwlock->lock);
    }
    rwlock->readers++;
    pthread_mutex_unlock(&rwlock->lock);
}

void my_rwlock_wrlock(my_rwlock_t *rwlock) {
    // printf("WRITE LOCKING\n");
    pthread_mutex_lock(&rwlock->lock);
    rwlock->waiting_writers++;
    while (rwlock->readers > 0 || rwlock->writers > 0) {
        pthread_cond_wait(&rwlock->writers_cond, &rwlock->lock);
    }
    rwlock->waiting_writers--;
    rwlock->writers++;
    pthread_mutex_unlock(&rwlock->lock);
}

void my_rwlock_unlock(my_rwlock_t *rwlock) {
    // printf("UNLOCKING\n");
    pthread_mutex_lock(&rwlock->lock);
    if (rwlock->writers > 0) {
        rwlock->writers--;
    } else {
        rwlock->readers--;
    }

    if (rwlock->writers == 0 && rwlock->waiting_writers > 0) {
        pthread_cond_signal(&rwlock->writers_cond);
    } else if (rwlock->writers == 0 && rwlock->readers == 0) {
        pthread_cond_broadcast(&rwlock->readers_cond);
    }
    pthread_mutex_unlock(&rwlock->lock);
}

my_rwlock_t *get_file_rwlock(const char *filename) {
    pthread_mutex_lock(&list_mutex);
    FileRWLock *current = file_rwlock_list;
    while (current) {
        if (strcmp(current->filename, filename) == 0) {
            pthread_mutex_unlock(&list_mutex);
            // printf("GOT LOCK FOR: %s\n",filename);
            return &current->rwlock;
        }
        current = current->next;
    }

    FileRWLock *new_entry = malloc(sizeof(FileRWLock));
    size_t filename_len = strlen(filename);
    new_entry->filename = malloc(filename_len + 1);
    strcpy(new_entry->filename, filename);
    my_rwlock_init(&new_entry->rwlock);
    // printf("MAKING LOCK FOR: %s\n",filename);
    new_entry->next = file_rwlock_list;
    file_rwlock_list = new_entry;

    pthread_mutex_unlock(&list_mutex);
    return &new_entry->rwlock;
}

void free_file_rwlock_list() {
    FileRWLock *current = file_rwlock_list;
    while (current) {
        FileRWLock *next = current->next;
        free(current->filename);
        my_rwlock_destroy(&current->rwlock);
        free(current);
        current = next;
    }
}


// size_t header_function(char * ptr, size_t size, size_t nmemb, void * userdata)
// {
//     size_t realsize = size * nmemb;

// }

// perhaps i could give everythread it's own curl object and then just have them set the URL everytime to
//reuse it

size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    size_t bytes = size * nitems;
    callback_data *data = (callback_data *)userdata;
    char * end_ptr;
    char new_buf[30];
    // int length;

    // Check for the initial response line

    


    

    if (strncmp(buffer, "HTTP/", 5) == 0) {
        // char response_line[256];
        // strncpy(response_line, buffer, bytes);
        // response_line[bytes] = '\0';
        // sendall(data->client_socket, response_line, (int *)&bytes);
        sendall(data->client_socket, buffer, (int *)&bytes);
        write(data->fp, buffer, bytes);
        // Add Connection header with keep-alive
        // char connection_header[25] = "Connection: close\r\n";
        // int connection_header_len = sizeof(connection_header);
        // sendall(data->client_socket, connection_header, &connection_header_len);
        // return bytes;
    }

    // Process desired headers
    if (strncasecmp(buffer, "Content-Type:", 13) == 0 ||
        strncasecmp(buffer, "Content-Length:", 15) == 0) {
        sendall(data->client_socket, buffer, (int *)&bytes);
        write(data->fp, buffer, bytes);
    }

    if ((end_ptr = strstr(buffer, "Content-Type:")))
    {
        int n = sprintf(new_buf, "Connection: Close\r\n\r\n");
        write(data->fp, new_buf, (size_t) n);
        sendall(data->client_socket, new_buf, &n);
        // continue;
        // snprintf(end_ptr, strlen("\r\nConnection: close"),"%s", "Connection: close\r\n");
        // length = (int)strlen(buffer);
        // printf("String: %s, length: %d\n", buffer, length);
        // write(data->fp, buffer, sizeof(length));
        // sendall(data->client_socket, buffer, &length);
        // return sendall(data->client_socket, buffer, &length) ? 0 : bytes;
        // return bytes;
    }

    // write(data->fp, buffer, bytes);
    // sendall(data->client_socket, buffer, (int *)&bytes);

    return bytes;
    // return sendall(data->client_socket, buffer, (int *)&bytes) ? 0 : bytes;
}



void connect_and_send(int *client_socket_fd) {
    int client_socket = *client_socket_fd;
    char buffer[BUFSIZE+1];
    ssize_t n_read;
    size_t msg_size = 0;
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    char req_method[10], req_uri[256], req_version[10] = "HTTP/?", keep_conn[20], host[256], path[2048];
    // int port = 80;
    char final_uri[3000];

    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();

    if (curl) {
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout , sizeof(timeout));

        for (;;) {
            memset(buffer, 0, BUFSIZE+1);
            memset(req_method, 0, sizeof(req_method));
            memset(req_uri, 0, sizeof(req_uri));
            memset(req_version, 0, sizeof(req_version));
            memset(keep_conn, 0, sizeof(keep_conn));
            memset(host, 0, sizeof(host));
            memset(path, 0, sizeof(path));
            memset(final_uri, 0, sizeof(final_uri));
            msg_size = 0;
            buffer[BUFSIZE] = '\0';

            for(;;)
            {
                if(check((n_read = read(client_socket, buffer+msg_size, sizeof(buffer)-msg_size-1)), "CONNECTION TIMEOUT"))
                {
                    // close(client_socket);
                    printf("FAILED CLOSING CLIENT SOCKET %d\n", client_socket);
                    curl_easy_cleanup(curl);
                    return;
                }
                msg_size += n_read;
                if(msg_size > BUFSIZE - 1 || strstr(buffer, "\r\n\r\n")) break;
            }

            // printf("REQUEST: %s\n", buffer);

            if(sscanf(buffer, "%s %s %s", req_method, req_uri, req_version) < 3)
            {
                int n = sprintf(buffer, "%s 400 Bad Request\r\n\r\n", req_version);
                sendall(client_socket, buffer, &n);
                continue;
            }

            printf("\nRequest received:\n%s %s %s\n", req_method, req_uri, req_version);
            char *connection_header = strstr(buffer, "Connection:"); // or "Proxy-Connection"
            if (connection_header) {
                sscanf(connection_header + 11, "%s", keep_conn);
            } else {
                strcpy(keep_conn, strcmp(req_version, "HTTP/1.1") == 0 ? "keep-alive" : "close");
            }

            if (strcmp(req_version, "HTTP/1.1") != 0 && strcmp(req_version, "HTTP/1.0") != 0)
            {
                int n = sprintf(buffer, "%s 505 HTTP Version Not Supported\r\n\r\n", req_version);
                sendall(client_socket, buffer, &n);
                continue;
            }


            char * host_start = strstr(req_uri, "://");
            if (host_start)
            {
                host_start += 3;
                char *path_start = strchr(host_start, '/');
                if(path_start)
                {  
                    strncpy(host, host_start, path_start - host_start);
                    // printf("HOST: %s\n", host);
                    host[path_start - host_start] = '\0';
                    strcpy(path, path_start);
                }
                else
                {
                    strcpy(host, host_start);
                    strcpy(path, "/");
                }
                sprintf(final_uri, "%s%s", host, path);
                // printf("URI to Pass %s\n:", final_uri);
            }
            else
            {
                host_start = strstr(buffer, "Host:");
                if(host_start)
                {
                    // Within Host: Parser
                    sscanf(host_start + 5, "%s", host);
                    strcpy(path, "/"); // added this
                    sprintf(final_uri, "%s%s", host, req_uri);
                    // printf("URI to Pass %s\n:", final_uri);
                }
                else
                {
                    int n = sprintf(buffer, "%s 400 Bad Request\r\n\r\n", req_version);
                    sendall(client_socket, buffer, &n);
                    continue;
                }
                
            }

            int port = 80; // default port
            char* port_start = strchr(host, ':');
            if (port_start)
            {
                *port_start = '\0';
                port_start++;
                char* endptr;
                port = strtol(port_start, &endptr, 10);
                if (*endptr != '\0' && *endptr != '\r' && *endptr != '\n') {
                    port = 80;
                }
            }

            if(is_host_blocked(host))
            {
                int n = sprintf(buffer, "%s 403 Forbidden\r\n\r\n", req_version);
                sendall(client_socket, buffer, &n);
                continue;
            }

            char * hash_file;
            callback_data data = {
                    .client_socket = client_socket,
                    .status = 0,
                    // .filename = get_file_url(final_uri),
                    // .filename = get_file_url(path),
                    // .filename = (hash_file = get_hashed_filename(path)),
                    .filename = (hash_file = get_file_url(path)),
                    // .url = final_uri,
                    .url = path,
                    .curl = curl,
                    .fp = -1,
                    .lock = NULL,
            };

            if (strcmp(req_method, "GET") == 0)
            {
                struct stat file_stat;
                time_t current_time = time(NULL);
                if(stat(data.filename, &file_stat) || current_time - file_stat.st_mtime > expire)
                {
                    data.fp = open(data.filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    printf("Creating: %s\n", data.filename);
                    // reget or initial get of the file
                    // currently not writing to the cache directory
                    data.lock = get_file_rwlock(data.filename);
                    free(hash_file);
                    my_rwlock_wrlock(data.lock);
                    curl_easy_setopt(curl, CURLOPT_URL, final_uri);
                    curl_easy_setopt(curl, CURLOPT_PROXY, "");
                    curl_easy_setopt(curl, CURLOPT_PORT, port);
                    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
                    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
                    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *) &data);
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, check_and_write);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &data);
                    res = curl_easy_perform(curl);
                    close(data.fp);
                    my_rwlock_unlock(data.lock);

                    if (res != CURLE_OK)
                    {
                        printf("NOT OK\n");
                        int n = sprintf(buffer, "%s 404 Not Found\r\n\r\n", req_version);
                        sendall(client_socket, buffer, &n);
                        // continue;
                    }
                    
                }
                else
                {
                    printf("Reading: %s\n", data.filename);
                    data.lock = get_file_rwlock(data.filename);
                    my_rwlock_rdlock(data.lock);
                    data.fp = open(data.filename, O_RDONLY);
                    int num_bytes = get_fsize(data.filename, file_stat);
                    off_t offset = 0;
                    free(hash_file);
                    while(num_bytes > 0)
                    {
                        ssize_t sent;
                        if(check((sent = sendfile(client_socket, data.fp, &offset, num_bytes)), "Error sending file")) break;
                        num_bytes -= sent;
                    }
                    close(data.fp);
                    my_rwlock_unlock(data.lock);
                    // continue;
                }

                // if(data.filename)
                // {
                    
                //     if (stat(data.filename, &file_stat) == -1) {
                //         perror("Error getting file information");
                //         exit(EXIT_FAILURE);
                //     }
                //     time_t current_time = time(NULL);
                //     double file_age = difftime(current_time, file_stat.st_mtime);
                //     if(file_age < expire)
                //     {
                //         data.lock = get_file_rwlock(data.filename);
                //         my_rwlock_rdlock(data.lock);
                //         data.fp = open(data.filename, O_RDONLY);
                //         // struct stat st; // REDUNDANT HERE I HAVE ONE ABOVE
                //         int num_bytes = get_fsize(data.filename, file_stat);
                //         off_t offset = 0;
                //         while(num_bytes > 0)
                //         {
                //             ssize_t sent;
                //             if(check((sent = sendfile(client_socket, data.fp, &offset, num_bytes)), "Error sending file")) break;
                //             num_bytes -= sent;
                //         }
                //         close(data.fp);
                //         my_rwlock_unlock(data.lock);
                //         continue;
                //     }
                // }
                // char * hash = get_hashed_filename(data.url);
                // char filename[256];
                // snprintf(filename, sizeof(filename), "./cache/%s", hash);
                // // printf("WRITE LOCKING FILENAME: %s\n", filename);
                // data.lock = get_file_rwlock(filename);
                // my_rwlock_wrlock(data.lock);
                // // curl_easy_setopt(curl, CURLOPT_URL, data.url);
                // curl_easy_setopt(curl, CURLOPT_URL, final_uri);
                // curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
                // curl_easy_setopt(curl, CURLOPT_PROXY, "");
                // curl_easy_setopt(curl, CURLOPT_PORT, port);
                // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
                // // curl_easy_setopt(curl, CURLOPT_MAXCONNECTS, 5L);
                // curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
                // curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *) &data);
                // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, check_and_write);
                // curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &data);
                // res = curl_easy_perform(curl);
                // my_rwlock_unlock(data.lock);
                // free(hash);

                // if (res != CURLE_OK) {
                //     printf("NOT OK");
                //     int n = sprintf(buffer, "%s 404 Not Found\r\n\r\n", req_version);
                //     sendall(client_socket, buffer, &n);
                //     continue;
                //     // break;
                // }

                // long response_code;
                // curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                // double content_length;
                // curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
                // const char *content_type;
                // curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);

                // int n = sprintf(buffer, "%s %ld OK\r\nContent-Type: %s\r\nContent-Length: %.0f\r\nConnection: %s\r\n\r\n", req_version, response_code, content_type, content_length, keep_conn);
                // sendall(client_socket, buffer, &n);

            } else {
                int n = sprintf(buffer, "%s 405 Method Not Allowed\r\n\r\n", req_version);
                sendall(client_socket, buffer, &n);
                // continue;
                // break;
            }

            break;

            if (strcmp(keep_conn, "close") == 0 || strcmp(keep_conn, "Close") == 0) {
                break;
            }

        }

        curl_easy_cleanup(curl);
        
    }
    // close(client_socket);
}


char *get_hashed_filename(const char *path) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    char *hashed_path = malloc(33);

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_get_digestbyname("MD5");

    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, path, strlen(path));
    EVP_DigestFinal_ex(mdctx, digest, NULL);

    EVP_MD_CTX_free(mdctx);

    for (int i = 0; i < 16; i++) {
        sprintf(&hashed_path[i * 2], "%02x", (unsigned int)digest[i]);
    }

    return hashed_path;
}

int file_exists(const char *filepath) {
    struct stat buffer;
    return (stat(filepath, &buffer) == 0);
}


char *get_file_url(const char *path) {
    char *hashed_filename = get_hashed_filename(path);
    char *dir = "./cache/";
    char *filepath = malloc(strlen(dir) + strlen(hashed_filename) + 1);
    if (!filepath) {
        fprintf(stderr, "Memory allocation error\n");
        free(hashed_filename);
        return NULL;
    }

    strcpy(filepath, dir);
    strcat(filepath, hashed_filename);

    return filepath;

    // if (!file_exists(filepath)) {
    //     return NULL;
    // }
    // else
    // {
    //     return filepath;
    // }
}



void * thread_function()
{
    int *pclient;
    while(!flag)
    {
        pthread_mutex_lock(&mutex);
        if((pclient = dequeue()) == NULL)
        {
            pthread_cond_wait(&conditional, &mutex);
            if (!flag)
            {
                pclient = dequeue();
                // printf("\n\nNEW REQUEST\n\n");
            }
        }
        pthread_mutex_unlock(&mutex);

        if(flag)
        {
            close(*pclient);
            free(pclient);
            pthread_exit(NULL);
        }
        if(pclient != NULL)
        {
            connect_and_send(pclient);
            close(*pclient);
            free(pclient);
        }
    }
    pthread_exit(NULL);
}



int is_host_blocked(const char *host) {
    FILE *blocklist_file = fopen("blocklist", "r");
    if (blocklist_file == NULL) {
        perror("Error opening blocklist file");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), blocklist_file) != NULL) {
        // Remove newline character if present
        size_t len = strlen(buffer);
        if (buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        if (strncmp(host, buffer, len) == 0) {
            fclose(blocklist_file);
            return 1;
        }
    }

    fclose(blocklist_file);
    return 0;
}