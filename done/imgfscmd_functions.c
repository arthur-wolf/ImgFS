/**
 * @file imgfscmd_functions.c
 * @brief imgFS command line interpreter for imgFS core commands.
 *
 * @author Mia Primorac
 */

#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// default values
static const uint32_t default_max_files = 128;
static const uint16_t default_thumb_res =  64;
static const uint16_t default_small_res = 256;

// max values
static const uint16_t MAX_THUMB_RES = 128;
static const uint16_t MAX_SMALL_RES = 512;

/**********************************************************************
 * Displays some explanations.
 ********************************************************************** */
int help(int useless _unused, char** useless_too _unused)
{
    puts("imgfscmd [COMMAND] [ARGUMENTS]");
    puts("  help: displays this help.");
    puts("  list <imgFS_filename>: list imgFS content.");
    puts("  create <imgFS_filename> [options]: create a new imgFS.");
    puts("      options are:");
    puts("          -max_files <MAX_FILES>: maximum number of files.");
    puts("                                  default value is 128");
    puts("                                  maximum value is 4294967295");
    puts("          -thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.");
    puts("                                  default value is 64x64");
    puts("                                  maximum value is 128x128");
    puts("          -small_res <X_RES> <Y_RES>: resolution for small images.");
    puts("                                  default value is 256x256");
    puts("                                  maximum value is 512x512");
    puts("  delete <imgFS_filename> <imgID>: delete image imgID from imgFS.");
    return 0;
}

/**********************************************************************
 * Opens imgFS file and calls do_list().
 ********************************************************************** */
int do_list_cmd(int argc, char** argv)
{
    M_REQUIRE_NON_NULL(argv);
    if (argc < 1) {  // No file name provided
        return ERR_INVALID_ARGUMENT;
    } else if (argc >= 2) {  // Too many arguments
        return ERR_INVALID_COMMAND;
    }

    const char* dbFilename = argv[0];
    struct imgfs_file imgfsFile;
    int result = do_open(dbFilename, "r", &imgfsFile);

    if (result != ERR_NONE) {
        perror("Error opening database file");
        return result;
    }

    result = do_list(&imgfsFile, STDOUT, NULL);
    if (result != ERR_NONE) {
        perror("Error listing database");
        // Not returning here, as we want to ensure the file is closed
    }

    do_close(&imgfsFile); // Ensure file is closed and resources are cleaned up

    return result;
}


/**********************************************************************
 * Prepares and calls do_create command.
********************************************************************** */
int do_create_cmd(int argc, char** argv)
{

    puts("Create");
    /* **********************************************************************
     * TODO WEEK 08: WRITE YOUR CODE HERE (and change the return if needed).
     * **********************************************************************
     */

    TO_BE_IMPLEMENTED();
    return NOT_IMPLEMENTED;
}

/**********************************************************************
 * Deletes an image from the imgFS.
 */
int do_delete_cmd(int argc, char** argv)
{
    /* **********************************************************************
     * TODO WEEK 08: WRITE YOUR CODE HERE (and change the return if needed).
     * **********************************************************************
     */

    TO_BE_IMPLEMENTED();
    return NOT_IMPLEMENTED;
}
