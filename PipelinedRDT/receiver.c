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

void transmission_done();

void write_file_to_disk();

/* PACKET LOSS */
double packet_loss_probability;
int packet_was_lost = 0;
int loss_count = 0;

/* SENDING AND RECEIVING */
int receiver_socket;
struct sockaddr_in receiver_address, sender_address;
struct packet received_data, send_data;
int bytes_read, bytes_sent;
long expected_packet = -1;
socklen_t addr_size = sizeof(struct sockaddr);

/* FILE*/
char *file_buffer = NULL;
FILE *file;
long buffer_size = 0;


int number_of_sent_packets = 0;

int main(int argc, char *argv[]) {
    init(argc, argv);
    initialize_receiver_socket();

    /* log file */
    FILE *log_file;
    char log_file_name[150] = "receive_log_";
    strcat(log_file_name, "a");
    strcat(log_file_name, ".txt");
    log_file = fopen(log_file_name, "wb");

    fprintf(log_file, "Packet loss probability: %.2f%%\n\n",  (double) packet_loss_probability * 100);

    /* timing */
    struct timeb start_time, current_time;
    ftime(&start_time);
    double elapsed_time;

    while (1) {


        /* receive packet and generate loss */
        bytes_read = recvfrom(receiver_socket, &received_data, sizeof(struct packet), 0,
                              (struct sockaddr *) &sender_address, &addr_size);
        packet_was_lost = random_loss(packet_loss_probability, &loss_count);

        if (expected_packet == -1)
        {ftime(&start_time);
            expected_packet = 0;
        }


        ftime(&current_time);
        elapsed_time =
                ((1000.0 * (current_time.time - start_time.time) + (current_time.millitm - start_time.millitm))) / 1000;
        fprintf(log_file, "%.3f\t|  pkt: %d\t|  received\n", elapsed_time,
                received_data.sequence_number);

        if (packet_was_lost && received_data.type != FINAL) {
            ftime(&current_time);
            elapsed_time =
                    ((1000.0 * (current_time.time - start_time.time) + (current_time.millitm - start_time.millitm))) / 1000;
            fprintf(log_file, "%.3f\t|  pkt: %d\t|  arrived but dropped\n", elapsed_time, received_data.sequence_number);
            printf("%sLOST    %s   pkt %s%d%s\n", COLOR_NEGATIVE, COLOR_NEUTRAL, COLOR_NUMBER,
                   received_data.sequence_number, COLOR_NEUTRAL);
            continue;
        }


        /* transmission done */
        if (received_data.type == FINAL) {
            memset(&send_data, 0, sizeof(struct packet));
            send_data.type = FINAL;
            send_data.sequence_number = 0;
            send_data.length = 0;

            // printf("\nTreated %s%d%s packets as lost\n", COLOR_NUMBER, loss_count, COLOR_NEUTRAL);
              bytes_sent = sendto(receiver_socket, &send_data, sizeof(struct packet), 0, (struct sockaddr *) &sender_address,
                              sizeof(struct sockaddr));

            ftime(&current_time);
            elapsed_time =
                    ((1000.0 * (current_time.time - start_time.time) + (current_time.millitm - start_time.millitm))) /
                    1000;
            fprintf(log_file, "%.3f\t|  pkt: %d\t|  sent\n", elapsed_time,
                    send_data.sequence_number);

            fprintf(log_file, "\nTotal packets: %d\n", number_of_sent_packets);
            fprintf(log_file, "Lost packets: %d\n", loss_count);
            fprintf(log_file, "Elapsed time: %.3f seconds \n", elapsed_time);
            fprintf(log_file, "Throughput: %.2f pkts/sec\n", number_of_sent_packets / elapsed_time);
            fprintf(log_file, "Target packet loss: %.4f%%\n",  (double) packet_loss_probability * 100);
            fprintf(log_file, "Actual packet loss: %.4f%%\n",  (double) (((double) loss_count)/ ((double) number_of_sent_packets)) * 100);

            fclose(log_file);

            write_file_to_disk();
            return 0;
        }
        if (!file_buffer) {
            buffer_size = sizeof(char) * received_data.total_length;
            file_buffer = (char *) malloc(buffer_size);
            if (!file_buffer) die("Not enough space for client file buffer.");
        }

        /* send acknowledgement */
        memset(&send_data, 0, sizeof(struct packet));
        send_data.type = ACK;
        send_data.length = 0;

        if (received_data.sequence_number <= expected_packet) {
            /* previous packet not lost */
            memcpy(file_buffer + received_data.sequence_number * PACKETSIZE, received_data.data, received_data.length);
            send_data.sequence_number = received_data.sequence_number;
            expected_packet = send_data.sequence_number + 1;
            sendto(receiver_socket, &send_data, sizeof(struct packet), 0, (struct sockaddr *) &sender_address,
                   sizeof(struct sockaddr));
            number_of_sent_packets++;
            ftime(&current_time);
            elapsed_time =
                    ((1000.0 * (current_time.time - start_time.time) + (current_time.millitm - start_time.millitm))) /
                    1000;
            fprintf(log_file, "%.3f\t|  ack: %d\t|  sent\n", elapsed_time,
                    send_data.sequence_number);
        } else {
            /* previous packet lost */
            send_data.sequence_number = expected_packet - 1;
            sendto(receiver_socket, &send_data, sizeof(struct packet), 0, (struct sockaddr *) &sender_address,
                   sizeof(struct sockaddr));
            number_of_sent_packets++;
            ftime(&current_time);
            elapsed_time =
                    ((1000.0 * (current_time.time - start_time.time) + (current_time.millitm - start_time.millitm))) /
                    1000;
            fprintf(log_file, "%.3f\t|  ack: %d\t|  sent\n", elapsed_time,
                    send_data.sequence_number);
        }
    }
}

/* write the received packets to a file on the disk */
void write_file_to_disk() {
    file = fopen("text2", "wb");
    if (file) {
        fwrite(file_buffer, buffer_size, 1, file);
    } else die("Error writing file");
}

/* Parses command line arguments */
void init(int argc, char *argv[]) {
    srand48(time(NULL));
    if (argc != 2) usage_error();
    packet_loss_probability = atof(argv[1]);
    printf("\n%s==================================================================", COLOR_ACTION);
    printf("\n%sRECEIVER IP: %s%s\n%sPACKET LOSS PROBABILITY: %s%.2f%%\n%sBUFFER SIZE: \nUPDATED BUFFER SIZE:%s",
           COLOR_CONTENT, COLOR_NUMBER, RECEIVER_IP, COLOR_CONTENT, COLOR_NUMBER,
           (double) packet_loss_probability * 100, COLOR_CONTENT, COLOR_NEUTRAL);
    printf("\n%s===================================================================\n\n", COLOR_ACTION);
}

/* Creates a receiver socket, initializes its address and binds it */
void initialize_receiver_socket() {
    if ((receiver_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1) die("Socket creation failed");

    memset((char *) &receiver_address, 0, sizeof(receiver_address));
    receiver_address.sin_family = SOCKET_DOMAIN;
    inet_pton(AF_INET, RECEIVER_IP, &(receiver_address.sin_addr));
    receiver_address.sin_port = htons(PORT);

    setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR,
               (const void *) 1, sizeof(int));
    char str[INET_ADDRSTRLEN];

    if (bind(receiver_socket, (struct sockaddr *) &receiver_address, sizeof(receiver_address)) == -1)
        die("Socket bind failed");
}