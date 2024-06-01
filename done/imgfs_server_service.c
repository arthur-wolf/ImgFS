/*
 * @file imgfs_server_services.c
 * @brief ImgFS server part, bridge between HTTP server layer and ImgFS library
 *
 * @author Konstantinos Prasopoulos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // uint16_t

#include "error.h"
#include "util.h" // atouint16
#include "imgfs.h"
#include "http_net.h"
#include "imgfs_server_service.h"

// Main in-memory structure for imgFS
static struct imgfs_file fs_file;
static uint16_t server_port;

#define MAX_RESOLUTION 10

#define URI_ROOT "/imgfs"

/********************************************************************//**
 * Startup function. Create imgFS file and load in-memory structure.
 * Pass the imgFS file name as argv[1] and optionnaly port number as argv[2]
 ********************************************************************** */
int server_startup (int argc, char **argv)
{
    if (argc < 2) return ERR_NOT_ENOUGH_ARGUMENTS;

    const char* imgfs_file_name = argv[1];

    // Open the imgFS file
    int err = do_open(imgfs_file_name, "rb+", &fs_file);
    if (err < 0) return err;

    // Print the header of the imgFS file
    print_header(&fs_file.header);

    // Handle the port number
    if (argc == 3 && argv[2] != NULL) {
        server_port = atouint16(argv[2]);
    } else {
        server_port = DEFAULT_LISTENING_PORT;
    }

    // Initialize the HTTP connection
    err = http_init(server_port, &handle_http_message);
    if (err <0) return err;

    printf("ImgFS server started on http://localhost:%u\n", server_port);

    return ERR_NONE;
}

/********************************************************************//**
 * Shutdown function. Free the structures and close the file.
 ********************************************************************** */
void server_shutdown (void)
{
    fprintf(stderr, "Shutting down...\n");
    http_close();
    do_close(&fs_file);
}

/**********************************************************************
 * Sends error message.
 ********************************************************************** */
static int reply_error_msg(int connection, int error)
{
#define ERR_MSG_SIZE 256
    char err_msg[ERR_MSG_SIZE]; // enough for any reasonable err_msg
    if (snprintf(err_msg, ERR_MSG_SIZE, "Error: %s\n", ERR_MSG(error)) < 0) {
        fprintf(stderr, "reply_error_msg(): sprintf() failed...\n");
        return ERR_RUNTIME;
    }
    return http_reply(connection, "500 Internal Server Error", "",
                      err_msg, strlen(err_msg));
}

/**********************************************************************
 * Sends 302 OK message.
 ********************************************************************** */
static int reply_302_msg(int connection)
{
    char location[ERR_MSG_SIZE];
    if (snprintf(location, ERR_MSG_SIZE, "Location: http://localhost:%d/" BASE_FILE HTTP_LINE_DELIM,
                 server_port) < 0) {
        fprintf(stderr, "reply_302_msg(): sprintf() failed...\n");
        return ERR_RUNTIME;
    }
    return http_reply(connection, "302 Found", location, "", 0);
}

/**********************************************************************
 * Simple handling of http message.
 ********************************************************************** */
int handle_http_message(struct http_message* msg, int connection)
{
    M_REQUIRE_NON_NULL(msg);
    debug_printf("handle_http_message() on connection %d. URI: %.*s\n",
                 connection,
                 (int) msg->uri.len, msg->uri.val);

    if (http_match_verb(&msg->uri, "/") || http_match_uri(msg, "/index.html")) {
        return http_serve_file(connection, BASE_FILE);
    }

    if (http_match_uri(msg, URI_ROOT "/list")      ||
        (http_match_uri(msg, URI_ROOT "/insert")
         && http_match_verb(&msg->method, "POST")) ||
        http_match_uri(msg, URI_ROOT "/read")      ||
        http_match_uri(msg, URI_ROOT "/delete")) {
        
        if (http_match_uri(msg, URI_ROOT "/list")) {
            return handle_list_call(connection);
        } else if (http_match_uri(msg, URI_ROOT "/read")) {
            return handle_read_call(msg, connection);
        } else if (http_match_uri(msg, URI_ROOT "/insert")) {
            return handle_insert_call(msg, connection);
        } else if (http_match_uri(msg, URI_ROOT "/delete")) {
            return handle_delete_call(msg, connection);
        }
        return reply_302_msg(connection);
        }
    else
        return reply_error_msg(connection, ERR_INVALID_COMMAND);
}

/**********************************************************************
 * Handles the list call.
 ********************************************************************** */
static int handle_list_call(int connection) {
    char *json_output;
    const char *header = "Content-Type: application/json" HTTP_LINE_DELIM;
    
    int err = do_list(&fs_file, JSON, &json_output);
    if (err != ERR_NONE) {
        free(json_output);
        return reply_error_msg(connection, err);
    }

    err = http_reply(connection, HTTP_OK, header, json_output, strlen(json_output));
    free(json_output);
    return err;
}

/**********************************************************************
 * Handles the read call.
 ********************************************************************** */
static int handle_read_call(struct http_message* msg, int connection)
{
    // Get the resolution parameter
    char str_resolution[MAX_RESOLUTION];
    int get_resolution_error = http_get_var(&msg->uri, "res", str_resolution, MAX_RESOLUTION);
    if (get_resolution_error == 0) return reply_error_msg(connection, ERR_INVALID_ARGUMENT);
    else if (get_resolution_error < 0) return reply_error_msg(connection, get_resolution_error);
    
    int resolution = resolution_atoi(str_resolution);
    if (resolution == -1) return reply_error_msg(connection, ERR_RESOLUTIONS);

    // Get the identificator parameter
    char img_id[MAX_IMG_ID];
    int get_id_error = http_get_var(&msg->uri, "img_id", img_id, MAX_IMG_ID);
    if (get_id_error == 0) return reply_error_msg(connection, ERR_INVALID_ARGUMENT);
    else if (get_id_error < 0) return reply_error_msg(connection, get_id_error);

    // Display the image to read
    char *image_buffer = NULL;
    uint32_t image_size = 0;
    //pthread_mutex_lock(&pthread_mutex);
    int do_read_error = do_read(img_id, resolution, &image_buffer, &image_size, &fs_file);
    //pthread_mutex_unlock(&pthread_mutex);

    if (do_read_error != 0) {
        free(image_buffer);
        return reply_error_msg(connection, do_read_error);
    }

    // Prepare the HTTP response
    char headers[] = "Content-Type: image/jpeg" HTTP_LINE_DELIM;

    // Send the response
    int error = http_reply(connection, HTTP_OK, headers, image_buffer, image_size);
    free(image_buffer);
    return error;
}

/**********************************************************************
 * Handles the delete call.
 ********************************************************************** */
static int handle_delete_call(struct http_message *msg, int connection)
{
    // Get the identificator parameter
    char img_id[MAX_IMG_ID];
    int get_id_error = http_get_var(&msg->uri, "img_id", img_id, MAX_IMG_ID);
    if (get_id_error == 0) return reply_error_msg(connection, ERR_INVALID_ARGUMENT);
    else if (get_id_error < 0) return reply_error_msg(connection, get_id_error);

    //pthread_mutex_lock(&pthread_mutex);
    int do_delete_error = do_delete(img_id, &fs_file);
    //pthread_mutex_unlock(&pthread_mutex);

    if (do_delete_error != 0) return reply_error_msg(connection, do_delete_error);

    return reply_302_msg(connection);
}

/**********************************************************************
 * Handles the insert call.
 ********************************************************************** */
static int handle_insert_call(struct http_message *msg, int connection)
{
    size_t content_len = msg->body.len;
    if (content_len == 0 || msg->body.val == NULL)
    {
        return reply_error_msg(connection, ERR_INVALID_ARGUMENT);
    }
    
    // Get the image name parameter
    char img_name[MAX_IMGFS_NAME];
    int get_name_error = http_get_var(&msg->uri, "name", img_name, MAX_IMGFS_NAME);
    if (get_name_error == 0) return reply_error_msg(connection, ERR_INVALID_ARGUMENT);
    else if (get_name_error < 0) return reply_error_msg(connection, get_name_error);

    // Allocate memory on the heap for the image content
    char *img_content = malloc(content_len);
    if (img_content == NULL) return reply_error_msg(connection, ERR_OUT_OF_MEMORY);
    memcpy(img_content, msg->body.val, content_len);

    // Insert the image into the image file system
    //pthread_mutex_lock(&pthread_mutex);
    int do_insert_error = do_insert(img_content, content_len, img_name, &fs_file);
    //pthread_mutex_unlock(&pthread_mutex);

    free(img_content);
    if (do_insert_error != 0) return reply_error_msg(connection, do_insert_error);

    return reply_302_msg(connection);
}
