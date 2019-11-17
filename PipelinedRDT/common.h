#ifndef PIPELINEDRDT_COMMON_H
#define PIPELINEDRDT_COMMON_H

#include <stdarg.h>

#define PORT 10080 // Hard-coded port number
#define SOCKET_DOMAIN AF_INET // IP version 4 Address Family (Ipv4)
#define SOCKET_TYPE SOCK_DGRAM // UDP Protocol
#define SOCKET_PROTOCOL 0 // IP Protocol


#define PACKETSIZE 1400

#define MAXBUFFERLENGTH 100



#define RECEIVER_IP "127.0.0.1"



#define MAX_CONNECTIONS 100
#define TRUE 1
#define FALSE 0

typedef enum {
    DATA,
    ACK
} packet_t;

struct packet {
    packet_t type;
    int sequence_number;
    int length;
    int total_length;
    char data[PACKETSIZE];
};

/* COLORS */
#define COLOR_ACTION "\033[1;33m"
#define COLOR_SERVER_CONTENT "\033[1;36m"
#define COLOR_NEUTRAL "\033[0m"
#define COLOR_NEGATIVE "\033[1;31m"
#define COLOR_POSITIVE "\033[0;32m"


/* Logs an error message to the terminal */
void die(char *message, ...) {
    va_list ap;
    va_start(ap, message);
    printf("%s[SENDER] ", COLOR_NEGATIVE);
    printf("%s%s\n", COLOR_NEUTRAL, message);
    vfprintf(stdout, message, ap);
    fprintf(stdout, "\n");
    fflush(stdout);
    perror("");
    va_end(ap);
    exit(1);
}

typedef enum {
    SENDER,
    RECEIVER
} sender_t;


/* Logs a success or neutral message to the terminal */
void live(char *message, ...) {
    va_list ap;
    va_start(ap, message);
    printf("%s[SENDER] ", COLOR_ACTION);
    printf("%s", COLOR_NEUTRAL);
    vfprintf(stdout, message, ap);
    fprintf(stdout, "\n");
    fflush(stdout);
    va_end(ap);
}

void usage_error() {
    fprintf(stdout, "CORRECT USAGE\n");
    fprintf(stdout, "---------------------------------------------------\n");
    fprintf(stdout, "Sender:   ./sender RECEIVER_IP TIMEOUT WINDOW_SIZE\n");
    fprintf(stdout, "Receiver: ./receiver PACKET_LOSS_PROBABILITY\n");
    fprintf(stdout, "---------------------------------------------------\n");
    exit(1);
}


#endif
