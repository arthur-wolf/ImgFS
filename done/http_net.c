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

int http_serve_file(int connection, const char* filename)
{
    M_REQUIRE_NON_NULL(filename);

    // open file
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "http_serve_file(): Failed to open file \"%s\"\n", filename);
        return http_reply(connection, "404 Not Found", "", "", 0);
    }

    // get its size
    fseek(file, 0, SEEK_END);
    const long pos = ftell(file);
    if (pos < 0) {
        fprintf(stderr, "http_serve_file(): Failed to tell file size of \"%s\"\n",
                filename);
        fclose(file);
        return ERR_IO;
    }
    rewind(file);
    const size_t file_size = (size_t) pos;

    // read file content
    char* const buffer = calloc(file_size + 1, 1);
    if (buffer == NULL) {
        fprintf(stderr, "http_serve_file(): Failed to allocate memory to serve \"%s\"\n", filename);
        fclose(file);
        return ERR_IO;
    }

    const size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        fprintf(stderr, "http_serve_file(): Failed to read \"%s\"\n", filename);
        fclose(file);
        return ERR_IO;
    }

    // send the file
    const int  ret = http_reply(connection, HTTP_OK,
                                "Content-Type: text/html; charset=utf-8" HTTP_LINE_DELIM,
                                buffer, file_size);

    // garbage collecting
    fclose(file);
    free(buffer);
    return ret;
}

/*******************************************************************
 * Handle connection
 */
static void *handle_connection(void *arg)
{
    if (arg == NULL) return &our_ERR_INVALID_ARGUMENT;

    int *sock_ptr = (int *)arg;
    int sock = *sock_ptr;

    char *rcvbuf = calloc(1, MAX_HEADER_SIZE);
    if (rcvbuf == NULL) return &our_ERR_OUT_OF_MEMORY;
    memset(rcvbuf, 0, MAX_HEADER_SIZE);

    struct http_message message;
    size_t total_read = 0;
    int content_len = 0;
    int extend_message = 0;
    int buffer_size = MAX_HEADER_SIZE;

    while (1) {
        ssize_t bytes_read = tcp_read(sock, rcvbuf + total_read, (size_t)buffer_size - total_read);

        // Connection closed
        if (bytes_read == 0) break;

        // Error
        if (bytes_read < 0) {
            free(rcvbuf);
            return &our_ERR_IO;
        }

        total_read += (size_t)bytes_read;
        int parse_result = http_parse_message(rcvbuf, total_read, &message, &content_len);

        // Error
        if (parse_result < 0) {
            free(rcvbuf);
            return &parse_result;
        }

        // Incomplete message
        if (parse_result == 0) {
            // Extend the buffer if needed
            if (!extend_message && content_len > 0 && total_read < (size_t)(content_len + MAX_HEADER_SIZE)) {
                extend_message = 1;
                buffer_size += content_len;
                char *new_rcvbuf = realloc(rcvbuf, (size_t)(buffer_size + content_len));
                if (new_rcvbuf == NULL) {
                    free(rcvbuf);
                    return &our_ERR_OUT_OF_MEMORY;
                }
                rcvbuf = new_rcvbuf;
            } else {
                continue;
            }
        }

        // Full message received
        if (parse_result > 0) {
            cb(&message, sock);
            memset(rcvbuf, 0, MAX_HEADER_SIZE);
            total_read = 0;
            content_len = 0;
            extend_message = 0;
            buffer_size = MAX_HEADER_SIZE;
        }
    }

    free(rcvbuf);
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
        if (close(passive_socket) == -1) perror("close() in http_close()");
        else passive_socket = -1;
    }
}

/*******************************************************************
 * Receive content
 */
int http_receive(void)
{
    int new_socket = tcp_accept(passive_socket);
    if (new_socket < 0) return ERR_IO;

    int *sock_ptr = calloc(1, sizeof(int));
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
 * Computes the length of the body length
 */
size_t compute_body_length(size_t body_len)
{
    if (body_len == 0) return 1;

    size_t len = 0;
    while (body_len > 0) {
        body_len /= 10;
        len++;
    }
    return len;
}

/*******************************************************************
 * Create and send HTTP reply
 */
int http_reply(int connection, const char* status, const char* headers, const char *body, size_t body_len)
{
    M_REQUIRE_NON_NULL(headers);
    M_REQUIRE_NON_NULL(status);
    if(body_len != 0 && body == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    // Compute the length of the body length
    const char* content_len = "Content-Length: ";
    char body_len_str [20];
    sprintf(body_len_str, "%zu", body_len);

    // Compute the size of the header
    size_t header_size = strlen(HTTP_PROTOCOL_ID) + strlen(status) + strlen(HTTP_LINE_DELIM) 
                        + strlen(headers) + strlen(content_len)+ strlen(body_len_str) + strlen(HTTP_HDR_END_DELIM);
    if(header_size > MAX_HEADER_SIZE) return ERR_INVALID_ARGUMENT;
    
    // Compute the size of the buffer
    size_t buffer_size = header_size + body_len + 1;
    char* buffer = calloc(buffer_size, 1);
    if(buffer == NULL) return ERR_OUT_OF_MEMORY;

    // Creating header
    snprintf(buffer, MAX_HEADER_SIZE, "%s%s%s%s%s%s%s", HTTP_PROTOCOL_ID, status, 
            HTTP_LINE_DELIM, headers, content_len, body_len_str, HTTP_HDR_END_DELIM);

   // Adding body
    if(body_len > 0) memcpy(buffer + header_size, body, body_len);

    // Send the reply
    if(tcp_send(connection, buffer, buffer_size-1) < 0) {
        free(buffer);
        return ERR_IO;
    }

    free(buffer);
    return ERR_NONE;
}

