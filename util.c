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

// sudo apt-get update
// sudo apt-get install libssl-dev


volatile sig_atomic_t flag = 0;

int sendall(int s, char *buf, int *len)
{
    int total = 0;
    int bytesleft = *len;
    int n;

    while(total < *len)
    {
        if((n = send(s, buf+total, bytesleft, 0)) <= 0)
        {
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

// void connect_and_send(int * client_socket_fd)
// {
//     int client_socket = *client_socket_fd;
//     char buffer[BUFSIZE+1], packet[1024], req_method[10], req_uri[256], keep_conn[20], http_req[1000], path[2048], host[2048];
//     // size_t num_bytes;
//     // filename[261]
//     // struct stat st;
//     int n;
//     int msg_size = 0;
//     // const char * mime_type;
//     char req_version[10] = "HTTP/?";
//     struct timeval timeout;
//     timeout.tv_sec = 10;
//     timeout.tv_usec = 0;
//     // int filefd;
//     // off_t offset = 0;
//     char * crlf;
//     // int port = 80;


//     setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout , sizeof(timeout));
//     // curl_global_init(CURL_GLOBAL_DEFAULT);
    
//     for(;;)
//     {
//         memset(buffer, 0, BUFSIZE+1);
//         memset(keep_conn, 0, 20);
//         memset(packet, 0, 1024);
//         msg_size = 0;
//         // offset = 0;
//         buffer[BUFSIZE] = '\0';


//         for(;;)
//         {
//             if(check((n = read(client_socket, buffer+msg_size, sizeof(buffer)-msg_size-1)), "CONNECTION TIMEOUT"))
//             {
//                 close(client_socket);
//                 return;
//             }
//             msg_size += n;
//             if(msg_size > BUFSIZE - 1 || strstr(buffer, "\r\n\r\n")) break;
//         }
//         printf("BUFFER\n%s\n", buffer);

        

//         if((crlf = strchr(buffer, '\r')) == NULL)
//         {
//             int p_sz = sprintf(packet,"%s 400 Bad Request\r\n\r\n", req_version);
//             check(sendall(client_socket, packet, &p_sz), "Error writing 400 header to client\n");
//             break;
//         }
//         else
//         {
//             int size = crlf - buffer;
//             strncpy(http_req, buffer, size);
//             http_req[size] = '\0';

//             if ((n = sscanf(http_req, "%s %s %s", req_method, req_uri, req_version)) < 3)
//             {
//                 int p_sz = sprintf(packet,"%s 400 Bad Request\r\n\r\n", req_version);
//                 check(sendall(client_socket, packet, &p_sz), "Error writing 400 header to client\n");
//                 break;
//             }

//             char *host_start = strstr(req_uri, "://");
//             if (host_start)
//             {
//                 host_start += 3;
//             }
//             else
//             {
//                 host_start = req_uri;
//                 // handle this error when the URL doesn't exist
//             }

//             char *path_start = strchr(host_start, '/');
//             if(path_start)
//             {  
//                 strncpy(host, host_start, path_start - host_start);
//                 host[path_start - host_start] = '\0';
//                 strcpy(path, path_start);
//             }
//             else
//             {
//                 strcpy(host, host_start);
//                 strcpy(path, "/");
//             }

//             printf("host: %s, path: %s, req_uri: %s\n", host, path, req_uri);


//             // char *port_start = strchr(host, ':');
//             // if(port_start)
//             // {
//             //     *port_start = '\0';
//             //     port = atoi(port_start + 1);
//             // }
//         }
        
//         if(strcmp(req_version, "HTTP/1.1") != 0 && strcmp(req_version, "HTTP/1.0") != 0)
//         {
//             int p_sz = sprintf(packet,"%s 505 HTTP Version Not Supported\r\n\r\n", req_version);
//             check(sendall(client_socket, packet, &p_sz), "Error writing 505 header to client\n");
//             break;
//         }

//         char * keep_alive = strstr(buffer, "Connection:");
//         if(keep_alive != NULL)
//         {
//             n = sscanf(keep_alive+11, "%s", keep_conn);
//             if(strcmp(keep_conn, "keep-alive") == 0 || strcmp(keep_conn, "Keep-alive") == 0 || strcmp(keep_conn, "Keep-Alive") == 0)
//                 sprintf(keep_conn, "%s", "Keep-Alive");
//             if(strcmp(keep_conn, "close") == 0 || strcmp(keep_conn, "Close") == 0)
//                 sprintf(keep_conn, "%s", "Close");
//         }
//         else
//         {
//             strcpy(keep_conn, "Close");
//         }

//         if(strcmp(req_method, "GET") == 0)
//         {
//             // if(is_host_blocked(host))
//             // {
//             //     int p_sz = sprintf(packet,"%s 403 Forbidden\r\n\r\n", req_version);
//             //     check(sendall(client_socket, packet, &p_sz), "Error writing 403 header to client\n");
//             //     break;
//             // }
//             // if(valid_path(req_uri, filename))
//             // {
//             //     int p_sz = sprintf(packet,"%s 403 Forbidden\r\n\r\n", req_version);
//             //     check(sendall(client_socket, packet, &p_sz), "Error writing 403 header to client\n");
//             //     break;
//             // }

//             curl_global_init(CURL_GLOBAL_ALL); // or before curl_easy_init
//             CURL *curl = curl_easy_init();
//             if(!curl)
//             {
//                 printf("FAILED\n");
//             }

//             callback_data data = {
//                     .client_socket = client_socket,
//                     .status = 0,
//                     .filename = get_file_url(path),
//                     // .url = "http://netsys.cs.colorado.edu/images/wine3.jpg",
//                     .url = req_uri,
//                     .path = path,
//                     .curl = curl,
//                     .fp = NULL,
//             };

//             printf("URL BEFORE CURL EASY: %s\n", data.url);

//             // curl_easy_setopt(curl, CURLOPT_URL, "http://example.com:8080/some/path"); handle different ports
//             curl_easy_setopt(curl, CURLOPT_PROXY, "http://localhost:8080");
//             curl_easy_setopt(curl, CURLOPT_URL, data.filename ? data.filename : data.url);
//             curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
//             // curl_easy_setopt(curl, CURLOPT_URL, "http://netsys.cs.colorado.edu/");
//             // curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
//             // curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&data);

//             curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, check_and_write);
//             curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&data);
//             curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
//             // curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); this line is for redirects
//             printf("CURLOPT WRITE STUFF SET\n");
//             CURLcode res = curl_easy_perform(curl);
//             printf("After CURL_PERFORM\n");
//             if(res != CURLE_OK)
//             {
//                 fprintf(stderr, "curl_easy_perform() failed: %s\n",
//                 curl_easy_strerror(res));
//                 printf("NO SUCCESS");
//             }
//             else
//             {
//                 printf("SUCCESS");
//             }
//             curl_easy_cleanup(curl);
//             curl_global_cleanup();

            

//             if(data.fp != NULL)
//                 fclose(data.fp);
//         }
//         else
//         {
//             int p_sz = sprintf(packet,"%s 405 Method Not Allowed\r\n\r\n", req_version);
//             check(sendall(client_socket, packet, &p_sz), "Error writing 405 header to client\n");
//             break;
//         }

//         if(strcmp(keep_conn, "Close") == 0)
//             break;
//     }
//     close(client_socket);
    
//     // curl_global_cleanup();
// }

// Firefox, Chrome, wget, curl, aria, netcat --> prolly not Safari or Explorer
// get the Host header and then 
// server needs req_method, req_uri, filepath, version, 
// we need Host and Connection header
// if the request from client doesn't have the host and port number we have to populate it in the request
// we send to the remote host
// write versions for both HTTP/1.0 and HTTP/1.1

size_t check_and_write(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    // status, curl, filename, fp, url, socket
    printf("\n\nIN THE WRITE CALLBACK\n\n");
    size_t realsize = size * nmemb;
    callback_data *data = (callback_data *)userdata;

    // if (data->status == 0) {
    //     curl_easy_getinfo(data->curl, CURLINFO_RESPONSE_CODE, &(data->status));
    // }

    // add checks for time expiration and also dynamic pages
    //  && data->status == 200
    if(data->filename == NULL)
    {
        if(data->fp == -1)
        {
            char * hash = get_hashed_filename(data->url);
            char filename[256];
            snprintf(filename, sizeof(filename), "./cache/%s", hash);
            printf("FILENAME: %s\n", filename);
            // data->fp = open(filename, O_APPEND);
            data->fp = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if(data->fp == -1)
            {
                fprintf(stderr, "Failed to open the file");
                exit(1);
            }

            free(hash);
        }
        write(data->fp, ptr, realsize);
    }
    
    sendall(data->client_socket, ptr, (int *)&realsize);

    return realsize;
}



size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    int *client_socket_fd = (int *)stream;
    size_t total_size = size * nmemb;
    printf("%s", (char *)ptr);
    sendall(*client_socket_fd, ptr, (int *)&total_size);
    return total_size;
}


// ...

void connect_and_send(int *client_socket_fd) {
    int client_socket = *client_socket_fd;
    char buffer[BUFSIZE+1];
    ssize_t n_read;
    size_t msg_size = 0;
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    char req_method[10], req_uri[256], req_version[10] = "HTTP/?", keep_conn[20];

    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();

    if (curl) {
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout , sizeof(timeout));

        for (;;) {
            memset(buffer, 0, BUFSIZE+1);
            msg_size = 0;
            buffer[BUFSIZE] = '\0';

            for(;;)
            {
                if(check((n_read = read(client_socket, buffer+msg_size, sizeof(buffer)-msg_size-1)), "CONNECTION TIMEOUT"))
                {
                    close(client_socket);
                    curl_easy_cleanup(curl);
                    return;
                }
                msg_size += n_read;
                if(msg_size > BUFSIZE - 1 || strstr(buffer, "\r\n\r\n")) break;
            }

            if (n_read <= 0) {
                close(client_socket);
                curl_easy_cleanup(curl);
                return;
            }

            sscanf(buffer, "%s %s %s", req_method, req_uri, req_version);
            printf("\nRequest received:\n%s %s %s\n", req_method, req_uri, req_version);
            char *connection_header = strstr(buffer, "Connection:"); // or "Proxy-Connection"
            if (connection_header) {
                sscanf(connection_header + 11, "%s", keep_conn);
            } else {
                // Default behavior for HTTP/1.1 is to keep the connection alive.
                strcpy(keep_conn, strcmp(req_version, "HTTP/1.1") == 0 ? "keep-alive" : "close");
            }

            if (strcmp(req_version, "HTTP/1.1") != 0 && strcmp(req_version, "HTTP/1.0") != 0) {
                int n = sprintf(buffer, "%s 505 HTTP Version Not Supported\r\n\r\n", req_version);
                sendall(client_socket, buffer, &n);
                curl_easy_cleanup(curl);
                return;
            }

            callback_data data = {
                    .client_socket = client_socket,
                    .status = 0,
                    .filename = get_file_url(req_uri),
                    .url = req_uri,
                    .curl = curl,
                    .fp = -1,
            };

            if(data.filename)
            {
                printf("\nFILENAME: %s\n", data.filename);
            }

            // we have to worry about files getting written at the same, yes the one that gets there before
            // will produce the filename and but the second request might be reading from a partially written
            // file which would not be good --> he said something about linux file something?
            // --> linux file advisory locks

            // for checking for blocklist we can just use strstr to get the host and check if the
            // host exists the blocklist
            // --> 

            if (strcmp(req_method, "GET") == 0) {
                printf("IN GET");
                if(data.filename)
                {
                    printf("IN CACHE");
                    if(data.fp == -1)
                        data.fp = open(data.filename, O_RDONLY);
                    struct stat st;
                    int num_bytes = get_fsize(data.filename, st);
                    off_t offset = 0;
                    while(num_bytes > 0)
                    {
                        ssize_t sent;
                        if(check((sent = sendfile(client_socket, data.fp, &offset, num_bytes)), "Error sending file")) break;
                        num_bytes -= sent;
                    }
                    close(data.fp);
                    continue;
                    // curl_easy_setopt(curl, CURLOPT_READDATA, (void *)&data);
                    // curl_easy_setopt(curl, CURLOPT_READFUNCTION, check_and_write);
                }
                // curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_TFTP | CURLPROTO_FILE);
                curl_easy_setopt(curl, CURLOPT_URL, data.url);
                curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
                curl_easy_setopt(curl, CURLOPT_PROXY, "");
                curl_easy_setopt(curl, CURLOPT_MAXCONNECTS, 3L);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, check_and_write);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &data);
                res = curl_easy_perform(curl);
                printf("AFTER PERFORM\n");
                if (res != CURLE_OK) {
                    printf("NOT OK");
                    int n = sprintf(buffer, "%s 404 Not Found\r\n\r\n", req_version);
                    sendall(client_socket, buffer, &n);
                    break;
                }

                long response_code;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                double content_length;
                curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
                const char *content_type;
                curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);

                int n = sprintf(buffer, "%s %ld OK\r\nContent-Type: %s\r\nContent-Length: %.0f\r\nConnection: %s\r\n\r\n", req_version, response_code, content_type, content_length, keep_conn);
                sendall(client_socket, buffer, &n);
                //printf("Request sent successfully.\n");

                // if(data.fp != NULL)
                //     fclose(data.fp);

            } else {
                int n = sprintf(buffer, "%s 405 Method Not Allowed\r\n\r\n", req_version);
                sendall(client_socket, buffer, &n);
                break;
            }

            if (strcmp(keep_conn, "close") == 0 || strcmp(keep_conn, "Close") == 0) {
                break;
            }

        }

        curl_easy_cleanup(curl);
        
    }
    close(client_socket);
}


size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    size_t realsize = size * nitems;
    callback_data *data = (callback_data *)userdata;

    sendall(data->client_socket, buffer, (int *)&realsize);
    // send(data->client_socket, buffer, realsize, 0);

    return realsize;
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

    if (!file_exists(filepath)) {
        return NULL;
    }
    else
    {
        return filepath;
    }

    // char *file_url_prefix = "file://";
    // char *file_url = malloc(strlen(file_url_prefix) + strlen(filepath) + 1);
    // if (!file_url) {
    //     fprintf(stderr, "Memory allocation error\n");
    //     free(filepath);
    //     free(hashed_filename);
    //     return NULL;
    // }

    // strcpy(file_url, file_url_prefix);
    // strcat(file_url, filepath);

    // free(filepath);
    // free(hashed_filename);

    // return file_url;
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
            free(pclient);
            pthread_exit(NULL);
        }
        if(pclient != NULL)
        {
            connect_and_send(pclient);
            free(pclient);
        }
    }
    pthread_exit(NULL);
}

const char * get_mime_type(const char* file_path)
{
    const char* file_extension = strrchr(file_path, '.');
    if (file_extension) {
        if (strcmp(file_extension, ".html") == 0 || strcmp(file_extension, ".htm") == 0) {
            return "text/html";
        } else if (strcmp(file_extension, ".txt") == 0) {
            return "text/plain";
        } else if (strcmp(file_extension, ".png") == 0) {
            return "image/png";
        } else if (strcmp(file_extension, ".gif") == 0) {
            return "image/gif";
        } else if (strcmp(file_extension, ".jpg") == 0 || strcmp(file_extension, ".jpeg") == 0) {
            return "image/jpeg";
        } else if (strcmp(file_extension, ".ico") == 0) {
            return "image/x-icon";
        } else if (strcmp(file_extension, ".css") == 0) {
            return "text/css";
        } else if (strcmp(file_extension, ".js") == 0) {
            return "application/javascript";
        } else {
            return "application/octet-stream";
        }
    } else {
        return "application/octet-stream";
    }
}

int valid_path(char * req_uri, char * filename)
{
    if (strcmp(req_uri, "/") == 0 || strcmp(req_uri, "/inside/") == 0)
    {
        sprintf(filename, "%s%s", DOCUMENT_ROOT, "/index.html");
    }
    else if (strstr(req_uri, "/inside/") != NULL)
    {
        sprintf(filename, "%s%s", DOCUMENT_ROOT, req_uri+7);
    }
    else
    {
        sprintf(filename, "%s%s", DOCUMENT_ROOT, req_uri);
    }

    if (access(filename, R_OK) == 0) {
        return 0;
    } else {

        return 1;
    }
    return 1;
}


int is_host_blocked(const char *host) {
    FILE *file;
    char line[256];
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    file = fopen("blocklist", "r");
    if (file == NULL) {
        printf("Error: Cannot open blocklist file.\n");
        return 0;
    }

    while (fgets(line, sizeof(line), file)) {
        // Remove newline character from the line
        line[strcspn(line, "\n")] = '\0';

        // Check if the host matches the current line in the blocklist
        if (strcmp(host, line) == 0) {
            fclose(file);
            return 1;
        }

        // If the host is an IP address, we need to compare it with the IP addresses of the blocklist hostnames
        he = gethostbyname(line);
        if (he != NULL) {
            addr_list = (struct in_addr **)he->h_addr_list;
            for (i = 0; addr_list[i] != NULL; i++) {
                if (strcmp(host, inet_ntoa(*addr_list[i])) == 0) {
                    fclose(file);
                    return 1;
                }
            }
        }
    }

    fclose(file);
    return 0;
}
