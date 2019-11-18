#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

/* FUNCTIONS */
void init(int argc, char *argv[]);

void initialize_receiver_socket();

/* PACKETS */
double packet_loss_probability;
char *file_buffer = NULL;

/* SENDER AND RECEIVER INFO */
int receiver_socket;
struct sockaddr_in receiver_address, sender_address;

/* SENDING AND RECEIVING */
struct packet received_data, sent_data;
int bytes_read;

socklen_t addr_size;

int main(int argc, char *argv[]) {
    init(argc, argv);
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

/* Parses command line arguments */
void init(int argc, char *argv[]) {
    if (argc != 2) usage_error();
    packet_loss_probability = atof(argv[1]);
    printf("\n%s=================================", COLOR_ACTION);
    printf("\n[RECEIVER CREATED]");
    printf("\n=================================");
    printf("\n%sRECEIVER IP: %s%s\n%sPACKET LOSS PROBABILITY: %s%f",
           COLOR_CONTENT, COLOR_NEUTRAL, RECEIVER_IP, COLOR_CONTENT, COLOR_NEUTRAL, packet_loss_probability);
    printf("\n%s=================================%s\n\n", COLOR_ACTION, COLOR_NEUTRAL);
}

/* Creates a receiver socket, initializes its address and binds it */
void initialize_receiver_socket() {
    if ((receiver_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1) die("Socket creation failed");
    else live("Socket created");

    memset((char *) &receiver_address, 0, sizeof(receiver_address));
    receiver_address.sin_family = SOCKET_DOMAIN;
    inet_pton(AF_INET, RECEIVER_IP, &(receiver_address.sin_addr));
    receiver_address.sin_port = htons(PORT);

    setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR,
               (const void *) 1, sizeof(int));
    char str[INET_ADDRSTRLEN];

    if (bind(receiver_socket, (struct sockaddr *) &receiver_address, sizeof(receiver_address)) == -1)
        die("Socket bind failed");
    else live("Socket bound\n");
}
