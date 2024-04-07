/**
 * @file imgfscmd_functions.h
 * @brief interface function for imgFS command line interpreter.
 *
 * @author Ludovic Mermod
 */

#pragma once

/**********************************************************************
 * Displays some explanations.
 ********************************************************************** */
int help (int, char*[]);

/********************************************************************
 * Opens imgFS file and calls do_list().
 *******************************************************************/
int do_list_cmd(int argc, char* argv[]);

/********************************************************************
 * Prepares and calls do_create command.
 *******************************************************************/
int do_create_cmd(int argc, char* argv[]);

/********************************************************************
 * Deletes an image from the imgFS.
 *******************************************************************/
int do_delete_cmd(int argc, char* argv[]);

/********************************************************************
 * Inserts an image into the imgFS.
 *******************************************************************/
int do_insert_cmd(int argc, char* argv[]);

/********************************************************************
 * Reads an image from the imgFS.
 *******************************************************************/
int do_read_cmd(int argc, char* argv[]);


// Command function pointer
typedef int (*command)(int argc, char* argv[]);

// Command mapping
typedef struct {
    const char* name; // Command name
    command func; // Function pointer to the command
} command_mapping;

command_mapping commands[] = {
        {"list", do_list_cmd},
        {"create", do_create_cmd},
        {"help", help},
        {"delete", do_delete_cmd}
};
