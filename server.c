#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "common.h"
#include <pthread.h>

//gcc -o server server.c -lpthread

//TODO: send not on same wifi working
//TODO: jpg files

void *connection_handler(void *);

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
    /* VARIABLES */
    int server_socket, client_socket;
    int *client_sock;

    struct sockaddr_in server_address, client_address;
    int address_length = sizeof(server_address);


    char response[BUFFER_SIZE];


    /* CREATE SERVER SOCKET */
    if ((server_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1)
        die("Socket creation failed");
    else live("Socket created");

    server_address.sin_family = SOCKET_DOMAIN;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(SERVER_PORT);

    /* BIND */
    /* assigns port number to server socket */
    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1)
        die("Socket bind failed");
    else live("Socket bound");

    /* LISTEN FOR CONNECTION REQUESTS */
    if (listen(server_socket, MAX_CONNECTIONS) == -1) die("Socket listen failed");

    while ((client_socket = accept(server_socket, (struct sockaddr *) &server_address,
                                   (socklen_t * ) & address_length))) {

        pthread_t sniffer_thread;
        client_sock = malloc(1);
        *client_sock = client_socket;

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void *) client_sock) == -1)
            die("Thread creation failed");
        else live("Connection thread created");

        pthread_join(sniffer_thread, NULL);
    }
    return 0;
}

void *connection_handler(void *socket_desc) {

    int socket = *(int *) socket_desc;
    int send_bytes;
    long value_read, value_write;
    char request[BUFFER_SIZE];
    char *server_response_header = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: text/html\r\n\n";
    char *server_response;

    /* READ CLIENT REQUEST */
    if ((value_read = read(socket, request, BUFFER_SIZE)) >= 0) {
        live("Client request received:\n%s%s%s\n", COLOR_CLIENT_CONTENT, request,
             COLOR_NEUTRAL);

        /* GET REQUESTED FILE */
        const char *start_of_path = strchr(request, '/') + 1;
        const char *end_of_path = strchr(request, 'H') - 1;
        char path[end_of_path - start_of_path];
        strncpy(path, start_of_path, end_of_path - start_of_path);
        path[sizeof(path)] = 0;
        live("Requested file:\n%s%s%s\n", COLOR_CLIENT_CONTENT, path, COLOR_NEUTRAL);

        /* CREATE HEADERS BASED ON FILE TYPE */
        char *dot = strrchr(path, '.');
        if (dot && !strcmp(dot, ".html")) { //HTML
            server_response_header =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n\n";
        } else if (dot && !strcmp(dot, ".jpg")) { //JPG
            server_response_header =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: image/jpg\r\n\n";

            live("RESPONSE: \n%s%s%s\n", COLOR_CLIENT_CONTENT, server_response_header, COLOR_NEUTRAL);
        }

        else if (dot && !strcmp(dot, ".jpeg")) { //JPG
            server_response_header =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: image/jpeg\r\n\n";

            live("RESPONSE: \n%s%s%s\n", COLOR_CLIENT_CONTENT, server_response_header, COLOR_NEUTRAL);
        }

        
        /* READ FILE CONTENTS */
        FILE *file = fopen(path, "rb");

        if (file != NULL) {
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            rewind(file);

            char *file_content = (char *) malloc(file_size + 1);
            fread(file_content, 1, file_size, file);
            file_content[file_size] = 0;

            fclose(file);

            /* ADD HEADER + BODY TO RESPONSE */
            server_response = malloc(strlen(server_response_header) + strlen(file_content) + 1);
            strcpy(server_response, server_response_header);
            strcat(server_response, file_content);

            free(file_content);

            /* WRITE SERVER RESPONSE */
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
    }



}
