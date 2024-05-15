#include "imgfs.h"
#include "util.h"
#include "socket_layer.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_SIZE 2048
#define MAX_BUFFER MAX_SIZE + sizeof("<EOF>")

int get_file_size(const char* file_path) {
    FILE* file = fopen(file_path, "rb");
    if (file == NULL) {
        perror("Failed to open file");
        return ERR_IO;
    }

    fseek(file, 0, SEEK_END);
    int file_size = (int)ftell(file);
    fclose(file);

    return file_size;
}

int connect_client(int argc, char **argv) {
    uint16_t port = atouint16(argv[1]);
    
    // Check the number of arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_port> <file_path>\n", argv[0]);
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("[-] Error creating socket");
        return ERR_IO;
    }

    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-] Failed to connect to server");
        close(client_sock);
        return ERR_IO;
    }

    printf("[+] Talking to server on port %d\n", port);

    return client_sock;
}

int main(int argc, char **argv) 
{
    int client_sock = connect_client(argc, argv);
    char* file_path = argv[2];

    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);

    // ------------------- FILE SIZE ---------------------------

    // Get the file size
    int file_size = get_file_size(file_path);
    if (file_size < 0) return file_size; 
    else if ((size_t)file_size > MAX_SIZE) {
        tcp_send(client_sock, "[-] ERROR : file size is too big\n", sizeof("[-] ERROR : file size is too big\n"));
        fprintf(stderr, "[-] File size is too big. Abort ...\n");
        printf("[+] Closing the connection ...\n");
        close(client_sock);
        return ERR_IO;
    }

    printf("[+] Sending file size: %d\n", file_size);

    // Write the file size to the buffer with the delimiter
    sprintf(buffer, "%d|", file_size);

    // Send the file size to the server
    tcp_send(client_sock, buffer, strlen(buffer));
    
    // Read the response from the server
    tcp_read(client_sock, buffer, MAX_SIZE);
    printf("[+] Server response: %s\n", buffer);

    // ------------------- FILE TRANSFER ---------------------------

    // Open the file
    printf("[+] Sending file: %s\n", file_path);
    FILE* file = fopen(file_path, "rb");
    if (file == NULL) {
        perror("[-] Failed to open file");
        return ERR_IO;
    }

    // Read the file and send it to the server
    fread(buffer, 1, (size_t)file_size, file);  // Read only the file content
    snprintf(buffer + file_size, sizeof("<EOF>"), "<EOF>");  // Add the EOF marker
    tcp_send(client_sock, buffer, (size_t)file_size + strlen("<EOF>"));  // Send file content + EOF marker

    printf("[+] File sent\n");

    // Read the response from the server
    tcp_read(client_sock, buffer, MAX_SIZE);
    printf("%s", buffer);

    // Close the file
    fclose(file);

    printf("[+] Closing the connection ...\n");

    // Close the connection
    close(client_sock);
    return 0;
}
