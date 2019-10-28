#include "common.h"

void die(char *message, ...) {
    va_list ap;
    va_start(ap, message);
    printf("%s[CLIENT] ", COLOR_NEGATIVE);
    printf("%s%s\n", COLOR_NEUTRAL, message);
    vfprintf(stdout, message, ap);
    fprintf(stdout, "\n");
    fflush(stdout);
    perror("");
    va_end(ap);
    exit(1);
}

void live(char *message, ...) {
    va_list ap;
    va_start(ap, message);
    printf("%s[CLIENT] ", COLOR_ACTION);
    printf("%s", COLOR_NEUTRAL);
    vfprintf(stdout, message, ap);
    fprintf(stdout, "\n");
    fflush(stdout);
    va_end(ap);
}

int main(int argc, char *argv[]) {
    /* VARIABLES */
    int n;
    int client_socket;
    struct socket_address server_address;
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int send_bytes;

    /* CREATE CLIENT SOCKET */
    /* creates a client socket */
    if ((client_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) == -1)
        die("Socket creation failed");
    else
        live("Socket created");

    bzero(&server_address, sizeof(server_address));
    server_address.family = AF_INET;
    server_address.port = htons(SERVER_PORT); // htons converts host byte order -> network byte order
    server_address.address.s_addr = inet_addr(IP_ADDRESS);

    /* CONNECT */
    /* connects client socket to server socket */
    if (connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
        close(client_socket);
        die("Connection with server failed");
    }

    char object_path_name[200];
    scanf("%s", object_path_name);

    /* WRITE CLIENT REQUEST */
    sprintf(request, "GET /%s HTTP/1.1\r\n\r\n", object_path_name);
    send_bytes = strlen(request);

    if (write(client_socket, request, send_bytes) != send_bytes)
        die("Writing client request failed");
    else
        live("Client request sent:\n%s%s%s", COLOR_CLIENT_CONTENT, request, COLOR_NEUTRAL);

    /* READ SERVER RESPONSE */
    while ((n = read(client_socket, response, BUFFER_SIZE - 1)) > 0) {
        live("Message received:\n%s%s%s\n", COLOR_SERVER_CONTENT, response,
             COLOR_NEUTRAL);
        memset(response, 0, BUFFER_SIZE);
    }

    if (n < 0) {
        die("Reading message failed");
        close(client_socket);
    } else if (n == 0) {
        live("Ending connection");
        close(client_socket);
    }

    close(client_socket);
    return 0;
}