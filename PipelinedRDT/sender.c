#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/poll.h>

/* FUNCTIONS */
void init(int argc, char *argv[]);

void initialize_sender_socket();

void initialize_receiver_address();

void ask_for_user_input();

void *handle_file_transmission(void *filename);

void transmission_done(time_t start);

/* SENDING AND RECEIVING */
int sender_socket, receiver_socket;
struct sockaddr_in sender_address, receiver_address;
struct packet received_data, send_data;
int bytes_sent, bytes_read;
struct hostent *receiver;
socklen_t addr_size = sizeof(struct sockaddr);

/* FILE*/
FILE *file;
char *file_buffer;
char filename[150];
long file_length = 0;

/* PACKETS */
int total_number_of_packets = -1;
int timeout;
int window_size, window_start = 0, window_end = 0;
int request_number = 0;
int current_packet = 0;
long seq_num = 0;
struct pollfd ufd;
int rv;
int number_of_sent_packets = 0; //non unique

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
    /* GOODPUT */
    time_t start = time(NULL);

    long last_ACK = 0;

    /* READ FILE */
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
        printf("\nLAST ACK: %ld\n", last_ACK);
        memset(&received_data, 0, sizeof(struct packet));
        ufd.fd = sender_socket;
        ufd.events = POLLIN;
        if ((rv = poll(&ufd, 1, timeout)) == -1) die("Polling error");
        else if (rv == 0) //timeout so resend packets starting from last_ack + 1
        {
            printf("\n%sTIMEOUT %d MILLISECONDS%s\n", COLOR_NEGATIVE, timeout, COLOR_NEUTRAL);
            received_data.sequence_number = last_ACK;
            seq_num = last_ACK;
            received_data.type = ACK;
            current_packet = window_start;
            printf("\nResending at most %d packet(s) starting from sequence number %ld\n", window_size,
                   last_ACK + PACKETSIZE);
            printf("%s%.0f%% %sof packets have been reliably transferred.\n\n", COLOR_NUMBER,
                   ((double) last_ACK / total_number_of_packets) * 100, COLOR_NEUTRAL);
            // sleep(0.5);
        } else if (ufd.revents & POLLIN) {
            bytes_read = recvfrom(sender_socket, &received_data, sizeof(struct packet), 0,
                                  (struct sockaddr *) &receiver_address, &addr_size);
        }

        print_packet_info_sender(&received_data, RECEIVING);

        servicerequest:
        //NEW
        if (last_ACK < total_number_of_packets) {
            //   if (last_ACK < file_length) {

            //NEW
            request_number = received_data.sequence_number; //initially the sequence number is 0 so request number is also 0
            // request_number = received_data.sequence_number / (int) PACKETSIZE;


            /* if previous packet was ACKED then shift window to the right */
            /* ack received so shift window and send next pkt */

            //NEW
            //for example lastACK is 0, then ack 1 arrives
            if (request_number > window_start && last_ACK == received_data.sequence_number - 1) {
                window_end = window_end + (request_number - window_start);
                window_start = request_number;
                last_ACK += 1;
            }

            /*if (request_number > window_start &&
                (last_ACK) == (received_data.sequence_number - PACKETSIZE)) { //for example last_ACK is 0, then received data seq number is 1400
                window_end = window_end + (request_number - window_start);
                window_start = request_number;
                last_ACK += PACKETSIZE;
            }*/

            /* while there are packets to send and we are within the current window */
            while (current_packet < total_number_of_packets && current_packet <= window_end &&
                   current_packet >= window_start) {
                memset(&send_data, 0, sizeof(struct packet));
                send_data.type = DATA;
                send_data.sequence_number = seq_num;
                if (current_packet == window_start) seq_num = last_ACK;
                //NEW
                int buffadd = seq_num * PACKETSIZE;
                //int buffadd = seq_num ;
                char *chunk = &file_buffer[buffadd];

                //NEW
                if ((file_length - (seq_num * PACKETSIZE)) < PACKETSIZE) send_data.length = (file_length % PACKETSIZE);
                    // if ((file_length - seq_num) < PACKETSIZE) send_data.length = (file_length % PACKETSIZE);
                else send_data.length = PACKETSIZE;


                memcpy(send_data.data, chunk, send_data.length);
                send_data.total_length = file_length;

                bytes_sent = sendto(sender_socket, &send_data, sizeof(struct packet), 0, (
                        struct sockaddr *) &receiver_address, sizeof(struct sockaddr));

                print_packet_info_sender(&send_data, SENDING);

                //NEW
                seq_num += ((bytes_sent - packet_header_size()) / PACKETSIZE);
                // seq_num += (bytes_sent - packet_header_size());
                current_packet++;

                number_of_sent_packets++;
            }
        }

        /* transmission done */
        //NEW
        //if(last_ACK+1>=total_number_of_packets) {
        if (last_ACK >= total_number_of_packets) {
            transmission_done(start);
            return 0;
        }
    }
}

/* send a FINAL packet to the receiver indicating the transmission was completed*/
void transmission_done(time_t start) {
    memset(&send_data, 0, sizeof(struct packet));
    send_data.type = FINAL;
    send_data.sequence_number = 0;
    send_data.length = 0;

    time_t end = time(NULL);
    double elapsed = (end - start);

    printf("\n%s=============================", COLOR_ACTION);
    printf("\n[FILE TRANSMISSION COMPLETED]");
    printf("\n=============================");
    printf("\n%sBytes: %s%ld%s\n", COLOR_CONTENT, COLOR_NUMBER, file_length, COLOR_NEUTRAL);
    bytes_sent = sendto(sender_socket, &send_data, sizeof(struct packet), 0, (struct sockaddr *) &receiver_address,
                        sizeof(struct sockaddr));
    printf("%sSent Packets (Unique): %s%d%s\n", COLOR_CONTENT, COLOR_NUMBER, total_number_of_packets, COLOR_NEUTRAL);
    printf("%sSent Packets (Total): %s%d%s\n", COLOR_CONTENT, COLOR_NUMBER, number_of_sent_packets, COLOR_NEUTRAL);
    if (elapsed < 0.00001) {
        printf("%sElapsed time too short to calculate goodput.%s\n", COLOR_CONTENT, COLOR_NEUTRAL);
    } else {
        printf("%sElapsed Time: %s%f\n", COLOR_CONTENT, COLOR_NUMBER, elapsed);
        printf("%sGoodput: %s%f%s", COLOR_CONTENT, COLOR_NUMBER, total_number_of_packets / elapsed, COLOR_NEUTRAL);
    }
    printf("\n%s=============================\n\n%s", COLOR_ACTION, COLOR_NEUTRAL);
}





