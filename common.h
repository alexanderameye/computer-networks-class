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
#include <time.h>

/* SERVER */
#define WEB_ROOT "./"
#define SERVER_PORT 10080
#define BUFFER_SIZE 4096
#define MAX_CONNECTIONS 100
#define SOCKET_TYPE SOCK_STREAM // Connection oriented TCP Protocol
#define SOCKET_DOMAIN AF_INET // IP version 4 Address Family (IPv4)
#define SOCKET_PROTOCOL 0 // IP Protocol

#define TRUE 1
#define FALSE 0

/* AUTHENTICATION */
#define LOGIN "username=alexanderameye&password=paswoord123"
#define USERNAME "alexanderameye"
#define COOKIE "Set-Cookie: logged_in=true; Expires="
#define SESSION_TIME 30

/* COLORS */
#define COLOR_ACTION "\033[1;33m"
#define COLOR_SERVER_CONTENT "\033[1;36m"
#define COLOR_NEUTRAL "\033[0m"
#define COLOR_NEGATIVE "\033[1;31m"
#define COLOR_POSITIVE "\033[0;32m"

/* Read the first line of the sent request and returns it */
int recv_line(int socket, unsigned char *request_header) {
#define EOL "\r\n\r\n"
#define EOL_SIZE 4
    unsigned char *ptr;
    int eol_matched = 0;
    ptr = request_header;
    while (recv(socket, ptr, 1, 0) == 1) {
        if (*ptr == EOL[eol_matched]) {
            eol_matched++;
            if (eol_matched == EOL_SIZE) {
                *(ptr + 1 - EOL_SIZE) = '\0'; // terminate string
                return strlen(request_header);
            }
        } else eol_matched = 0;
        ptr++;
    }
    return 0;
}

/* Converts a tm struct to a string we can use for our cookie expiration time */
char *time_string(const struct tm *timeptr) {
    static char wday_name[7][3] = {
            "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static char mon_name[12][3] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    static char result[31];

    sprintf(result, "%.3s, %d %.3s %d %.2d:%.2d:%.2d GMT",
            wday_name[timeptr->tm_wday],
            timeptr->tm_mday,
            mon_name[timeptr->tm_mon],
            1900 + timeptr->tm_year,
            timeptr->tm_hour,
            timeptr->tm_min,
            timeptr->tm_sec);
    return result;
}

/* Sends a string to the socket byte by byte */
int send_string(int socket, unsigned char *buffer) {
    int sent;
    int bytes = strlen(buffer);
    while (bytes > 0) {
        sent = send(socket, buffer, bytes, 0);
        if (sent == -1) return 0;
        bytes -= sent;
        buffer += sent;
    }
    return 1;
}

/* Get the file size */
int get_file_size(int file) {
    struct stat stat_struct;
    if (fstat(file, &stat_struct) == -1) return -1;
    return (int) stat_struct.st_size;
}

/* Logs an error message to the terminal */
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

/* Logs a success or neutral message to the terminal */
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
