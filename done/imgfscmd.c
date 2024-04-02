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

const int commands_size = sizeof(commands) / sizeof(commands[0]);

/*******************************************************************************
 * MAIN
 */
int main(int argc, char* argv[])
{
    int ret = 0;

    if (argc < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        argc--; argv++; // Skip the program's name

        int commandFound = 0; // Flag
        for (int i = 0; i < commands_size; ++i) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                // Execute the function corresponding to the command
                commandFound = 1; // Command is found
                ret = commands[i].func(argc, argv);
                break; // Exit the loop once the command is executed since each command is unique
            }
        }

        // If no command was matched, we were given an invalid command name
        if (!commandFound) {
            ret = ERR_INVALID_COMMAND; // Assuming ERR_INVALID_COMMAND is a defined error for unrecognized commands
        }
    }

    if (ret) {
        fprintf(stderr, "ERROR: %s\n", ERR_MSG(ret));
        help(argc, argv);
    }

    return ret;
}

