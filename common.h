#ifndef HTTPSERVER_COMMON_H
#define HTTPSERVER_COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <sys/stat.h>

/* SERVER */
#define WEBROOT "./"
#define SERVER_PORT 10080 // standard HTTP port
#define BUFFER_SIZE 4096
#define MAX_CONNECTIONS 5
#define SOCKET_TYPE SOCK_STREAM // Connection oriented TCP Protocol
#define SOCKET_DOMAIN AF_INET // IP version 4 Address Family (IPv4)
#define SOCKET_PROTOCOL 0 // IP Protocol
#define IP_ADDRESS "127.0.0.1" // localhost

/* AUTHENTICATION */
#define USERNAME "alexanderameye"
#define PASSWORD "paswoord123"

/* COLORS */
#define COLOR_ACTION "\033[1;33m"
#define COLOR_SERVER_CONTENT "\033[1;36m"
#define COLOR_NEUTRAL "\033[0m"
#define COLOR_NEGATIVE "\033[1;31m"
#define COLOR_POSITIVE "\033[0;32m"
#define COLOR_CLIENT_CONTENT "\033[1;35m"


int recv_line(int socket, unsigned char *dest_buffer) {

#define EOL "\r\n\r\n" // detects end of line, can be limited to \r\n
#define EOL_SIZE 4
    unsigned char *ptr; // used to traverse the line
    int eol_matched = 0;
    ptr = dest_buffer;
    while (recv(socket, ptr, 1, 0) == 1) { // read a single byte
        if (*ptr == EOL[eol_matched]) { // check if byte matches and EOL character
            eol_matched++;
            if (eol_matched == EOL_SIZE) { // check if all bytes match EOL
                *(ptr + 1 - EOL_SIZE) = '\0'; // terminate string
                return strlen(dest_buffer); // return received bytes
            }
        } else eol_matched = 0;
        ptr++;
    }
    return 0;
}

void get_password(int socket, unsigned char *destination_buffer) {

    long value_read;
    char request[BUFFER_SIZE];

    if ((value_read = read(socket, request, BUFFER_SIZE)) >= 0) {
        const char *s = request;
        const char *PATTERN1 = "&password=";
        const char *PATTERN2 = " ";

        char *target = NULL;
        char *start, *end;

        if (start = strstr(s, PATTERN1)) {
            start += strlen(PATTERN1);
            if (end = strstr(start, PATTERN2)) {
                target = (char *) malloc(end - start + 1);
                memcpy(target, start, end - start);
                target[end - start] = '\0';
            }
        }

        if (target) printf("PASSWORD: %s\n", target);

        free(target);
    }
}

void get_username(int socket, unsigned char *destination_buffer) {
    long value_read;
    char request[BUFFER_SIZE];

    if ((value_read = read(socket, request, BUFFER_SIZE)) >= 0) {
        const char *s = request;
        const char *PATTERN1 = "username=";
        const char *PATTERN2 = "&password=";

        char *target = NULL;
        char *start, *end;

        if (start = strstr(s, PATTERN1)) {
            start += strlen(PATTERN1);
            if (end = strstr(start, PATTERN2)) {
                target = (char *) malloc(end - start + 1);
                memcpy(target, start, end - start);
                target[end - start] = '\0';
            }
        }

        if (target) printf("USERNAME: %s\n", target);

        free(target);
    }

}


int send_string(int socket, unsigned char *buffer) {
    int sent;
    int bytes = strlen(buffer);
    while (bytes > 0) {
        sent = send(socket, buffer, bytes, 0); // send bytes to socket
        if (sent == -1) return 0;
        bytes -= sent;
        buffer += sent;
    }
    return 1;
}

int get_file_size(int file) {
    struct stat stat_struct;
    if (fstat(file, &stat_struct) == -1) return -1;
    return (int) stat_struct.st_size;
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

#endif
