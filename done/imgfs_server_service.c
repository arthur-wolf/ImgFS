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
    err = http_init(server_port, NULL);
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

