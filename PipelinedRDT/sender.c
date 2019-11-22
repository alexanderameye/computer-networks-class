#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/poll.h>
#include <sys/timeb.h>

/* FUNCTIONS */
void init(int argc, char *argv[]);

void initialize_sender_socket();

void initialize_receiver_address();

double calculate_elapsed_time(struct timeb start, struct timeb current);

void *handle_file_transmission(void *filename);

/* SENDER */
double timeout;
int window_size;
int sender_socket;
struct sockaddr_in sender_address;
struct packet received_data, send_data;
int bytes_sent, bytes_read;


/* RECEIVER */
int receiver_socket;
struct sockaddr_in receiver_address;
struct hostent *receiver;
socklen_t addr_size = sizeof(struct sockaddr);

/* FILE */
char filename[150];

/* Program entry point  */
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
    timeout = atof(argv[2]);
    window_size = atoi(argv[3]);
    printf("\n%s==================================================================", COLOR_ACTION);
    printf("\n%sRECEIVER IP: %s%s\n%sTIMEOUT: %s%f\n%sWINDOW SIZE: %s%d\n%sPORT: %s%d",
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
    /* LOG FILE */
    FILE *log_file;
    char log_file_name[150] = "send_log_";
    strcat(log_file_name, name);
    strcat(log_file_name, ".txt");
    log_file = fopen(log_file_name, "wb");

    /* TIMING */
    struct timeb start_time, current_time;
    ftime(&start_time);

    /* HOUSEKEEPING */
    int number_of_duplicate_acks = 0, received_ack = 0, current_packet = 0;
    int number_of_unique_packets, number_of_sent_packets = 0;
    int window_start = 0, window_end = window_size - 1;
    long seq_num = 0, last_ACK = -1;
    received_data.sequence_number = -1;

    /* TIMEOUT */
    struct pollfd ufd;
    int rv;

    /* READ FILE */
    FILE *file = fopen(name, "rb");
    if (!file) die("File not found");
    fseek(file, 0, SEEK_END);
    long file_length = ftell(file);
    rewind(file);
    char *file_buffer;
    file_buffer = (char *) malloc((file_length) * sizeof(char));
    if (!file_buffer) die("File too large");
    fread(file_buffer, file_length, 1, file);
    fclose(file);

    /* divide file into packets */
    number_of_unique_packets = ((file_length - 1) / PACKETSIZE) + 3;

    /* log file information */
    fprintf(log_file, "File name: %s\n", (char *) name);
    fprintf(log_file, "File size: %ld bytes\n", file_length);
    fprintf(log_file, "Data: %d packets of %d bytes, 1 packet of %ld bytes\n", number_of_unique_packets - 3,
            PACKETSIZE, file_length - ((number_of_unique_packets - 3) * PACKETSIZE));
    fprintf(log_file, "Other: 1 packet of %ld bytes and 1 packet of 0 bytes\n", strlen(name));
    fprintf(log_file, "Total: %d packets \n\n", number_of_unique_packets);
    fprintf(log_file, "=================================================================\n");
    fprintf(log_file, "| Time\t| Type | Index\t| Action   | Extra\t\t\t|\n");
    fprintf(log_file, "=================================================================\n");


    goto send_packets;

    while (1) {
        memset(&received_data, 0, sizeof(struct packet));
        ufd.fd = sender_socket, ufd.events = POLLIN;
        if ((rv = poll(&ufd, 1, timeout * 1000)) == -1) die("Polling error");
        else if (ufd.revents & POLLIN) { /* received ack */
            bytes_read = recvfrom(sender_socket, &received_data, sizeof(struct packet), 0,
                                  (struct sockaddr *) &receiver_address, &addr_size);

            ftime(&current_time);
            received_ack = received_data.sequence_number;

            if (received_ack == last_ACK) {
                /* duplicate ack */
                number_of_duplicate_acks += 1;
                fprintf(log_file, "| %.3f\t| ack  | %d\t| received | duplicate %d\t\t|\n",
                        calculate_elapsed_time(start_time, current_time),
                        received_data.sequence_number, number_of_duplicate_acks);
            } else if (received_ack == last_ACK + 1 && received_ack >= window_start) {
                /* ack in-order, shift window */
                window_end = window_end + (received_ack + 1 - window_start);
                window_start = received_ack + 1;
                last_ACK += 1;
                fprintf(log_file, "| %.3f\t| ack  | %d\t| received | window [%d,%d]\t\t|\n",
                        calculate_elapsed_time(start_time, current_time),
                        received_data.sequence_number, window_start, window_end);
            }
        } else if (rv == 0) {
            /* timeout */
            received_data.sequence_number = last_ACK;
            seq_num = last_ACK + 1;
            received_data.type = ACK;
            current_packet = window_start;

            ftime(&current_time);
            fprintf(log_file, "| %.3f\t| pkt  | %ld\t| timeout  | since %.3f\t\t|\n",
                    calculate_elapsed_time(start_time, current_time),
                    last_ACK + 1, calculate_elapsed_time(start_time, current_time) - timeout);
        }

        send_packets:

        /* send packets within window */
        if (last_ACK + 2 < number_of_unique_packets) {
            while (current_packet < number_of_unique_packets - 1 && current_packet <= window_end &&
                   current_packet >= window_start) {

                memset(&send_data, 0, sizeof(struct packet));
                send_data.type = DATA;
                send_data.sequence_number = seq_num;

                /* after timeout send first pkt that was not yet acked */
                if (current_packet == window_start) seq_num = last_ACK + 1;

              //  int buffer_address = seq
                if (seq_num == 0) {
                    /* file name packet */
                    send_data.length = strlen(name);

                    memcpy(send_data.data, name, send_data.length);
                    send_data.total_length = strlen(name);
                } else {
                    /* data packets */
                    if ((file_length - ((seq_num - 1) * PACKETSIZE)) < PACKETSIZE)
                        send_data.length = (file_length % PACKETSIZE);
                    else send_data.length = PACKETSIZE;

                    memcpy(send_data.data, &file_buffer[(seq_num-1) * PACKETSIZE], send_data.length);
                    send_data.total_length = file_length;
                }

                bytes_sent = sendto(sender_socket, &send_data, sizeof(struct packet), 0, (
                        struct sockaddr *) &receiver_address, sizeof(struct sockaddr));

                ftime(&current_time);

                if (current_packet == window_start && number_of_sent_packets != 0) {
                    fprintf(log_file, "| %.3f\t| pkt  | %d\t| sent     | retransmission %d bytes\t|\n",
                            calculate_elapsed_time(start_time, current_time),
                            send_data.sequence_number, send_data.length);
                } else
                    fprintf(log_file, "| %.3f\t| pkt  | %d\t| sent     | %d bytes\t\t\t|\n",
                            calculate_elapsed_time(start_time, current_time),
                            send_data.sequence_number, send_data.length);

                seq_num += ((bytes_sent - packet_header_size()) / PACKETSIZE);
                current_packet++;
                number_of_sent_packets++;
            }
        } else {
            /* transmission done, send final packet */
            memset(&send_data, 0, sizeof(struct packet));
            send_data.sequence_number = number_of_unique_packets - 1;
            send_data.type = FINAL;
            send_data.length = 0;

            bytes_sent = sendto(sender_socket, &send_data, sizeof(struct packet), 0,
                                (struct sockaddr *) &receiver_address,
                                sizeof(struct sockaddr));

            number_of_sent_packets++;

            ftime(&current_time);
            double elapsed_time = calculate_elapsed_time(start_time, current_time);
            fprintf(log_file, "| %.3f\t| pkt  | %d\t| sent     | final\t\t\t|\n", elapsed_time,
                    send_data.sequence_number);
            fprintf(log_file, "=================================================================\n");

            fprintf(log_file, "\nSent packets: %d\n", number_of_sent_packets);
            fprintf(log_file, "Unique packets: %d\n", number_of_unique_packets);
            fprintf(log_file, "Elapsed time: %.3f seconds \n", elapsed_time);
            fprintf(log_file, "Throughput: %.2f pkts/sec\n", number_of_sent_packets / elapsed_time);
            fprintf(log_file, "Goodput: %.2f pkts/sec\n", number_of_unique_packets / elapsed_time);
            fprintf(log_file, "Badput: %.2f pkts/sec\n",
                    (number_of_sent_packets - number_of_unique_packets) / elapsed_time);
            fprintf(log_file, "Efficiency: %.2f%%\n",
                    ((double) number_of_unique_packets / (double) number_of_sent_packets) * 100);

            fclose(log_file);
            return 0;
        }
    }
}

double calculate_elapsed_time(struct timeb start, struct timeb current) {
    return ((1000.0 * (current.time - start.time) +
             (current.millitm - start.millitm))) /
           1000;
}







