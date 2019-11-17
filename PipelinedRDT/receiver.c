#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

double packet_loss_probability;
char *buffer = NULL; //holds received data

int main(int argc, char *argv[])
{
    if (argc != 2) usage_error();

    /* PARSE COMMAND LINE ARGUMENTS */
    packet_loss_probability = atof(argv[1]);

    printf("%s[PROGRAM EXECUTION]\nPACKET LOSS PROBABILITY: %s%f\n\n",COLOR_SERVER_CONTENT, COLOR_NEUTRAL, packet_loss_probability);

}

