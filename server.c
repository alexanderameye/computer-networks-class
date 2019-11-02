#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "common.h"

/* For example http://localhost:10080/alex.html
 * url turned into request message
 * GET /alex.html HTTP/1.1
 * Host: localhost:10080
 *
 * */

void die(char *message, ...);
void live(char *message, ...);

int main(int argc, char const *argv[]) {
    /* VARIABLES */
    int server_socket, client_socket;
    long value_read, value_write;
    struct sockaddr_in server_address;
    int address_length = sizeof(server_address);
    int send_bytes;
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char *server_response = "No message";

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

            /* GET REQUESTED FILE */
            const char *start_of_path = strchr(request, '/') + 1;
            const char *end_of_path = strchr(request, 'H') - 1;
            char path[end_of_path - start_of_path];
            strncpy(path, start_of_path, end_of_path - start_of_path);
            path[sizeof(path)] = 0;
            live("Requested file:\n%s%s%s\n", COLOR_CLIENT_CONTENT, path, COLOR_NEUTRAL);

            /* CREATE HEADERS BASED ON FILE TYPE */
            char *response_header;
            char *dot = strrchr(path, '.');
            if (dot && !strcmp(dot, ".html")) {
                response_header =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n\n";

                FILE *file;
                char *content;
                long numbytes;

                file = fopen(path, "r");

                fseek(file, 0L, SEEK_END);
                numbytes = ftell(file); //returns current file position
                fseek(file, 0L, SEEK_SET); //reset file position

                content = (char *) calloc(numbytes, sizeof(char));

                fread(content, sizeof(char), numbytes, file);
                fclose(file);

                //  char *content = "<h1>AA</h1>";
                server_response = (char *) malloc(1 + strlen(response_header) + strlen(content));
                strcpy(server_response, response_header);
                strcat(server_response, content);

                free(content);


            } else if (dot && !strcmp(dot, ".jpg")) {
                response_header =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: image/jpg\r\n\n";

                FILE *file;
                char *content;
                long numbytes;

                file = fopen(path, "r");

                fseek(file, 0L, SEEK_END);
                numbytes = ftell(file); //returns current file position
                fseek(file, 0L, SEEK_SET); //reset file position

                content = (char *) calloc(numbytes, sizeof(char));

                fread(content, sizeof(char), numbytes, file);
                fclose(file);


                //  char *content = "<h1>AA</h1>";
                server_response = (char *) malloc(1 + strlen(response_header) + strlen(content));
                strcpy(server_response, response_header);
                strcat(server_response, content);

                free(content);


            } else {
                response_header =
                        "HTTP/1.1 404 Not Found\r\n"
                        "Content-Type: image/jpg\r\n\n<h1>404</h1>";

            }

            memset(request, 0, BUFFER_SIZE);
        } else {
            die("Reading client request failed");
            close(client_socket);
        }


        /* WRITE SERVER RESPONSE */

        send_bytes = strlen(server_response);

        if (write(client_socket, server_response, send_bytes) != send_bytes)
            die("Writing server response failed");
        else
            live("Server response sent:\n%s%s%s", COLOR_SERVER_CONTENT, server_response, COLOR_NEUTRAL);

        close(client_socket);
    }
    return 0;
}



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
