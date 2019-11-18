#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/poll.h>
#include <unistd.h>

/* FUNCTIONS */
void init(int argc, char *argv[]);

void initialize_sender_socket();

void initialize_receiver_address();

void ask_for_user_input();

void *handle_file_transmission(void *filename);

void transmission_done();

/* SENDER AND RECEIVER INFO */
int sender_socket, receiver_socket;
struct sockaddr_in sender_address, receiver_address;
struct hostent *receiver;

/* SENDING AND RECEIVING */
struct packet received_data, sent_data;
int bytes_sent, bytes_read;

/* FILE */
char filename[150];
long file_length = 0;
char *file_buffer; // to read data of served file

/* PACKETS */
int total_number_of_packets = -1;
int timeout, window_size;

int window_start = 0;
int window_end = 0;
int request_number = 0;
int sn = 0;
long seq_num = 0;
struct pollfd ufd;
int rv;

socklen_t addr_size = sizeof(struct sockaddr);


int main(int argc, char *argv[]) {
    init(argc, argv);
    initialize_sender_socket();
    initialize_receiver_address();
    ask_for_user_input();
}

/* Parses command line arguments */
void init(int argc, char *argv[]) {
    if (argc != 4) usage_error();
    receiver = (struct hostent *) gethostbyname(argv[1]);
    timeout = atoi(argv[2]);
    window_size = atoi(argv[3]);
    printf("\n%s======================", COLOR_ACTION);
    printf("\n[SENDER CREATED]");
    printf("\n======================");
    printf("\n%sRECEIVER IP: %s%s\n%sTIMEOUT: %s%d\n%sWINDOW SIZE: %s%d\n%sPORT NUMBER: %s%d",
           COLOR_CONTENT, COLOR_NEUTRAL, receiver->h_name, COLOR_CONTENT, COLOR_NEUTRAL, timeout,
           COLOR_CONTENT, COLOR_NEUTRAL,
           window_size, COLOR_CONTENT, COLOR_NEUTRAL, PORT);
    printf("\n%s======================%s\n\n", COLOR_ACTION, COLOR_NEUTRAL);
    window_end = window_size - 1;
}

/* Creates a sender socket and initializes its address */
void initialize_sender_socket() {
    if ((sender_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1) die("Socket creation failed");
    else live("Socket created\n");

    memset((char *) &sender_address, 0, sizeof(sender_address));
    sender_address.sin_family = SOCKET_DOMAIN;
    sender_address.sin_addr.s_addr = INADDR_ANY;
    sender_address.sin_port = htons(PORT);

    setsockopt(sender_socket, SOL_SOCKET, SO_REUSEADDR,
               (const void *) 1, sizeof(int));
    char str[INET_ADDRSTRLEN];
}

/* Initializes an address for the receiver based on the entered IP address */
void initialize_receiver_address() {
    memset((char *) &receiver_address, 0, sizeof(receiver_address));
    receiver_address.sin_family = SOCKET_DOMAIN;
    receiver_address.sin_addr = *((struct in_addr *) receiver->h_addr);
    receiver_address.sin_port = htons(PORT);
}

/* Continuously asks the user for a file to be transmitted */
void ask_for_user_input() {
    while (1) {
        live("Waiting for a file to be transmitted");
        printf("%sFile name: %s", COLOR_CONTENT, COLOR_NEUTRAL);
        scanf("%s", filename);

        pthread_t main_thread;
        if (pthread_create(&main_thread, NULL, handle_file_transmission, (void *) filename) == -1)
            die("Thread creation failed");
        else live("File transmission thread created");
        pthread_join(main_thread, NULL);
    }
}

/* Handles the file transmission to the receiver */
void *handle_file_transmission(void *filename) {
    long last_ACK = 0; // We know that the first ACK# we get will be 1024


    /* READ FILE */
    FILE *file;
    file = fopen(filename, "rb");
    if (!file) die("File not found");
    fseek(file, 0, SEEK_END);
    file_length = ftell(file);
    rewind(file);
    file_buffer = (char *) malloc((file_length) * sizeof(char));
    if (!file_buffer) die("File too large");
    fread(file_buffer, file_length, 1, file);
    fclose(file);
    live("File fetched");
    printf("%sFile size: %s%ld bytes%s\n", COLOR_CONTENT, COLOR_NUMBER, file_length, COLOR_NEUTRAL);

    /* DIVIDE FILE INTO PACKETS */
    total_number_of_packets = ((file_length - 1) / PACKETSIZE) + 1; // +1 to compensate for integer division
    printf("%sPackets: %s%d%s packets of %s%d bytes%s, %s1%s packet of %s%ld bytes%s\n\n", COLOR_CONTENT, COLOR_NUMBER,
           total_number_of_packets - 1, COLOR_NEUTRAL, COLOR_NUMBER,
           PACKETSIZE, COLOR_NEUTRAL, COLOR_NUMBER, COLOR_NEUTRAL, COLOR_NUMBER,
           (file_length - 1) - ((total_number_of_packets - 1) * PACKETSIZE), COLOR_NEUTRAL);


    goto servicerequest;


    while (1) {
        //receive
        memset(&received_data, 0, sizeof(struct packet));
        ufd.fd = sender_socket;
        ufd.events = POLLIN;
        printf("Polling for a client request.....\n");
        rv = poll(&ufd, 1, timeout);
        if (rv == -1) {
            perror("poll");
        } else if (rv == 0) //timeout so resend packets starting from last_ack + 1
        {
            printf("%sTIMEOUT%s\n", COLOR_NEGATIVE, COLOR_NEUTRAL);
            received_data.sequence_number = last_ACK;
            seq_num = last_ACK;
            received_data.type = ACK;
            sn = window_start;
            printf("Resending at most %d packets beginning with sequence number %ld\n", window_size, last_ACK);
            printf("%.0f%% of packets have been reliably transferred.\n", (double) last_ACK / (double) PACKETSIZE);
            sleep(2);
        } else {
            if (ufd.revents & POLLIN) {
                bytes_read = recvfrom(sender_socket, &received_data, sizeof(struct packet), 0,
                                      (struct sockaddr *) &receiver_address, &addr_size);
            }
        }

        servicerequest:
        if (last_ACK < file_length) {
            request_number = received_data.sequence_number / (int) PACKETSIZE;


            /* if previous packet was ACKED then shift window*/
            if (request_number > window_start && last_ACK == (received_data.sequence_number - PACKETSIZE)) {
                window_end = window_end + (request_number - 0);
                window_start = request_number;
                last_ACK += PACKETSIZE;
            }

            /* while there are packets to send and we are within the current window */
            while (sn < total_number_of_packets && sn <= window_end &&
                   sn >= window_start) {
                memset(&sent_data, 0, sizeof(struct packet));
                sent_data.type = DATA;
                sent_data.sequence_number = seq_num;
                if (sn == window_start) seq_num = last_ACK;
                int buffadd = seq_num;
                char *chunk = &file_buffer[buffadd];
                if ((file_length - seq_num) < PACKETSIZE) sent_data.length = (file_length % PACKETSIZE);
                else sent_data.length = PACKETSIZE;
                memcpy(sent_data.data, chunk, sent_data.length);
                sent_data.total_length = file_length;

                bytes_sent = sendto(sender_socket, &sent_data, sizeof(struct packet), 0, (
                        struct sockaddr *) &receiver_address, sizeof(struct sockaddr));

                print_packet_info_server(&sent_data);

                seq_num += (bytes_sent - packet_header_size());
                sn++;
            }
        }

        /* transmission done */
        if (last_ACK >= file_length) transmission_done();
    }
}

/* send a FINAL packet to the receiver indicating the transmission was completed*/
void transmission_done() {
    memset(&sent_data, 0, sizeof(struct packet));
    sent_data.type = FINAL;
    sent_data.sequence_number = 0;
    sent_data.length = 0;

    live("File transmission completed");
    printf("%sBytes transmitted: %s%ld bytes%s\n", COLOR_CONTENT, COLOR_NUMBER, file_length, COLOR_NEUTRAL);
    bytes_sent = sendto(sender_socket, &sent_data, sizeof(struct packet), 0, (struct sockaddr *) &receiver_address,
                        sizeof(struct sockaddr));
}





