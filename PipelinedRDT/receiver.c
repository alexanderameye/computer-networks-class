#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

double packet_loss_probability;
char *buffer = NULL; //holds received data
int receiver_socket;
struct sockaddr_in receiver_address, sender_address;

/* FUNCTIONS */
void initialize_receiver_socket();

/* SENT AND RECEIVED DATA */
struct packet received_data;
struct packet sent_data;
int bytes_read;
socklen_t addr_size;

int main(int argc, char *argv[]) {
    /* PARSE COMMAND LINE ARGUMENTS */
    if (argc != 2) usage_error();
    packet_loss_probability = atof(argv[1]);
    printf("%s[RECEIVER EXECUTION]\nRECEIVER IP: %s%s\n%sPACKET LOSS PROBABILITY: %s%f\n\n",
           COLOR_CONTENT, COLOR_NEUTRAL, RECEIVER_IP, COLOR_CONTENT, COLOR_NEUTRAL,
           packet_loss_probability);

    initialize_receiver_socket();


    addr_size = sizeof(struct sockaddr);

    while (1) {
        bytes_read = recvfrom(receiver_socket, &received_data, sizeof(struct packet), 0,
                              (struct sockaddr *) &sender_address, &addr_size);
        char filename[150];
        strcpy(filename, received_data.data);
        printf("The file %s will be sent\n", filename);
    }
}

void initialize_receiver_socket() {
    /* CREATE RECEIVER SOCKET */
    if ((receiver_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1) die("Socket creation failed");
    else live("Socket created");

    /* INITIALIZE RECEIVER ADDRESS */
    memset((char *) &receiver_address, 0, sizeof(receiver_address));
    receiver_address.sin_family = SOCKET_DOMAIN;
    inet_pton(AF_INET, RECEIVER_IP, &(receiver_address.sin_addr));
    receiver_address.sin_port = htons(PORT);

    /* ALLOW US TO RERUN SERVER IMMEDIATELY AFTER KILLING IT */
    setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR,
               (const void *) 1, sizeof(int));
    char str[INET_ADDRSTRLEN];

    /* BIND RECEIVER ADDRESS TO SOCKET */
    if (bind(receiver_socket, (struct sockaddr *) &receiver_address, sizeof(receiver_address)) == -1)
        die("Socket bind failed");
    else live("Socket bound\n");
}
