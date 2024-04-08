/**
 * @file imgfscmd.c
 * @brief imgFS command line interpreter for imgFS core commands.
 *
 * Image Filesystem Command Line Tool
 *
 * @author Mia Primorac
 */

#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused

#include <stdlib.h>
#include <string.h>

command_mapping commands[] = {
    {"list", do_list_cmd},
    {"create", do_create_cmd},
    {"help", help},
    {"delete", do_delete_cmd}
};

int commands_size = sizeof(commands) / sizeof(commands[0]);

/*******************************************************************************
 * MAIN
 */
int main(int argc, char* argv[])
{
    int ret = 0;

    if (argc < 2) { // We need at least 2 arguments : the program's name and the command's name
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        argc--; argv++; // Skip the program's name

        int commandFound = 0;
        for (int i = 0; i < commands_size ; ++i) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                // Execute the function corresponding to the command
                commandFound = 1;
                argc--; argv++; // Call the function with the right arguments, as specified in the handout
                ret = commands[i].func(argc, argv);
                break; // Exit the loop once the command is executed since each command is unique
            }
        }

        // If no command was matched, we were given an invalid command name
        if (!commandFound) {
            ret = ERR_INVALID_COMMAND;
        }
    }

    if (ret) { // if (ret != 0)
        fprintf(stderr, "ERROR: %s\n", ERR_MSG(ret));
        help(argc, argv);
    }

    return ret;
}
