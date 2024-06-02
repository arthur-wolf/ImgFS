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

#define N_COMMANDS 7

const command_mapping commands[N_COMMANDS] = {
    {"list", do_list_cmd},
    {"create", do_create_cmd},
    {"help", help},
    {"delete", do_delete_cmd},
    {"create", do_create_cmd},
    {"read", do_read_cmd},
    {"insert", do_insert_cmd}
};

/*******************************************************************************
 * MAIN
 */
int main(int argc, char* argv[])
{
    int ret = ERR_INVALID_COMMAND;

    if (argc < 2) { // We need at least 2 arguments : the program's name and the command's name
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        if (VIPS_INIT(argv[0]) != 0) vips_error_exit("unable to start VIPS");
        
        argc--; argv++; // Skip the program's name

        for (int i = 0; i < N_COMMANDS ; ++i) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                // Execute the function corresponding to the command

                ret = commands[i].func(argc - 1, argv + 1);
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
