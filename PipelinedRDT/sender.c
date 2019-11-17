#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

void serve(int port);

int sender_socket, receiver_socket;
char *buffer; //holds data of sent file
int timeout, window_size;
struct sockaddr_in sender_address, receiver_address;
char *receiver_ip_address;

char filename[150];
long file_length = 0;


int total_number_of_packets = -1;

int main(int argc, char *argv[]) {
    if (argc != 4) usage_error();

    /* PARSE COMMAND LINE ARGUMENTS */
    receiver_ip_address = argv[1];
    timeout = atoi(argv[2]);
    window_size = atoi(argv[3]);

    printf("%s[PROGRAM EXECUTION]\nRECEIVER IP: %s%s\n%sTIMEOUT: %s%d\n%sWINDOW SIZE: %s%d\n%sPORT NUMBER: %s10080\n\n",
           COLOR_SERVER_CONTENT, COLOR_NEUTRAL, receiver_ip_address, COLOR_SERVER_CONTENT, COLOR_NEUTRAL, timeout,
           COLOR_SERVER_CONTENT, COLOR_NEUTRAL,
           window_size, COLOR_SERVER_CONTENT, COLOR_NEUTRAL);

    serve(PORT);
}


void serve(int port) {
    /* VARIABLES */
    socklen_t receiver_address_length;

    /* CREATE SENDER SOCKET */
    if ((sender_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1) die("Socket creation failed");
    else live("Socket created");

    /* ALLOW US TO RERUN SERVER IMMEDIATELY AFTER KILLING IT */
    setsockopt(sender_socket, SOL_SOCKET, SO_REUSEADDR,
               (const void *) 1, sizeof(int));

    /* SET UP ADDRESS */
    memset((char *) &sender_address, 0, sizeof(sender_address));
    sender_address.sin_family = SOCKET_DOMAIN;
    sender_address.sin_addr.s_addr = htonl(INADDR_ANY);
    sender_address.sin_port = htons(port);

    /* BIND PORT NUMBER TO SENDER SOCKET */
    if (bind(sender_socket, (struct sockaddr *) &sender_address, sizeof(sender_address)) == -1)
        die("Socket bind failed");
    else live("Socket bound\n");



    /* OPEN FILE */
    printf("File name: ");
    scanf("%s", filename);
    FILE *file;
    printf("%s", filename);
    file = fopen(filename, "rb");
    if(!file) die("File not found");

    /* READ FILE */
    fseek(file, 0, SEEK_END);
    file_length = ftell(file);
    rewind(file);
    buffer = (char *)malloc((file_length)*sizeof(char));
    if(!buffer) die("File too large to fit into memory");
    fread(buffer, file_length, 1, file);
    fclose(file);
    printf("File read, %ld bytes.\n", file_length);

    /* DIVIDE FILE INTO PACKETS */
    total_number_of_packets = ((file_length-1)/PACKETSIZE) +1;
    printf("File will be divided into %d packets of %d bytes and 1 packet of %ld bytes.\n", total_number_of_packets-1, PACKETSIZE, (file_length)-((total_number_of_packets-1) * PACKETSIZE));








    receiver_address_length = sizeof(struct sockaddr);

}
