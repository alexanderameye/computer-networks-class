#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>

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
long expected_packet = 0;
socklen_t addr_size = sizeof(struct sockaddr);

/* FILE*/
char *file_buffer = NULL;
FILE *file;
long buffer_size = 0;

int main(int argc, char *argv[]) {
    init(argc, argv);
    initialize_receiver_socket();

    while (1) {
        /* receive packet and generate loss */
        bytes_read = recvfrom(receiver_socket, &received_data, sizeof(struct packet), 0,
                              (struct sockaddr *) &sender_address, &addr_size);
        packet_was_lost = random_loss(packet_loss_probability, &loss_count);
        if (packet_was_lost) {
            printf("%sPACKET LOST%s\n\n", COLOR_NEGATIVE, COLOR_NEUTRAL);
            continue;
        }
        print_packet_info_receiver(&received_data, RECEIVING);

        /* transmission done */
        if (received_data.type == FINAL) {
            transmission_done();
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
        send_data.length = 0; // ack has no length

        if (received_data.sequence_number <= expected_packet) {
            /* previous packet not lost */
            memcpy(file_buffer + received_data.sequence_number, received_data.data, received_data.length);
            send_data.sequence_number = received_data.sequence_number + PACKETSIZE;
            expected_packet = send_data.sequence_number;
            sendto(receiver_socket, &send_data, sizeof(struct packet), 0, (struct sockaddr *) &sender_address,
                   sizeof(struct sockaddr));
            print_packet_info_receiver(&send_data, SENDING);
        } else {
            /* previous packet lost */
            send_data.sequence_number = expected_packet;
            sendto(receiver_socket, &send_data, sizeof(struct packet), 0, (struct sockaddr *) &sender_address,
                   sizeof(struct sockaddr));
            print_packet_info_receiver(&send_data, SENDING);
        }

    }
}

/* send a FINAL packet to the sender acknowledging the transmission was completed*/
void transmission_done() {
    memset(&send_data, 0, sizeof(struct packet));
    send_data.type = FINAL;
    send_data.sequence_number = 0;
    send_data.length = 0;
    live("File transmission completed");
    printf("Treated %s%d%s packets as lost\n", COLOR_NUMBER, loss_count, COLOR_NEUTRAL);
    bytes_sent = sendto(receiver_socket, &send_data, sizeof(struct packet), 0, (struct sockaddr *) &sender_address,
                        sizeof(struct sockaddr));
    write_file_to_disk();
}

/* write the received packets to a file on the disk */
void write_file_to_disk() {
    file = fopen("text2", "wb");
    if (file) {
        fwrite(file_buffer, buffer_size, 1, file);
        live("File written");
    } else die("Error writing file");
}

/* Parses command line arguments */
void init(int argc, char *argv[]) {
    srand48(time(NULL));
    if (argc != 2) usage_error();
    packet_loss_probability = atof(argv[1]);
    printf("\n%s=================================", COLOR_ACTION);
    printf("\n[RECEIVER CREATED]");
    printf("\n=================================");
    printf("\n%sRECEIVER IP: %s%s\n%sPACKET LOSS PROBABILITY: %s%.0f%%",
           COLOR_CONTENT, COLOR_NEUTRAL, RECEIVER_IP, COLOR_CONTENT, COLOR_NEUTRAL,
           (double) packet_loss_probability * 100);
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
