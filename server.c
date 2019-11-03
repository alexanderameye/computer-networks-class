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
#define HTML401 "<html><head> <title>401 Unauthorized</title> <style> html{ height: 100%; } body{ font-family: 'Segoe UI', sans-serif;; height: 100%; } .wrap { width: 100%; height: 100%; display: flex; justify-content: center; align-items: center; background: #fafafa; } .login-form{ border: 1px solid #ddd; padding: 2rem; background: #ffffff; } .form-button{ background: #fcce03; border: 1px solid #ddd; color: #ffffff; padding: 10px; width: 100%;font-weight:bold; } .form-header{ text-align: center; margin-bottom : 1rem; } </style></head><body> <div class=\"wrap\"> <form class=\"login-form\"> <div class=\"form-header\"> <h3>401 Unauthorized</h3> </div> <button class=\"form-button\" onclick=\"location.href='/index.html'\" type=\"button\">Go Back</button> </form> </div></body></html>\r\n"
#define HTML403 "<html><head> <title>403 Forbidden</title> <style> html{ height: 100%; } body{ font-family: 'Segoe UI', sans-serif;; height: 100%; } .wrap { width: 100%; height: 100%; display: flex; justify-content: center; align-items: center; background: #fafafa; } .login-form{ border: 1px solid #ddd; padding: 2rem; background: #ffffff; } .form-button{ background: #7303fc; border: 1px solid #ddd; color: #ffffff; padding: 10px; width: 100%;font-weight:bold; } .form-header{ text-align: center; margin-bottom : 1rem; } </style></head><body> <div class=\"wrap\"> <form class=\"login-form\"> <div class=\"form-header\"> <h3>403 Forbidden</h3> </div> <button class=\"form-button\" onclick=\"location.href='/index.html'\" type=\"button\">Go Back</button> </form> </div></body></html>\r\n"

void *handle_connection(void *);

int logged_in = 0;

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
    unsigned char request_header[BUFFER_SIZE];
    int file_size = recv_line(socket, request_header);
    live("Client request received:\n%s%s%s\n", COLOR_POSITIVE, request_header,
         COLOR_NEUTRAL);

    /* PARSE REQUEST */
    unsigned char *ptr; // used to traverse the request
    unsigned char file_path[BUFFER_SIZE];

    ptr = strstr(request_header, " HTTP/"); // find first occurrence
    if (ptr == NULL) die("Request was not HTTP");
    else {
        *ptr = 0; // reset pointer
        ptr = NULL;

        /* GET REQUEST */
        if (strncmp(request_header, "GET ", 4) == 0) {
            ptr = request_header + 4;
            if (ptr[strlen(ptr) - 1] == '/') strcat(ptr, "index.html"); // index.html is default
            strcpy(file_path, WEBROOT);
            strcat(file_path, ptr); // append request file to path

            if (logged_in) { /* LOGGED IN */
                requested_file = open(file_path, O_RDONLY, 0); // open file
                live("Requested file:\n%s\t %s%s\n", COLOR_POSITIVE, file_path, COLOR_NEUTRAL);

                /* 404 NOT FOUND */
                if (requested_file == -1) {
                    live("Response:\n%s\t %s%s\n", COLOR_NEGATIVE, "404 Not Found", COLOR_NEUTRAL);
                    send_string(socket, "HTTP/1.1 404 NOT FOUND\r\n\r\n");
                    send_string(socket, HTML404);
                }

                    /* 200 OK */
                else {
                    live("Response:\n%s\t %s%s\n", COLOR_POSITIVE, "200 OK", COLOR_NEUTRAL);
                    send_string(socket, "HTTP/1.0 200 OK\r\n\r\n");
                    if (ptr == request_header + 4) { // GET Request
                        if ((file_size = get_file_size(requested_file)) == -1) die("Failed getting file size");
                        if ((ptr = (unsigned char *) malloc(file_size)) == NULL)
                            die("Failed allocating memory for reading");
                        read(requested_file, ptr, file_size);
                        send(socket, ptr, file_size, 0);
                        free(ptr);
                    }
                    close(requested_file);
                }
            } else {  /* NOT LOGGED IN */
                if (strcmp(ptr, "/index.html") == 0) // ONLY ALLOWED ACCESS
                {
                    requested_file = open(file_path, O_RDONLY, 0); // open file
                    live("Requested file:\n%s\t %s%s\n", COLOR_POSITIVE, file_path, COLOR_NEUTRAL);

                    if (requested_file == -1) {  /* 404 NOT FOUND */
                        live("Response:\n%s\t %s%s\n", COLOR_NEGATIVE, "404 Not Found", COLOR_NEUTRAL);
                        send_string(socket, "HTTP/1.1 404 NOT FOUND\r\n\r\n");
                        send_string(socket, HTML404);
                    } else {  /* 200 OK */
                        live("Response:\n%s\t %s%s\n", COLOR_POSITIVE, "200 OK", COLOR_NEUTRAL);
                        send_string(socket, "HTTP/1.0 200 OK\r\n\r\n");
                        if (ptr == request_header + 4) { // GET Request
                            if ((file_size = get_file_size(requested_file)) == -1) die("Failed getting file size");
                            if ((ptr = (unsigned char *) malloc(file_size)) == NULL)
                                die("Failed allocating memory for reading");
                            read(requested_file, ptr, file_size);
                            send(socket, ptr, file_size, 0);
                            free(ptr);
                        }
                        close(requested_file);
                    }
                } else // FORBIDDEN ALWAYS IF NOT LOGGED IN EVEN IF NON EXISTENT PAGE
                {
                    live("Response:\n%s\t %s%s\n", COLOR_NEGATIVE, "403 Forbidden", COLOR_NEUTRAL);
                    send_string(socket, "HTTP/1.1 403 FORBIDDEN\r\n\r\n");
                    send_string(socket, HTML403);
                }
            }
        }

            /* POST REQUEST */
        else if (strncmp(request_header, "POST ", 5) == 0) {
            ptr = request_header + 5; //checks if POST is present

            if (ptr[strlen(ptr) - 1] == '/') strcat(ptr, "index.html"); // index.html is default
            strcpy(file_path, WEBROOT);
            strcat(file_path, ptr); // append request file to path

            unsigned char request_full[BUFFER_SIZE];

            long value_read;
            if ((value_read = read(socket, request_full, BUFFER_SIZE)) >= 0) {

                live("Entered credentials:\n%s\t %s%s\n", COLOR_POSITIVE, request_full, COLOR_NEUTRAL);
                live("Expected credentials:\n%s\t %s%s\n", COLOR_POSITIVE, LOGIN, COLOR_NEUTRAL);
            }

            if (strcmp(request_full, LOGIN) == 0) {
                logged_in = 1;

                requested_file = open(".//secret.html", O_RDONLY, 0); // open file
                live("Requested file:\n%s\t %s%s\n", COLOR_POSITIVE, ".//secret.html", COLOR_NEUTRAL);

                if (requested_file == -1) {  /* 404 NOT FOUND */
                    live("Response:\n%s\t %s%s\n", COLOR_NEGATIVE, "404 Not Found", COLOR_NEUTRAL);
                    send_string(socket, "HTTP/1.1 404 NOT FOUND\r\n\r\n");
                    send_string(socket, HTML404);
                } else {  /* 200 OK */
                    live("Response:\n%s\t %s%s\n", COLOR_POSITIVE, "200 OK", COLOR_NEUTRAL);
                    send_string(socket, "HTTP/1.0 200 OK\r\n\r\n");

                    if ((file_size = get_file_size(requested_file)) == -1) die("Failed getting file size");
                    if ((ptr = (unsigned char *) malloc(file_size)) == NULL)
                        die("Failed allocating memory for reading");
                    read(requested_file, ptr, file_size); //if you print ptr here you get html body
                    send(socket, ptr, file_size, 0);
                    free(ptr);
                    close(requested_file);
                }

            } else // FORBIDDEN ALWAYS IF NOT LOGGED IN EVEN IF NON EXISTENT PAGE
            {
                logged_in = 0;
                live("Response:\n%s\t %s%s\n", COLOR_NEGATIVE, "401 Unauthorized", COLOR_NEUTRAL);
                send_string(socket, "HTTP/1.1 401 Unauthorized\r\n\r\n");
                send_string(socket, HTML401);
            }


        } else die("Unknown request");


    }
    shutdown(socket, SHUT_RDWR);
}


