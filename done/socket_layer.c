#include "imgfs.h"
#include "socket_layer.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>


#define MAX_PENDING_CONNECTIONS 5

int tcp_server_init(uint16_t port) 
{
    // Create a TCP socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("[-] Error opening socket");
        return ERR_IO;
    }
    printf("[+] TCP server socket created\n");
    
    // Set up the address structure and zero it out
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    // Initialize the address structure
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_port = htons(port); // Convert port to network byte order
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Bind to localhost

    // Binding the socket to the address
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("[-] Error binding socket");
        close(socket_fd);
        return ERR_IO;
    }
    printf("[+] Server socket binded %d\n", port);

    // Listen for incoming connections
    if (listen(socket_fd, MAX_PENDING_CONNECTIONS) == -1) { // Allow up to 5 connections to wait in the queue
        perror("[-] Error listening on socket");
        close(socket_fd);
        return ERR_IO;
    }
    printf("[+] Server socket listening...\n");
    
    return socket_fd;
}

/**
 * @brief Blocking call that accepts a new TCP connection
 */
int tcp_accept(int passive_socket) 
{
    return accept(passive_socket, NULL, NULL);
}

/**
 * @brief Blocking call that reads the active socket once and stores the output in buf
 */
ssize_t tcp_read(int active_socket, char* buf, size_t buflen)
{
    M_REQUIRE_NON_NULL(buf);
    return recv(active_socket, buf, buflen, 0);
}

ssize_t tcp_send(int active_socket, const char* response, size_t response_len)
{
    M_REQUIRE_NON_NULL(response);
    return send(active_socket, response, response_len, 0);
}
