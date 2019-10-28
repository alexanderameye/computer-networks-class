#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "common.h"

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
    long value_read, value_write;
    struct sockaddr_in server_address;
    int address_length = sizeof(server_address);
    int send_bytes;
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char *server_response = "Hello from server";

    /* CREATE SERVER SOCKET */
    /* creates a server socket */
    if ((server_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1)
        die("Socket creation failed");
    else live("Socket created");

    server_address.sin_family = SOCKET_DOMAIN;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(SERVER_PORT);

    memset(server_address.sin_zero, '\0', sizeof server_address.sin_zero);

    /* BIND */
    /* assigns port number to server socket */
    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1)
        die("Socket bind failed");
    else live("Socket bound");

    /* LISTEN */
    if (listen(server_socket, MAX_CONNECTIONS) == -1) die("Socket listen failed");

    while (1) {
        live("Socket listening...");
        if ((client_socket = accept(server_socket, (struct sockaddr *) &server_address,
                                    (socklen_t * ) & address_length)) == -1)
            die("Socket accept failed");
        else live("Client socket accepted");

        /* READ CLIENT REQUEST */
        if ((value_read = read(client_socket, request, BUFFER_SIZE)) >= 0) {
            live("Client request received:\n%s%s%s\n", COLOR_CLIENT_CONTENT, request,
                 COLOR_NEUTRAL);
            memset(request, 0, BUFFER_SIZE);
        } else {
            die("Reading client request failed");
            close(client_socket);
        }

        /* WRITE SERVER RESPONSE */
        sprintf(response, "GET /%s HTTP/1.1\r\n\r\n", server_response);
        send_bytes = strlen(response);

        if (write(client_socket, response, send_bytes) != send_bytes)
            die("Writing server response failed");
        else
            live("Server response sent:\n%s%s%s", COLOR_SERVER_CONTENT, response, COLOR_NEUTRAL);

        close(client_socket);
    }
    return 0;
}