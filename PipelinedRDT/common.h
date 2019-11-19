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
    ACK,
    FINAL
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
#define COLOR_CONTENT "\033[1;36m"
#define COLOR_NEUTRAL "\033[0m"
#define COLOR_NEGATIVE "\033[1;31m"
#define COLOR_NUMBER "\033[0;32m"
#define COLOR_POSITIVE "\033[0;32m"


/* Logs an error message to the terminal */
void die(char *message, ...) {
    va_list ap;
    va_start(ap, message);
    printf("%sSender: ", COLOR_NEGATIVE);
    printf("%s%s\n", COLOR_NEUTRAL, message);
    vfprintf(stdout, message, ap);
    fprintf(stdout, "\n");
    fflush(stdout);
    perror("");
    va_end(ap);
    exit(1);
}

typedef enum {
    SENDING,
    RECEIVING
} action;

int packet_header_size() {
    return sizeof(struct packet) - PACKETSIZE; // Minus the 512 bytes of data
}


int random_loss(const double probability, int *counter) {
    double random = drand48(); //generate random double [0.0, 1.0]
    if (random >= probability) return 0;
    else {
        *counter = *counter + 1;
        return 1;
    }
}


void print_packet_info_sender(const struct packet *packet, action type) {
    if (type == SENDING) {
        printf("%sSENT    %s   pkt %s%d%s", COLOR_ACTION, COLOR_NEUTRAL, COLOR_NUMBER,
               packet->sequence_number, COLOR_NEUTRAL);
        if (packet->length < PACKETSIZE && packet->length > 0)
            printf(" with size %s%d bytes%s", COLOR_NUMBER, packet->length,
                   COLOR_NEUTRAL);
        printf("\n");
    } else {
        printf("%sRECEIVED%s   ack %s%d%s", COLOR_CONTENT, COLOR_NEUTRAL, COLOR_NUMBER,
               packet->sequence_number, COLOR_NEUTRAL);
        printf("\n");
    }
    return;
}

void print_packet_info_receiver(const struct packet *packet, action type) {
    if (type == RECEIVING) {
        printf("%sRECEIVED%s   pkt %s%d%s", COLOR_CONTENT, COLOR_NEUTRAL, COLOR_NUMBER,
               packet->sequence_number, COLOR_NEUTRAL);
        if (packet->length < PACKETSIZE && packet->length > 0)
            printf(" with size %s%d bytes%s", COLOR_NUMBER, packet->length,
                   COLOR_NEUTRAL); // for last packet, not full size so specify
        printf("\n");
    } else {
        printf("%sSENT    %s   ack %s%d%s", COLOR_ACTION, COLOR_NEUTRAL, COLOR_NUMBER,
               packet->sequence_number, COLOR_NEUTRAL);
        printf("\n");
    }
    return;
}

/* Logs a success or neutral message to the terminal */
void live(char *message, ...) {
    va_list ap;
    va_start(ap, message);
    printf("%s[ACTION] ", COLOR_ACTION);
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
