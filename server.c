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
#include <time.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#define HTML404 "<html><head> <title>404 Not Found</title> <style> html{ height: 100%; } body{ font-family: 'Segoe UI', sans-serif;; height: 100%; } .wrap { width: 100%; height: 100%; display: flex; justify-content: center; align-items: center; background: #fafafa; } .login-form{ border: 1px solid #ddd; padding: 2rem; background: #ffffff; } .form-button{ background: #d95b52; border: 1px solid #ddd; color: #ffffff; padding: 10px; width: 100%;font-weight:bold; } .form-header{ text-align: center; margin-bottom : 1rem; } </style></head><body> <div class=\"wrap\"> <form class=\"login-form\"> <div class=\"form-header\"> <h3>404 Not Found</h3> </div> <button class=\"form-button\" onclick=\"location.href='/index.html'\" type=\"button\">Login</button> </form> </div></body></html>\r\n"
#define HTML401 "<html><head> <title>401 Unauthorized</title> <style> html{ height: 100%; } body{ font-family: 'Segoe UI', sans-serif;; height: 100%; } .wrap { width: 100%; height: 100%; display: flex; justify-content: center; align-items: center; background: #fafafa; } .login-form{ border: 1px solid #ddd; padding: 2rem; background: #ffffff; } .form-button{ background: #fcce03; border: 1px solid #ddd; color: #ffffff; padding: 10px; width: 100%;font-weight:bold; } .form-header{ text-align: center; margin-bottom : 1rem; } </style></head><body> <div class=\"wrap\"> <form class=\"login-form\"> <div class=\"form-header\"> <h3>401 Unauthorized</h3> </div> <button class=\"form-button\" onclick=\"location.href='/index.html'\" type=\"button\">Login</button> </form> </div></body></html>\r\n"
#define HTML403 "<html><head> <title>403 Forbidden</title> <style> html{ height: 100%; } body{ font-family: 'Segoe UI', sans-serif;; height: 100%; } .wrap { width: 100%; height: 100%; display: flex; justify-content: center; align-items: center; background: #fafafa; } .login-form{ border: 1px solid #ddd; padding: 2rem; background: #ffffff; } .form-button{ background: #7303fc; border: 1px solid #ddd; color: #ffffff; padding: 10px; width: 100%;font-weight:bold; } .form-header{ text-align: center; margin-bottom : 1rem; } </style></head><body> <div class=\"wrap\"> <form class=\"login-form\"> <div class=\"form-header\"> <h3>403 Forbidden</h3> </div> <button class=\"form-button\" onclick=\"location.href='/index.html'\" type=\"button\">Login</button> </form> </div></body></html>\r\n"

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


/* PROCESS THE RECEIVED CONNECTION */
void *handle_connection(void *client_socket) {
    time_t end;
    int logged_in = FALSE;

    int socket = *(int *) client_socket;
    int requested_file;

    /* READ REQUEST HEADER */
    unsigned char request_header[BUFFER_SIZE];
    int file_size = recv_line(socket, request_header);
    live("Client request received:\n%s%s%s\n", COLOR_POSITIVE, request_header,
         COLOR_NEUTRAL);

    /* CHECK IF LOGGED IN */
    if (strstr(request_header, "Cookie: logged_in=true") != NULL) logged_in = TRUE;
    else logged_in = FALSE;

    printf("\nLOGIN STATUS: %d\n", logged_in);

    /* PARSE REQUEST HEADER */
    unsigned char *ptr;
    unsigned char file_path[BUFFER_SIZE];

    ptr = strstr(request_header, " HTTP/");
    if (ptr == NULL) die("Request was not HTTP");
    else {
        *ptr = 0;
        ptr = NULL;

        if (strncmp(request_header, "GET ", 4) == 0) { /* GET */
            ptr = request_header + 4;

            if (logged_in) {
                if (ptr[strlen(ptr) - 1] == '/') strcat(ptr, "secret.html"); //logged in so secret is default
                strcpy(file_path, WEBROOT);
                strcat(file_path, ptr);


                requested_file = open(file_path, O_RDONLY, 0); // open file
                live("Requested file:\n%s\t %s%s\n", COLOR_POSITIVE, file_path, COLOR_NEUTRAL);

                if (strcmp(ptr, "/cookie.html") == 0) //COOKIE FILE
                {
                    live("Response:\n%s\t %s%s\n", COLOR_POSITIVE, "200 OK", COLOR_NEUTRAL);
                    send_string(socket, "HTTP/1.1 200 OK\r\n\r\n");
                    char cookie_info[BUFFER_SIZE];
                    strcpy(cookie_info, "<html><head><title>Cookie ");
                    strcat(cookie_info, USERNAME);
                    strcat(cookie_info, "</title></head><body><p>");
                    strcat(cookie_info, USERNAME);
                    strcat(cookie_info, " ");

                    char buffer[BUFFER_SIZE];

                    time_t current = time(NULL);
                    double remaining = difftime(end, current);

                    sprintf(buffer, "%d", (int) remaining);
                    strcat(cookie_info, buffer);
                    strcat(cookie_info, " seconds left</p></body></html>");
                    send_string(socket, cookie_info);
                } else if (requested_file == -1) { /* 404 NOT FOUND */
                    live("Response:\n%s\t %s%s\n", COLOR_NEGATIVE, "404 Not Found", COLOR_NEUTRAL);
                    send_string(socket, "HTTP/1.1 404 NOT FOUND\r\n\r\n");
                    send_string(socket, HTML404);
                } else { /* 200 OK */
                    live("Response:\n%s\t %s%s\n", COLOR_POSITIVE, "200 OK", COLOR_NEUTRAL);
                    send_string(socket, "HTTP/1.1 200 OK\r\n\r\n");
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
            } else { /* NOT LOGGED IN */
                if (ptr[strlen(ptr) - 1] == '/') strcat(ptr, "index.html"); //not logged in so loginpage is default
                strcpy(file_path, WEBROOT);
                strcat(file_path, ptr);


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
                        send_string(socket, "HTTP/1.1 200 OK\r\n\r\n");
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
        } else if (strncmp(request_header, "POST ", 5) == 0) { /* POST */
            ptr = request_header + 5;

            if (logged_in) {
                if (ptr[strlen(ptr) - 1] == '/') strcat(ptr, "secret.html"); //logged in so secret is default
                strcpy(file_path, WEBROOT);
                strcat(file_path, ptr);

                requested_file = open(file_path, O_RDONLY, 0); // open file
                live("Requested file:\n%s\t %s%s\n", COLOR_POSITIVE, file_path, COLOR_NEUTRAL);

                if (requested_file == -1) { /* 404 NOT FOUND */
                    live("Response:\n%s\t %s%s\n", COLOR_NEGATIVE, "404 Not Found", COLOR_NEUTRAL);
                    send_string(socket, "HTTP/1.1 404 NOT FOUND\r\n\r\n");
                    send_string(socket, HTML404);
                } else { /* 200 OK */
                    live("Response:\n%s\t %s%s\n", COLOR_POSITIVE, "200 OK", COLOR_NEUTRAL);
                    send_string(socket, "HTTP/1.1 200 OK\r\n\r\n");
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
            } else { //NOT LOGGED IN
                unsigned char request_full[BUFFER_SIZE];

                long value_read;
                if ((value_read = read(socket, request_full, BUFFER_SIZE)) >= 0) {

                    live("Entered credentials:\n%s\t %s%s\n", COLOR_POSITIVE, request_full, COLOR_NEUTRAL);
                    live("Expected credentials:\n%s\t %s%s\n", COLOR_POSITIVE, LOGIN, COLOR_NEUTRAL);
                }

                if (strcmp(request_full, LOGIN) == 0) { //LOGIN CORRECT

                    if (ptr[strlen(ptr) - 1] == '/') strcat(ptr, "secret.html");
                    strcpy(file_path, WEBROOT);
                    strcat(file_path, ptr);

                    unsigned char cookie[BUFFER_SIZE];

                    end = time(NULL) + SESSION_TIME;
                    struct tm *local = gmtime(&end);


                    strcpy(cookie, COOKIE);
                    strcat(cookie, time_string(local));

                    if (ptr[strlen(ptr) - 1] == '/') strcat(ptr, "secret.html"); //logged in so secret is default
                    strcpy(file_path, WEBROOT);
                    strcat(file_path, ptr);

                    requested_file = open(file_path, O_RDONLY, 0); // open file
                    live("Requested file:\n%s\t %s%s\n", COLOR_POSITIVE, file_path, COLOR_NEUTRAL);

                    if (requested_file == -1) { /* 404 NOT FOUND */
                        live("Response:\n%s\t %s%s\n", COLOR_NEGATIVE, "404 Not Found", COLOR_NEUTRAL);
                        send_string(socket, "HTTP/1.1 404 NOT FOUND\r\n\r\n");
                        send_string(socket, HTML404);
                    } else { /* 200 OK */
                        live("Response:\n%s\t %s%s\n", COLOR_POSITIVE, "200 OK", COLOR_NEUTRAL);
                        send_string(socket, "HTTP/1.1 200 OK\r\n");
                        printf("\nCOOKIE:%s\n", cookie);
                        send_string(socket, cookie);
                        send_string(socket, "\r\n\r\n");

                        if (ptr == request_header + 4) {
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
                    live("Response:\n%s\t %s%s\n", COLOR_NEGATIVE, "401 Unauthorized", COLOR_NEUTRAL);
                    send_string(socket, "HTTP/1.1 401 Unauthorized\r\n\r\n");
                    send_string(socket, HTML401);
                }
            }

        } else die("Unknown request");


    }
    shutdown(socket, SHUT_RDWR);
}


