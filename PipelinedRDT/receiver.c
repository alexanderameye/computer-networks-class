#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr,"usage: %s hostname message\n",argv[0]);
        exit(1);
    }


    /************************/
    /*       Definitions    */
    /************************/
    int receiver_socket;
    struct sockaddr_in their_addr; 	/*Server address information 	*/
    struct hostent *he;		/*Struct for name resolving	*/
    int numbytes;			/*Number of bytes that were sent*/
    int I; 				/*Index			      */



    /*--------------------------*/
    /*Communication preparation */
    /*--------------------------*/

    if ((he=gethostbyname(argv[1])) == NULL) {  /*Transform host name into address	*/
        herror("gethostbyname");
        exit(1);
    }

    if ((receiver_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) { /*Socket definition	*/
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET;      			/*Scoket address family		*/
    their_addr.sin_port = htons(PORT);  			/*Port in network byte order 	*/
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);	/*Host address			*/
    bzero(&(their_addr.sin_zero), 8);     			/* zero the rest of the struct	*/


    /*----------------------*/
    /*Communication process */
    /*----------------------*/

    for (I=1;I<100;I++){

        printf("Now sending packet number %d \n",I);
        if ((numbytes=sendto(receiver_socket, argv[2], strlen(argv[2]), 0, \
        	     (struct sockaddr *)&their_addr, sizeof(struct sockaddr))) == -1) {
            perror("sendto");
            exit(1);
        }
    }



    printf("sent %d bytes to %s\n",numbytes,inet_ntoa(their_addr.sin_addr));

    close(receiver_socket);

    return 0;
}

