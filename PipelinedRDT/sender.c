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

void transmission_done(time_t start, int total_number_of_packets, int number_of_sent_packets);

void *handle_file_transmission(void *filename);


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

int timeout;
int window_size;


int main(int argc, char *argv[]) {
    init(argc, argv);
    initialize_sender_socket();
    initialize_receiver_address();

    /* request file input */
    while (1) {
        printf("%sFile: %s", COLOR_CONTENT, COLOR_NEUTRAL);
        scanf("%s", filename);

        pthread_t main_thread;
        if (pthread_create(&main_thread, NULL, handle_file_transmission, (void *) filename) == -1)
            die("Thread creation failed");
        pthread_join(main_thread, NULL);
    }
}

/* Parses command line arguments */
void init(int argc, char *argv[]) {
    if (argc != 4) usage_error();
    receiver = (struct hostent *) gethostbyname(argv[1]);
    timeout = atoi(argv[2]);
    window_size = atoi(argv[3]);
    printf("\n%s==================================================================", COLOR_ACTION);
    printf("\n%sRECEIVER IP: %s%s\n%sTIMEOUT: %s%d\n%sWINDOW SIZE: %s%d\n%sPORT: %s%d",
           COLOR_CONTENT, COLOR_NUMBER, receiver->h_name, COLOR_CONTENT, COLOR_NUMBER, timeout,
           COLOR_CONTENT, COLOR_NUMBER,
           window_size, COLOR_CONTENT, COLOR_NUMBER, PORT);
    printf("\n%s===================================================================\n\n", COLOR_ACTION);
}

/* Creates a sender socket and initializes its address */
void initialize_sender_socket() {
    if ((sender_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1) die("Socket creation failed");

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

/* Handles the file transmission to the receiver */
void *handle_file_transmission(void *name) {
    FILE *log_file;
    char log_file_name[150] = "send_log_";
    strcat(log_file_name, name);
    strcat(log_file_name,".txt");
    log_file = fopen(log_file_name, "wb");


    int total_number_of_packets = -1, number_of_sent_packets = 0;
    int window_start = 0, window_end = 0;
    window_end = window_size - 1;
    int request_number = 0;
    int current_packet = 0;
    long seq_num = 0;
    struct pollfd ufd;
    int rv;

    /* GOODPUT */
    time_t start = time(NULL);
    long last_ACK = -1;
    received_data.sequence_number = -1;

    /* READ FILE */
    file = fopen(name, "rb");
    if (!file) die("File not found");
    fseek(file, 0, SEEK_END);
    file_length = ftell(file);
    rewind(file);
    file_buffer = (char *) malloc((file_length) * sizeof(char));
    if (!file_buffer) die("File too large");
    fread(file_buffer, file_length, 1, file);
    fclose(file);

    /* divide file into packets */
    total_number_of_packets = ((file_length - 1) / PACKETSIZE) + 1; // +1 to compensate for integer division

    fprintf(log_file, "File size: %ld bytes\n", file_length);
    fprintf(log_file, "Packets: %d packets of %d bytes, 1 packet of %ld bytes\n\n", total_number_of_packets - 1,
            PACKETSIZE, file_length - ((total_number_of_packets - 1) * PACKETSIZE));

    goto servicerequest;


    while (1) {
        memset(&received_data, 0, sizeof(struct packet));
        ufd.fd = sender_socket;
        ufd.events = POLLIN;
        if ((rv = poll(&ufd, 1, timeout)) == -1) die("Polling error");
        else if (rv == 0) {
            //printf("\n%sTIMEOUT %s%dms%s", COLOR_NEGATIVE, COLOR_NUMBER, timeout, COLOR_NEUTRAL);
            received_data.sequence_number = last_ACK;
            seq_num = last_ACK + 1;
            received_data.type = ACK;
            current_packet = window_start;
            //printf("\nResending at most %d packet(s) starting from pkt %ld\n", window_size,
            //    last_ACK + 1);
            // printf("%s%.0f%% %sof packets transmitted\n\n", COLOR_NUMBER,
            //  ((double) last_ACK / total_number_of_packets) * 100, COLOR_NEUTRAL);
        } else if (ufd.revents & POLLIN) {
            bytes_read = recvfrom(sender_socket, &received_data, sizeof(struct packet), 0,
                                  (struct sockaddr *) &receiver_address, &addr_size);
        }

        fprintf(log_file, "ack: %d | received\n", received_data.sequence_number);

//        print_packet_info_sender(&received_data, RECEIVING);

        servicerequest:
        if (last_ACK + 1 < total_number_of_packets) {
            request_number = received_data.sequence_number;

            /* is the ACK valid and should the window be shifted, or should the ACK be ignored*/
            if (request_number >= window_start && last_ACK == received_data.sequence_number - 1) {
                window_end = window_end + (request_number + 1 - window_start);
                window_start = request_number + 1;
                last_ACK += 1;
            }

            /* while there are packets to send and we are within the current window */
            while (current_packet <= total_number_of_packets && current_packet <= window_end &&
                   current_packet >= window_start) {
                memset(&send_data, 0, sizeof(struct packet));
                send_data.type = DATA;
                send_data.sequence_number = seq_num;

                if (current_packet == window_start) seq_num = last_ACK + 1;
                int buffadd = seq_num * PACKETSIZE;
                char *chunk = &file_buffer[buffadd];
                if ((file_length - ((seq_num - 1) * PACKETSIZE)) < PACKETSIZE)
                    send_data.length = (file_length % PACKETSIZE);
                else send_data.length = PACKETSIZE;
                memcpy(send_data.data, chunk, send_data.length);
                send_data.total_length = file_length;

                bytes_sent = sendto(sender_socket, &send_data, sizeof(struct packet), 0, (
                        struct sockaddr *) &receiver_address, sizeof(struct sockaddr));

                fprintf(log_file, "pkt: %d | sent\n", send_data.sequence_number);

                //  print_packet_info_sender(&send_data, SENDING);

                seq_num += ((bytes_sent - packet_header_size()) / PACKETSIZE);
                current_packet++;
                number_of_sent_packets++;
            }
        }

        /* transmission done */
        if (last_ACK + 1 >= total_number_of_packets) {
            memset(&send_data, 0, sizeof(struct packet));
            send_data.type = FINAL;
            send_data.sequence_number = 0;
            send_data.length = 0;

            time_t end = time(NULL);
            double elapsed = (end - start);

            /* printf("\n%s==================================================================", COLOR_ACTION);

             printf("\n[TRANSMISSION COMPLETED]");
             printf("\n==================================================================");
             printf("\n%sBytes: %s%ld%s\n", COLOR_CONTENT, COLOR_NUMBER, file_length, COLOR_NEUTRAL);*/
            bytes_sent = sendto(sender_socket, &send_data, sizeof(struct packet), 0,
                                (struct sockaddr *) &receiver_address,
                                sizeof(struct sockaddr));

            fprintf(log_file, "\nTotal packets: %d\n", number_of_sent_packets);
            fprintf(log_file, "Unique packets: %d\n", total_number_of_packets);
            fprintf(log_file, "Elapsed time: %f\n", elapsed);
            if(elapsed < 0.0001) fprintf(log_file, "Elapsed time too short to calculate accurate goodput.\n");
            else fprintf(log_file, "Goodput: %f\n", total_number_of_packets / elapsed);

            /*  printf("%sPackets (Unique): %s%d%s\n", COLOR_CONTENT, COLOR_NUMBER, total_number_of_packets, COLOR_NEUTRAL);
              printf("%sPackets (Total): %s%d%s\n", COLOR_CONTENT, COLOR_NUMBER, number_of_sent_packets, COLOR_NEUTRAL);
              if (elapsed < 0.00001) {
                  printf("%sElapsed time too short to calculate goodput.%s\n", COLOR_CONTENT, COLOR_NEUTRAL);
              } else {
                  printf("%sElapsed Time: %s%f\n", COLOR_CONTENT, COLOR_NUMBER, elapsed);
                  printf("%sGoodput: %s%f%s\n", COLOR_CONTENT, COLOR_NUMBER, total_number_of_packets / elapsed, COLOR_NEUTRAL);
              }
              printf("%s==================================================================\n\n%s", COLOR_ACTION, COLOR_NEUTRAL);*/
            fclose(log_file);
            return 0;
        }
    }
}






