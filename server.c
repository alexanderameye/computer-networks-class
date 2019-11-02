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

#include <sys/socket.h>
#include <arpa/inet.h>

#define HTML404 "<html><head> <title>404 Not Found</title> <style> html{ height: 100%; } body{ font-family: 'Segoe UI', sans-serif;; height: 100%; } .wrap { width: 100%; height: 100%; display: flex; justify-content: center; align-items: center; background: #fafafa; } .login-form{ border: 1px solid #ddd; padding: 2rem; background: #ffffff; } .form-button{ background: #d95b52; border: 1px solid #ddd; color: #ffffff; padding: 10px; width: 100%;font-weight:bold; } .form-header{ text-align: center; margin-bottom : 1rem; } </style></head><body> <div class=\"wrap\"> <form class=\"login-form\"> <div class=\"form-header\"> <h3>404 Not Found</h3> </div> <button class=\"form-button\" onclick=\"location.href='/index.html'\" type=\"button\">Go Back</button> </form> </div></body></html>\r\n"

void *handle_connection(void *);

int main(int argc, char const *argv[]) {
    int server_socket, client_socket, *client;
    struct sockaddr_in server_address, client_address;

    /* CREATE SERVER SOCKET */
    if ((server_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1) die("Socket creation failed");
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
        live("%sSocket listening...%s\n", COLOR_SERVER_CONTENT, COLOR_NEUTRAL);

        socklen_t sin_size = sizeof(struct sockaddr_in);

        /* ACCEPT CONNECTION REQUESTS */
        if ((client_socket = accept(server_socket, (struct sockaddr *) &client_address, &sin_size)) == -1)
            die("Socket accept failed");
        else live("Client socket accepted");

        pthread_t main_thread;
        client = malloc(1);
        *client = client_socket;

        if (pthread_create(&main_thread, NULL, handle_connection, (int *) client) == -1) die("Thread creation failed");
        else live("Client connection thread created");

        pthread_join(main_thread, NULL);
    }
    return 0;
}

void *handle_connection(void *client_socket) {
    int socket = *(int *) client_socket;
    int requested_file;

    /* READ REQUEST */
    unsigned char request[BUFFER_SIZE];
    int file_size = recv_line(socket, request);
    live("Client request received:\n%s\t %s%s\n", COLOR_POSITIVE, request,
         COLOR_NEUTRAL);

    /* PARSE REQUEST */
    unsigned char *ptr; // used to traverse the request
    unsigned char file_path[BUFFER_SIZE];

    ptr = strstr(request, " HTTP/"); // find first occurrence
    if (ptr == NULL) die("Request was not HTTP");
    else {
        *ptr = 0; // reset pointer
        ptr = NULL;
        if (strncmp(request, "GET ", 4) == 0) ptr = request + 4; // checks if GET is present
        if (strncmp(request, "HEAD ", 5) == 0) ptr = request + 5; // checks if HEAD is present
        if (ptr == NULL) die("Unknown request"); // ptr was not moved by GET or HEAD
        else { //ptr was moved, now points to requested file
            if (ptr[strlen(ptr) - 1] == '/') strcat(ptr, "index.html");     // add 'index.html' to the end
            strcpy(file_path, WEBROOT);
            strcat(file_path, ptr);
            requested_file = open(file_path, O_RDONLY, 0);
            live("Opening requested file:\n%s\t %s%s\n", COLOR_POSITIVE, file_path, COLOR_NEUTRAL);
            if (requested_file == -1) { // handle 404 not found
                live("Response:\n%s\t %s%s\n", COLOR_NEGATIVE, "404 Not Found", COLOR_NEUTRAL);
                send_string(socket, "HTTP/1.1 404 NOT FOUND\r\n\r\n");
                send_string(socket, HTML404);
            } else {
                live("Response:\n%s\t %s%s\n", COLOR_POSITIVE, "200 OK", COLOR_NEUTRAL);
                send_string(socket, "HTTP/1.0 200 OK\r\n\r\n");
                if (ptr == request + 4) { // GET Request
                    if ((file_size = get_file_size(requested_file)) == -1) die("Failed getting file size");
                    if ((ptr = (unsigned char *) malloc(file_size)) == NULL)
                        die("Failed allocating memory for reading");
                    read(requested_file, ptr, file_size);
                    send(socket, ptr, file_size, 0);
                    free(ptr);
                }
                close(requested_file);
            }
        }
    }
    shutdown(socket, SHUT_RDWR);
}


