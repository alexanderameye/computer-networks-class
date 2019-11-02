#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "common.h"
#include <pthread.h>
#include <sys/types.h>



#include <fcntl.h>


#include <sys/stat.h>
#include <sys/socket.h>

#include <arpa/inet.h>

typedef struct {
    char *ext;
    char *mediatype;
} extn;

extn extensions[] = {
        {"gif",  "image/gif"},
        {"txt",  "text/plain"},
        {"jpg",  "image/jpg"},
        {"jpeg", "image/jpeg"},
        {"png",  "image/png"},
        {"ico",  "image/ico"},
        {"zip",  "image/zip"},
        {"gz",   "image/gz"},
        {"tar",  "image/tar"},
        {"htm",  "text/html"},
        {"html", "text/html"},
        {"php",  "text/html"},
        {"pdf",  "application/pdf"},
        {"zip",  "application/octet-stream"},
        {"rar",  "application/octet-stream"},
        {0,      0}};


//gcc -o server server.c -lpthread


int get_file_size(int);
void *handle_connection(void *);

void die(char *message, ...) {
    va_list ap;
    va_start(ap, message);
    printf("%s[SERVER] ", COLOR_NEGATIVE);
    printf("%s%s\n", COLOR_NEUTRAL, message);
    vfprintf(stdout, message, ap);
    fprintf(stdout, "\n");
    fflush(stdout);
    perror("");
    va_end(ap);
    exit(1);
}

void live(char *message, ...) {
    va_list ap;
    va_start(ap, message);
    printf("%s[SERVER] ", COLOR_ACTION);
    printf("%s", COLOR_NEUTRAL);
    vfprintf(stdout, message, ap);
    fprintf(stdout, "\n");
    fflush(stdout);
    va_end(ap);
}

int main(int argc, char const *argv[]) {
    int server_socket, client_socket;
    int *client_sock;
    struct sockaddr_in server_address, client_address;
    socklen_t sin_size;

    /* CREATE SERVER SOCKET */
    if ((server_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1)
        die("Socket creation failed");
    else live("Socket created");

    server_address.sin_family = SOCKET_DOMAIN;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(SERVER_PORT);
    memset(&(server_address.sin_zero), '\0', 8);

    /* BIND PORT NUMBER TO SERVER SOCKET */
    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1)
        die("Socket bind failed");
    else live("Socket bound");

    /* LISTEN FOR CONNECTION REQUESTS */
    if (listen(server_socket, MAX_CONNECTIONS) == -1) die("Socket listen failed");

    while (1) {
        live("Socket listening...");
        sin_size = sizeof(struct sockaddr_in);
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &sin_size)) == -1)
            die("Socket accept failed");
        else live("Client socket accepted");

        pthread_t sniffer_thread;
        client_sock = malloc(1);
        *client_sock = client_socket;

        if (pthread_create(&sniffer_thread, NULL, handle_connection, (int *) client_sock) == -1)
            die("Thread creation failed");
        else live("Connection thread created");

        pthread_join(sniffer_thread, NULL);
    }
    return 0;
}


/* This function handles the connection on the passed socket from the
 * passed client address.  The connection is processed as a web request
 * and this function replies over the connected socket.  Finally, the
 * passed socket is closed at the end of the function.
 */
void *handle_connection(void * socket_desc) {
    int socket = *(int *) socket_desc;
    int send_bytes;
    long value_read, value_write;
    char *server_response_header = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: text/html\r\n\n";
    char *server_response;

int fd;
    unsigned char request[BUFFER_SIZE], resource[BUFFER_SIZE];
    int length = recv_line(socket, request);
    unsigned char *ptr; //used to traverse the request

    ptr = strstr(request, " HTTP/");//search for valid looking request
    if (ptr == NULL) printf(" NOT HTTP!\n");
    else {
        *ptr = 0; // terminate the buffer at the end of the URL
        ptr = NULL; // set ptr to NULL (used to flag for an invalid request)
        if (strncmp(request, "GET ", 4) == 0)  // get request
            ptr = request + 4; // ptr is the URL
        if (strncmp(request, "HEAD ", 5) == 0) // head request
            ptr = request + 5; // ptr is the URL

        if (ptr == NULL) { // then this is not a recognized request
            printf("\tUNKNOWN REQUEST!\n");
        } else { // valid request, with ptr pointing to the resource name
            if (ptr[strlen(ptr) - 1] == '/')  // for resources ending with '/'
                strcat(ptr, "index.html");     // add 'index.html' to the end
            strcpy(resource, WEBROOT);     // begin resource with web root path
            strcat(resource, ptr);         //  and join it with resource path
            fd = open(resource, O_RDONLY, 0); // try to open the file
            printf("\tOpening \'%s\'\t", resource);
            if (fd == -1) { // if file is not found
                printf(" 404 Not Found\n");
                send_string(socket, "HTTP/1.0 404 NOT FOUND\r\n");
                send_string(socket, "<html><head><title>404 Not Found</title></head>");
                send_string(socket, "<body><h1>URL not found</h1></body></html>\r\n");
            } else {      // otherwise, serve up the file
                printf(" 200 OK\n");
                send_string(socket, "HTTP/1.0 200 OK\r\n");
                if (ptr == request + 4) { // then this is a GET request
                    if ((length = get_file_size(fd)) == -1) die("Getting resource file size");

                    if ((ptr = (unsigned char *) malloc(length)) == NULL) die("Allocating memory for reading resource");

                    read(fd, ptr, length); // read the file into memory
                    send(socket, ptr, length, 0);  // send it to socket
                    free(ptr); // free file memory
                }
                close(fd); // close the file
            } // end if block for file found/not found
        } // end if block for valid request
    } // end if block for valid HTTP
    shutdown(socket, SHUT_RDWR); // close the socket gracefully
}


int get_file_size(int fd) {
    struct stat stat_struct;

    if(fstat(fd, &stat_struct) == -1)
        return -1;
    return (int) stat_struct.st_size;
}


  /*  if ((value_read = read(socket, request, BUFFER_SIZE)) >= 0) {
        live("Client request received:\n%s%s%s\n", COLOR_CLIENT_CONTENT, request,
             COLOR_NEUTRAL);

        const char *start_of_path = strchr(request, '/') + 1;
        const char *end_of_path = strchr(request, 'H') - 1;
        char path[end_of_path - start_of_path];
        strncpy(path, start_of_path, end_of_path - start_of_path);
        path[sizeof(path)] = 0;
        live("Requested file:\n%s%s%s\n", COLOR_CLIENT_CONTENT, path, COLOR_NEUTRAL);

        char *dot = strrchr(path, '.');
        if (dot && !strcmp(dot, ".html")) { //HTML
            server_response_header =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n\n";
        } else if (dot && !strcmp(dot, ".jpeg")) { //JPEG
            server_response_header =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: image/jpeg\r\n\n";

            live("RESPONSE: \n%s%s%s\n", COLOR_CLIENT_CONTENT, server_response_header, COLOR_NEUTRAL);
        }



        FILE *file = fopen(path, "rb");

        if (file != NULL) {
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            rewind(file);

            char *file_content = (char *) malloc(file_size + 1);
            fread(file_content, 1, file_size, file);
            file_content[file_size] = 0;

            fclose(file);

            server_response = malloc(strlen(server_response_header) + strlen(file_content) + 1);
            strcpy(server_response, server_response_header);
            strcat(server_response, file_content);

            free(file_content);


            send_bytes = strlen(server_response);

            if (write(socket, server_response, send_bytes) != send_bytes)
                die("Writing server response failed");
            else
                live("Server response sent:\n%s%s%s", COLOR_SERVER_CONTENT, server_response, COLOR_NEUTRAL);

            free(server_response);
            free(socket_desc);
            close(socket);

        } else {
            printf("%s[SERVER] ", COLOR_NEGATIVE);
            printf("%s%s\n", COLOR_NEUTRAL, "No such file or directory, skipped");
            free(socket_desc);

            close(socket);
        }

    } else {
        die("Reading client request failed");
        free(socket_desc);

        close(socket);
    }*/
