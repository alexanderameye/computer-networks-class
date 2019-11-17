#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>

/* FUNCTIONS */
void wait_for_user_input(); // ask user for file name input
void initialize_sender_socket(); // create and bind sender socket
void *handle_file_transmission(void *filename); // handle file transmission
void initialize_receiver_address();

/* SENDER/RECEIVER INFO */
int sender_socket, receiver_socket;
struct sockaddr_in sender_address, receiver_address;
struct hostent *receiver;


/* SENDING AND RECEIVING */
struct packet received_data, sent_data;
int bytes_sent;
char filename[150];
long file_length = 0;
char *buffer; //holds data of sent file

/* PACKETS */
int total_number_of_packets = -1;
int timeout, window_size;


socklen_t receiver_address_length;


int main(int argc, char *argv[]) {
    /* PARSE COMMAND LINE ARGUMENTS */
    if (argc != 4) usage_error();
    receiver = (struct hostent *) gethostbyname(argv[1]);
    timeout = atoi(argv[2]);
    window_size = atoi(argv[3]);
    printf("%s[PROGRAM EXECUTION]\nRECEIVER IP: %s%s\n%sTIMEOUT: %s%d\n%sWINDOW SIZE: %s%d\n%sPORT NUMBER: %s10080\n\n",
           COLOR_SERVER_CONTENT, COLOR_NEUTRAL, receiver->h_name, COLOR_SERVER_CONTENT, COLOR_NEUTRAL, timeout,
           COLOR_SERVER_CONTENT, COLOR_NEUTRAL,
           window_size, COLOR_SERVER_CONTENT, COLOR_NEUTRAL);

    initialize_sender_socket();
    initialize_receiver_address();

    wait_for_user_input();
}


void initialize_sender_socket() {
    /* CREATE SENDER SOCKET */
    if ((sender_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1) die("Socket creation failed");
    else live("Socket created");

    /* INITIALIZE SENDER ADDRESS */
    memset((char *) &sender_address, 0, sizeof(sender_address));
    sender_address.sin_family = SOCKET_DOMAIN;
    sender_address.sin_addr.s_addr = INADDR_ANY;
    sender_address.sin_port = htons(PORT);

    /* ALLOW US TO RERUN SERVER IMMEDIATELY AFTER KILLING IT */
    setsockopt(sender_socket, SOL_SOCKET, SO_REUSEADDR,
               (const void *) 1, sizeof(int));
    char str[INET_ADDRSTRLEN];
}

void initialize_receiver_address()
{
    /* INITIALIZE RECEIVER ADDRESS USING USER-ENTERED IP ADDRESS */
    memset((char *) &receiver_address, 0, sizeof(receiver_address));
    receiver_address.sin_family = SOCKET_DOMAIN;
    receiver_address.sin_addr = *((struct in_addr *) receiver->h_addr);
    receiver_address.sin_port = htons(PORT);
}

void wait_for_user_input() {
    /* ASK USER FOR A FILE CONTINUOUSLY */
    while (1) {
        printf("File Name: ");
        scanf("%s", filename);

        pthread_t main_thread;
        if (pthread_create(&main_thread, NULL, handle_file_transmission, (void *) filename) == -1)
            die("Thread creation failed");
        else live("File transmission thread created");
        pthread_join(main_thread, NULL);
    }
}

void *handle_file_transmission(void *filename) {
    char *name;
    if (filename) {
        /* OPEN FILE */
        name = (char *) filename;
        FILE *file;
        file = fopen(name, "rb");
        if (!file) die("File not found");

        /* READ FILE */
        fseek(file, 0, SEEK_END);
        file_length = ftell(file);
        rewind(file);
        buffer = (char *) malloc((file_length) * sizeof(char));
        if (!buffer) die("File too large to fit into memory");
        fread(buffer, file_length, 1, file);
        fclose(file);
        printf("File read, %ld bytes.\n", file_length);

        /* DIVIDE FILE INTO PACKETS */
        total_number_of_packets = ((file_length - 1) / PACKETSIZE) + 1;
        printf("File will be divided into %d packets of %d bytes and 1 packet of %ld bytes.\n\n",
               total_number_of_packets - 1,
               PACKETSIZE, (file_length) - ((total_number_of_packets - 1) * PACKETSIZE));

        /* SEND FILE NAME */
        memset(&sent_data, 0, sizeof(struct packet));
        sent_data.type = DATA;
        sent_data.sequence_number = 0;
        strcpy(sent_data.data, name);
        sent_data.length = strlen(name);


        //send from sender_socket to receiver address
        bytes_sent = sendto(sender_socket, &sent_data, sizeof(struct packet), 0, (struct sockaddr *) &receiver_address,
                            sizeof(struct sockaddr));


        //sendto(socket, buffer containing data, size of buf, flags, dest addr, addr length)
        //we know dest addr because we inputted it, we don't need receiver to send us something first


        //RECEIVE ACKS LATER
        //recvfrom(socket, buf to receive data, len of buf, flags, src addr, addr length)

    }

}





