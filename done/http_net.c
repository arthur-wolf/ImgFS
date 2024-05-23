/*
 * @file http_net.c
 * @brief HTTP server layer for CS-202 project
 *
 * @author Konstantinos Prasopoulos
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>

#include "http_prot.h"
#include "http_net.h"
#include "socket_layer.h"
#include "error.h"

static int passive_socket = -1;
static EventCallback cb;

#define MK_OUR_ERR(X) \
static int our_ ## X = X

MK_OUR_ERR(ERR_NONE);
MK_OUR_ERR(ERR_INVALID_ARGUMENT);
MK_OUR_ERR(ERR_OUT_OF_MEMORY);
MK_OUR_ERR(ERR_IO);

/*******************************************************************
 * Handle connection
 */
static void *handle_connection(void *arg)
{
    if (arg == NULL) return &our_ERR_INVALID_ARGUMENT;

    int *sock_ptr = (int *)arg;
    int sock = *sock_ptr;
    static int our_ERR_IO = ERR_IO;
    static int our_ERR_NONE = ERR_NONE;

    char buffer[MAX_HEADER_SIZE];
    memset(buffer, 0, MAX_HEADER_SIZE);

    // Read the header
    int total_read = 0;
    while (total_read < MAX_HEADER_SIZE) {
        int bytes_read = tcp_read(sock, buffer + total_read, MAX_HEADER_SIZE - total_read);
        if (bytes_read < 0) {
            return &our_ERR_IO;
        }
        total_read += bytes_read;

        // Check if we have received the end of the headers
        if (strstr(buffer, HTTP_HDR_END_DELIM) != NULL) {
            break;
        }
    }

    // Check if we have read the entire header
    if (total_read >= MAX_HEADER_SIZE) {
        return &our_ERR_IO;
    }

    // Check for the "test: ok" header
    if (strstr(buffer, "test: ok") != NULL) {
        if (http_reply(sock, HTTP_OK, "", "", 0) != ERR_NONE) {
            return &our_ERR_IO;
        }
    } else {
        if (http_reply(sock, HTTP_BAD_REQUEST, "", "", 0) != ERR_NONE) {
            return &our_ERR_IO;
        }
    }

    return &our_ERR_NONE;
}



/*******************************************************************
 * Init connection
 */
int http_init(uint16_t port, EventCallback callback)
{
    passive_socket = tcp_server_init(port);
    cb = callback;
    return passive_socket;
}

/*******************************************************************
 * Close connection
 */
void http_close(void)
{
    if (passive_socket > 0) {
        if (close(passive_socket) == -1)
            perror("close() in http_close()");
        else
            passive_socket = -1;
    }
}

/*******************************************************************
 * Receive content
 */
int http_receive(void)
{
    int new_socket = tcp_accept(passive_socket);
    if (new_socket < 0) {
        return ERR_IO;
    }

    int *sock_ptr = malloc(sizeof(int));
    if (sock_ptr == NULL) {
        close(new_socket);
        return ERR_OUT_OF_MEMORY;
    }

    *sock_ptr = new_socket;

    void *result = handle_connection((void *)sock_ptr);
    int ret = *(int *)result;

    free(sock_ptr);
    close(new_socket);

    return ret;
}


/*******************************************************************
 * Serve a file content over HTTP
 */
int http_serve_file(int connection, const char* filename)
{
    int ret = ERR_NONE;
    return ret;
}

/*******************************************************************
 * Create and send HTTP reply
 */
int http_reply(int connection, const char* status, const char* headers, const char* body, size_t body_len)
{
    M_REQUIRE_NON_NULL(status);
    M_REQUIRE_NON_NULL(headers);
    if (connection < 0 || (body == NULL && body_len > 0)) {
        return ERR_INVALID_ARGUMENT;
    }

    const char* content_len = "Content-Length: ";
    char body_len_str [20];
    snprintf(body_len_str, "%zu", body_len);

    // Calculate the size of the header part
    size_t header_len = strlen(HTTP_PROTOCOL_ID) + strlen(status) + strlen(HTTP_LINE_DELIM) +
                        strlen(headers) + strlen(content_len) + strlen(body_len_str) + strlen(HTTP_HDR_END_DELIM);

    // Allocate buffer for the entire message
    size_t total_len = header_len + body_len + 1; 
    char *buffer = calloc(1, total_len);
    if (buffer == NULL) return ERR_OUT_OF_MEMORY;
    
    // Fill the buffer with the HTTP response
    int ret = snprintf(buffer, MAX_HEADER_SIZE, "%s%s%s%s%s%s%s", 
            HTTP_PROTOCOL_ID, status, HTTP_LINE_DELIM,
            headers, content_len, body_len_str, HTTP_HDR_END_DELIM);

    if (ret < 0 || (size_t)ret >= header_len) {
        free(buffer);
        return ERR_IO;
    }

    // Copy the body (if any) to the buffer
    if (body_len > 0 && body != NULL) {
        memcpy(buffer + header_len, body, body_len);
    }

    // Send the buffer to the socket
    ssize_t bytes_sent = send(connection, buffer, total_len - 1, 0);
    free(buffer);

    if (bytes_sent < 0 || (size_t)bytes_sent != total_len) {
        return ERR_IO;
    }

    return ERR_NONE;
}

