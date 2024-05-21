#include "imgfs.h"
#include "util.h"
#include "socket_layer.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SIZE_THRESHOLD 1024
#define MAX_SIZE 2048
#define MAX_BUFFER MAX_SIZE + sizeof("<EOF>")

void serve(int server_sock)
{
    while (1) {
        printf("-------------------------\n");
        printf("[+] Waiting for a size...\n");

        // Accept a new client connection
        int client_sock = tcp_accept(server_sock);
        if (client_sock < 0) continue; // Skip to the next client
        printf("[+] Client connected\n");

        char buffer[MAX_BUFFER];

        // -------------- FILE SIZE --------------------

        // Read the size of the file
        tcp_read(client_sock, buffer, MAX_SIZE);

        if (strcmp(buffer, "[-] ERROR : file size is too big\n") == 0) {
            printf("%s", buffer);
            printf("[+] Closing the connection ...\n\n");
            close(client_sock);
            continue;
        }

        // Extract the size of the file
        int size = atoi(buffer);
        printf("[+] Received file size: %d --> accepted\n", size);

        // Send the response acknowledging the size
        if (size > SIZE_THRESHOLD) {
            tcp_send(client_sock, "Big file", sizeof("Big file"));
        } else {
            tcp_send(client_sock, "Small file", sizeof("Small file"));
        }

        // -------------- RECEIVE FILE --------------------

        printf("[+] About to receive a file of %d bytes ...\n", size);

        // Read the file
        tcp_read(client_sock, buffer, MAX_BUFFER - 1);  // Read into buffer with space for null-terminator

        char *eof = strstr(buffer, "<EOF>");
        if (eof != NULL) {
            *eof = '\0'; // Remove the EOF marker
            printf("[+] Received file :\n");
            printf("******************************************\n");
            printf("%s", buffer);
            printf("******************************************\n");
            tcp_send(client_sock, "[+] File received successfully\n", sizeof("[+] File received successfully\n"));
        } else {
            printf("[-] File transfer failed");
            tcp_send(client_sock, "[-] File transfer failed\n", sizeof("[-] File transfer failed\n"));
        }

        printf("[+] Closing the connection ...\n\n");

        // Close the connection
        close(client_sock);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "[-] Usage: %s <port>\n", argv[0]);
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    int server_sock = tcp_server_init(atouint16(argv[1]));
    if (server_sock < 0) {
        fprintf(stderr, "[-] Failed to initialize server on port %s\n", argv[1]);
        return ERR_IO;
    }
    printf("[+] Server started on port %s\n\n", argv[1]);

    serve(server_sock);

    // The server never reaches this point
    close(server_sock);
    return 0;
}
