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
    //sigset_t mask;
    //sigemptyset(&mask);
    //sigaddset(&mask, SIGINT);
    //sigaddset(&mask, SIGTERM);
    //pthread_sigmask(SIG_BLOCK, &mask, NULL);

    if (arg == NULL) return &our_ERR_INVALID_ARGUMENT;
    int *socket_d = (int *)arg;

    char *rcvbuf = (char *)calloc(1, MAX_HEADER_SIZE + 1);
    if (rcvbuf == NULL)
    {
        close(*socket_d);
        return &our_ERR_IO;
    }

    ssize_t total_read = 0;
    ssize_t nb_read = 0;
    size_t header_size = 0;
    int found_end_delim = 0;
    int extended = 0;

    // Read the header until we find the end delimiter
    while (total_read < MAX_HEADER_SIZE)
    {
        nb_read = tcp_read(*socket_d, rcvbuf + total_read, MAX_HEADER_SIZE - (size_t)total_read);
        if (nb_read < 0)
        {
            free(rcvbuf);
            close(*socket_d);
            return &our_ERR_IO;
        }

        total_read += nb_read;
        char *end_delim_position = strstr(rcvbuf, HTTP_HDR_END_DELIM);

        // If we found the end delimiter, we stop reading
        if (end_delim_position != NULL)
        {
            header_size = (size_t)(end_delim_position - rcvbuf) + strlen(HTTP_HDR_END_DELIM);
            found_end_delim = 1;
            break;
        }
    }

    if (!found_end_delim)
    {
        free(rcvbuf);
        close(*socket_d);
        return &our_ERR_IO;
    }

    struct http_message http_message_out;
    int content_length;

    // Parse the message contained in the buffer
    while (1)
    {
        // Parse the message contained in the buffer
        int parsing_error = http_parse_message(rcvbuf, (size_t)total_read, &http_message_out, &content_length);

        if (parsing_error < 0)
        {
            free(rcvbuf);
            close(*socket_d);
            return &our_ERR_IO;
        }
        else if (parsing_error == 0)
        {

            // If we did not extend the message yet and we have a partial body
            if (!extended && (content_length > 0 && total_read < content_length))
            {
                // Extend the message buffer until being able to store everything
                // (header & content) into
                size_t new_size = header_size + (size_t)content_length;
                char *new_rcvbuf = (char *)realloc(rcvbuf, new_size);
                if (new_rcvbuf == NULL)
                {
                    free(rcvbuf);
                    close(*socket_d);
                    return &our_ERR_OUT_OF_MEMORY;
                }

                // We extend only once
                rcvbuf = new_rcvbuf;
                extended = 1;

                // Continue reading from the socket until the entire message is read
                while ((size_t)total_read < new_size)
                {
                    // Position the reading pointer to the right position
                    nb_read = tcp_read(*socket_d, rcvbuf + total_read, new_size - (size_t)total_read);
                    if (nb_read < 0)
                    {
                        free(rcvbuf);
                        close(*socket_d);
                        return &our_ERR_IO;
                    }
                    total_read += nb_read;
                }
            }
        }
        else if (parsing_error > 0)
        {

            // Call the event call back
            if (cb(&http_message_out, *socket_d) != 0)
            {
                free(rcvbuf);
                close(*socket_d);
                return &our_ERR_IO;
            }
            // Reset variables for a new tcp reading round
            total_read = 0;
            nb_read = 0;
            found_end_delim = 0;
            content_length = 0;
            header_size = 0;
            extended = 0;
            break;
        }
    }

    free(rcvbuf);
    close(*socket_d);
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
    if (new_socket < 0) return ERR_IO;


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
    M_REQUIRE_NON_NULL(status);
    M_REQUIRE_NON_NULL(headers);
    if(body == NULL && body_len != 0){
        return ERR_INVALID_ARGUMENT;
    }

    const char* content_len = "Content-Length: ";
    char body_len_str [20];
    sprintf(body_len_str, "%zu", body_len);

    size_t header_size = strlen(HTTP_PROTOCOL_ID) + strlen(status)
                         + strlen(HTTP_LINE_DELIM) + strlen(headers) + strlen(content_len)
                         + strlen(body_len_str) + strlen(HTTP_HDR_END_DELIM);

    if(header_size > MAX_HEADER_SIZE){
        return ERR_INVALID_ARGUMENT;
    }
    size_t buffer_size = header_size + body_len + 1;
    char* buffer = calloc(buffer_size, 1);
    if(buffer == NULL){
        return ERR_OUT_OF_MEMORY;
    }

    //------ Creating header -------------------
    snprintf(buffer, MAX_HEADER_SIZE, "%s%s%s%s%s%s%s",
            HTTP_PROTOCOL_ID, status, HTTP_LINE_DELIM,
            headers, content_len, body_len_str, HTTP_HDR_END_DELIM);

    //------------ Body -----------------------
    if(body_len > 0) {
        memcpy(buffer + header_size, body, body_len);
    }

    if(tcp_send(connection, buffer, buffer_size-1) < 0){
        free(buffer);
        return ERR_IO;
    }
    free(buffer);
    return ERR_NONE;
}

