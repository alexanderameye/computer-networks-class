#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr,"usage: %s ip timeout windowsize\n",argv[0]);
        exit(1);
    }

    /* VARIABLES */
    int sender_socket, receiver_socket;
    struct sockaddr_in sender_address, receiver_address;
    char buffer[MAXBUFFERLENGTH];

    int addr_len, numbytes;
    struct timeval time;
    struct timezone zone;

    char *receiver_ip_address = argv[1];
    int timeout = atoi(argv[2]);
    int window_size = atoi(argv[3]);

    char *hello = "Hello from server";

    printf("%s[PROGRAM EXECUTION]\nIP: %s%s\n%sTIMEOUT: %s%d\n%sWINDOW SIZE: %s%d\n%sPORT NUMBER: %s10080\n\n", COLOR_SERVER_CONTENT, COLOR_NEUTRAL, receiver_ip_address, COLOR_SERVER_CONTENT, COLOR_NEUTRAL, timeout, COLOR_SERVER_CONTENT, COLOR_NEUTRAL,
           window_size, COLOR_SERVER_CONTENT, COLOR_NEUTRAL);


    /* CREATE SENDER SOCKET */
    if ((sender_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1) die("Socket creation failed");
    else live("Socket created");

    /* ALLOWS US TO RERUN SERVER IMMEDIATELY AFTER WE KILL IT */
    setsockopt(sender_socket, SOL_SOCKET, SO_REUSEADDR,
               (const void *)1, sizeof(int));

    /* BUILD SENDER ADDRESS */
    sender_address.sin_family = SOCKET_DOMAIN;
    sender_address.sin_addr.s_addr = INADDR_ANY; //inet_addr(receiver_ip_address); //only accept 1 IP
    sender_address.sin_port = htons(PORT);
    memset(&(sender_address.sin_zero), '\0', sizeof(sender_address.sin_zero));

    /* BIND PORT NUMBER TO SENDER SOCKET */
    if (bind(sender_socket, (struct sockaddr *) &sender_address, sizeof(sender_address)) == -1)
        die("Socket bind failed");
    else live("Socket bound");





    /* WAIT FOR DATAGRAMS */
    printf("Before the loop \n");
    addr_len = sizeof(struct sockaddr);

    if (gettimeofday(&time,&zone) < 0 ){
        perror("bind");
        exit(1);
    };
    if ((numbytes=recvfrom(sender_socket, buffer, MAXBUFFERLENGTH, 0, \
                            (struct sockaddr *)&receiver_address, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    printf(" After first packet - Time= %ld and %ld \n",time.tv_sec, time.tv_usec);

    for (int i=1;i<=100;i++){
        if ((numbytes=recvfrom(sender_socket, buffer, MAXBUFFERLENGTH, 0, \
               	            (struct sockaddr *)&receiver_address, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        if (gettimeofday(&time,&zone) < 0 ){
            perror("bind");
            exit(1);
        };
        printf("Time= %ld and %ld ",time.tv_sec, time.tv_usec);

        printf("got packet %d from %s ",i,inet_ntoa(receiver_address.sin_addr));
        printf("packet is %d bytes long ",numbytes);
        buffer[numbytes] = '\0';
        printf("packet contains \"%s\"\n",buffer);
    }


    if (gettimeofday(&time,&zone) < 0 ){
        perror("bind");
        exit(1);
    };
    printf("Time= %ld and %ld \n",time.tv_sec, time.tv_usec);

    printf("got packet from %s\n",inet_ntoa(receiver_address.sin_addr));
    printf("packet is %d bytes long\n",numbytes);
    buffer[numbytes] = '\0';
    printf("packet contains \"%s\"\n",buffer);

    close(sender_socket);
}
