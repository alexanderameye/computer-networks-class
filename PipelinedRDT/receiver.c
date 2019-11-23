#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/timeb.h>

/* FUNCTIONS */
void init(int argc, char *argv[]);

void initialize_receiver_socket();

double calculate_elapsed_time(struct timeb start, struct timeb current);

/* SENDER */
struct sockaddr_in sender_address;


/* RECEIVER */
int receiver_socket;
struct sockaddr_in receiver_address;
struct packet received_data, send_data;
double packet_loss_probability;
int packet_was_lost = 0, loss_count = 0;
int bytes_read, bytes_sent;
long expected_packet = -1;
socklen_t addr_size = sizeof(struct sockaddr);
int number_of_sent_packets = 0, number_of_received_packets = 0, number_of_unique_packets = 0;

/* FILE */
char *file_buffer = NULL;
FILE *file;
long buffer_size = 0;

/* Program entry point  */
int main(int argc, char *argv[]) {
    init(argc, argv);
    initialize_receiver_socket();

    /* LOG FILE */
    FILE *log_file;

    /* FILE */
    char file_name[150];

    /* TIMING */
    struct timeb start_time, current_time;
    ftime(&start_time);
    double elapsed_time;

    while (1) {
        number_of_received_packets++;

        /* receive packet and generate loss */
        bytes_read = recvfrom(receiver_socket, &received_data, sizeof(struct packet), 0,
                              (struct sockaddr *) &sender_address, &addr_size);
        packet_was_lost = random_loss(packet_loss_probability, &loss_count);

        if (expected_packet == -1) {
            /* file name packet */
            ftime(&start_time);
            expected_packet = 0;
            strcpy(file_name, received_data.data);

            char log_file_name[150] = "receive_log_";
            strcat(log_file_name, file_name);
            strcat(log_file_name, ".txt");
            log_file = fopen(log_file_name, "wb");

            fprintf(log_file, "Receiving file: %s\n", file_name);
            fprintf(log_file, "Packet loss probability: %.2f%%\n\n", (double) packet_loss_probability * 100);
            fprintf(log_file, "=================================================================\n");
            fprintf(log_file, "| Time\t| Type | Index\t| Action   | Extra\t\t\t|\n");
            fprintf(log_file, "=================================================================\n");
        }

        ftime(&current_time);

        if (packet_was_lost && received_data.type != FINAL) {
            /* packet dropped */
            fprintf(log_file, "| %.3f\t| pkt  | %d\t| received | dropped\t\t\t|\n",
                    calculate_elapsed_time(start_time, current_time), received_data.sequence_number);
            printf("%sLOST    %s   pkt %s%d%s\n", COLOR_NEGATIVE, COLOR_NEUTRAL, COLOR_NUMBER,
                   received_data.sequence_number, COLOR_NEUTRAL);
            continue;
        } else if (!packet_was_lost && received_data.type != FINAL) {
            /* send ack and read packet */
            fprintf(log_file, "| %.3f\t| pkt  | %d\t| received | %d bytes\t\t\t|\n",
                    calculate_elapsed_time(start_time, current_time), received_data.sequence_number,
                    received_data.length);

            memset(&send_data, 0, sizeof(struct packet));
            send_data.type = ACK;
            send_data.length = 0;

            if (received_data.sequence_number <= expected_packet) {
                /* previous packet not lost, read packet */

                if (received_data.sequence_number != 0) { /* read packet if it is a data packet */


                    if (!file_buffer) {
                        buffer_size = sizeof(char) * received_data.total_length;
                        file_buffer = (char *) malloc(buffer_size);
                        if (!file_buffer) die("Not enough space for receiver file buffer.");
                    }

                    memcpy(file_buffer + (received_data.sequence_number - 1) * PACKETSIZE, received_data.data,
                           received_data.length);
                }

                number_of_unique_packets++;
                send_data.sequence_number = received_data.sequence_number;
                expected_packet = send_data.sequence_number + 1;
                sendto(receiver_socket, &send_data, sizeof(struct packet), 0, (struct sockaddr *) &sender_address,
                       sizeof(struct sockaddr));
                ftime(&current_time);
                fprintf(log_file, "| %.3f\t| ack  | %d\t| sent     | expect %ld\t\t\t|\n",
                        calculate_elapsed_time(start_time, current_time), send_data.sequence_number, expected_packet);
            } else {
                /* previous packet lost */
                send_data.sequence_number = expected_packet - 1;
                sendto(receiver_socket, &send_data, sizeof(struct packet), 0, (struct sockaddr *) &sender_address,
                       sizeof(struct sockaddr));
                ftime(&current_time);
                fprintf(log_file, "| %.3f\t| ack  | %d\t| sent     | resent, expect %ld\t\t|\n",
                        calculate_elapsed_time(start_time, current_time), send_data.sequence_number, expected_packet);
            }
            number_of_sent_packets++;
        }

        /* transmission done */
        if (received_data.type == FINAL) {
            ftime(&current_time);
            elapsed_time = calculate_elapsed_time(start_time, current_time);
            fprintf(log_file, "| %.3f\t| pkt  | %d\t| received | final\t\t\t|\n", elapsed_time,
                    received_data.sequence_number);
            fprintf(log_file, "=================================================================\n\n");
            fprintf(log_file, "\nSent packets: %d\n", number_of_sent_packets);
            fprintf(log_file, "Unique packets: %d\n", number_of_unique_packets);
            fprintf(log_file, "Received packets: %d\n", number_of_received_packets);
            fprintf(log_file, "Lost packets: %d\n", loss_count);
            fprintf(log_file, "Elapsed time: %.3f seconds \n", elapsed_time);
            fprintf(log_file, "Throughput: %.2f pkts/sec\n", number_of_sent_packets / elapsed_time);
            fprintf(log_file, "Goodput: %.2f pkts/sec\n", number_of_unique_packets / elapsed_time);
            fprintf(log_file, "Badput: %.2f pkts/sec\n",
                    (number_of_sent_packets - number_of_unique_packets) / elapsed_time);
            fprintf(log_file, "Efficiency: %.2f%%\n",
                    ((double) number_of_unique_packets / (double) number_of_sent_packets) * 100);
            fprintf(log_file, "Target packet loss: %.3f%%\n", (double) packet_loss_probability * 100);
            fprintf(log_file, "Actual packet loss: %.3f%%\n",
                    (double) (((double) loss_count) / ((double) number_of_received_packets)) * 100);

            fclose(log_file);

            file = fopen(strcat(file_name, "_copy"), "wb");
            if (file) {
                fwrite(file_buffer, buffer_size, 1, file);
            } else die("Error writing file");

            return 0;
        }
    }
}


/* Parses command line arguments */
void init(int argc, char *argv[]) {
    srand48(time(NULL));
    if (argc != 2) usage_error();
    packet_loss_probability = atof(argv[1]);
    printf("\n%s==================================================================", COLOR_ACTION);
    printf("\n%sRECEIVER IP: %s%s\n%sPORT: %s%d\n%sPACKET LOSS PROBABILITY: %s%.2f%%%s\n",
           COLOR_CONTENT, COLOR_NUMBER, RECEIVER_IP, COLOR_CONTENT, COLOR_NUMBER, PORT, COLOR_CONTENT, COLOR_NUMBER,
           (double) packet_loss_probability * 100, COLOR_NEUTRAL);
}

/* Creates a receiver socket, initializes its address and binds it */
void initialize_receiver_socket() {
    if ((receiver_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1) die("Socket creation failed");

    memset((char *) &receiver_address, 0, sizeof(receiver_address));
    receiver_address.sin_family = SOCKET_DOMAIN;
    inet_pton(AF_INET, RECEIVER_IP, &(receiver_address.sin_addr));
    receiver_address.sin_port = htons(PORT);

    /* increase receive buffer */



    /* allow us to restart receiver right after it was closed*/
    setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR,
               (const void *) 1, sizeof(int));
    char str[INET_ADDRSTRLEN];

    if (bind(receiver_socket, (struct sockaddr *) &receiver_address, sizeof(receiver_address)) == -1)
        die("Socket bind failed");

    /* get receive buffer size */
    int socket_buffer_size;
    socklen_t len = sizeof(socket_buffer_size);
    getsockopt(receiver_socket, SOL_SOCKET, SO_RCVBUF, &socket_buffer_size, &len);

    printf("%sBUFFER SIZE: %s%d\n", COLOR_CONTENT, COLOR_NUMBER, socket_buffer_size);

    /* increase receive buffer size */
    socket_buffer_size = 10 * 1024 * 1024;

    setsockopt(receiver_socket, SOL_SOCKET, SO_RCVBUF, &socket_buffer_size, sizeof(socket_buffer_size));
    len = sizeof(socket_buffer_size);
    getsockopt(receiver_socket, SOL_SOCKET, SO_RCVBUF, &socket_buffer_size, &len);

    printf("%sUPDATED BUFFER SIZE: %s%d%s", COLOR_CONTENT, COLOR_NUMBER, socket_buffer_size, COLOR_CONTENT);
    printf("\n%s===================================================================\n\n", COLOR_ACTION);
}

double calculate_elapsed_time(struct timeb start, struct timeb current) {
    return ((1000.0 * (current.time - start.time) +
             (current.millitm - start.millitm))) /
           1000;
}
