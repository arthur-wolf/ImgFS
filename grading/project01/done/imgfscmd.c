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

#include <vips/vips.h>
#include <string.h>

const command_mapping commands[] = {
    {"list", do_list_cmd},
    {"create", do_create_cmd},
    {"help", help},
    {"delete", do_delete_cmd}
};

const int commands_size = sizeof(commands) / sizeof(commands[0]); // grader: -2, does not need to be global

/*******************************************************************************
 * MAIN
 */
int main(int argc, char* argv[])
{
    VIPS_INIT(argv[0]); // grader : -1, VIPS_INIT could fail and initialization result is not checked
    // moving VIPS_INIT after input verification could also be beneficial (no malus)

    int ret = ERR_INVALID_COMMAND;

    if (argc < 2) { // We need at least 2 arguments : the program's name and the command's name
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        argc--; argv++; // Skip the program's name

        for (int i = 0; i < commands_size ; ++i) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                // Execute the function corresponding to the command

                argc--; argv++; // Call the function with the right arguments, as specified in the handout
                ret = commands[i].func(argc, argv);
                break; // Exit the loop once the command is executed since each command is unique
            }
        }
    }

    if (ret) { // if (ret != 0)
        fprintf(stderr, "ERROR: %s\n", ERR_MSG(ret));
        help(argc, argv);
    }

    vips_shutdown();

    return ret;
}
