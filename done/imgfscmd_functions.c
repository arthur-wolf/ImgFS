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

    return ERR_NONE;
}

/**********************************************************************
 * Opens imgFS file and calls do_list().
 ********************************************************************** */
// One argument : imgFS_filename
int do_list_cmd(int argc, char** argv)
{
    M_REQUIRE_NON_NULL(argv);
    if (argc < 1) {  // No file name provided
        return ERR_INVALID_ARGUMENT;
    } else if (argc > 1) {  // Too many arguments
        return ERR_INVALID_COMMAND;
    }

    const char* dbFilename = argv[0];
    M_REQUIRE_NON_NULL(dbFilename);

    struct imgfs_file imgfsFile;
    int result = do_open(dbFilename, "r", &imgfsFile);

    if (result != ERR_NONE) {
        return result;
    }

    result = do_list(&imgfsFile, STDOUT, NULL);

    do_close(&imgfsFile); // Ensure file is closed and resources are cleaned up

    return result;
}


/**********************************************************************
 * Prepares and calls do_create command.
********************************************************************** */
// At least one argument : imgFS_filename + options
int do_create_cmd(int argc, char** argv)
{
    // Validate arguments
    M_REQUIRE_NON_NULL(argv); // We need an imgFS filename
    if (argc < 1) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    // Default values
    const char* imgfs_filename = argv[0]; // The first argument is the imgFS filename (mandatory)
    uint32_t max_files = default_max_files;
    uint16_t thumb_res[2] = {default_thumb_res, default_thumb_res};
    uint16_t small_res[2] = {default_small_res, default_small_res};

    // Parse options
    // Start at 1 because the first argument cannot be repeated
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-max_files") == 0) {
            if (i + 1 >= argc) {    // If we don't have a value for the -max_files option
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }

            // Convert the value to an unsigned 32-bit integer
            max_files = atouint32(argv[i + 1]);
            if (max_files == 0) { // If the value is not a valid number
                return ERR_MAX_FILES;
            }
            ++i; // Skip the value of the -max_files option

        } else if (strcmp(argv[i], "-thumb_res") == 0) { // atouint32 conversion error
            if (i + 2 >= argc) {    // If we don't have two values for the -thumb_res option
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }

            thumb_res[0] = atouint16(argv[i + 1]);
            thumb_res[1] = atouint16(argv[i + 2]);

            if (thumb_res[0] == 0 || thumb_res[1] == 0) { // If at least one of the values is not a valid number
                return ERR_RESOLUTIONS;
            }

            if (thumb_res[0] > MAX_THUMB_RES || thumb_res[1] > MAX_THUMB_RES) { // Resolution too big
                return ERR_RESOLUTIONS;
            }

            i += 2; // Skip the values of the -thumb_res option

        } else if (strcmp(argv[i], "-small_res") == 0) {
            if (i + 2 >= argc) {    // If we don't have two values for the -small_res option
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }

            small_res[0] = atouint16(argv[i + 1]);
            small_res[1] = atouint16(argv[i + 2]);

            if (small_res[0] == 0 || small_res[1] == 0) { // atouint16 conversion error
                return ERR_RESOLUTIONS;
            }

            if (small_res[0] > MAX_SMALL_RES || small_res[1] > MAX_SMALL_RES) { // Resolution too big
                return ERR_RESOLUTIONS;
            }

            i += 2; // Skip the values of the -small_res option

        } else {
            return ERR_INVALID_ARGUMENT; // Undefined option
        }
    }

    // Create the imgFS with the given parameters
    struct imgfs_file imgfsFile = {
        .header = {
            .max_files = max_files,
            .resized_res = {thumb_res[0], thumb_res[1], small_res[0], small_res[1]}
        }
    };

    // Copy the imgFS filename to the header
    strncpy(imgfsFile.header.name, imgfs_filename, MAX_IMGFS_NAME);

    //
    int err = do_create(imgfs_filename, &imgfsFile);
    do_close(&imgfsFile);

    return err;
}

/**********************************************************************
 * Deletes an image from the imgFS.
 **********************************************************************/
// Two arguments: imgFS_filename + imgID
int do_delete_cmd(int argc, char** argv)
{
    // Validate arguments
    M_REQUIRE_NON_NULL(argv); // We need exactly an imgFS filename and an image ID
    if (argc < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    } else if (argc > 2) {
        return ERR_INVALID_ARGUMENT;
    }

    char* imgFS_filename = argv[0];     // The first argument is the imgFS filename
    char* imgID = argv[1];        // The second argument is the image ID
    // imgFS_filename and imgID must be non-NULL
    M_REQUIRE_NON_NULL(imgFS_filename);
    M_REQUIRE_NON_NULL(imgID);

    // Check if the image ID is within bounds
    if ((strlen(imgID) == 0) || (strlen(imgID) > MAX_IMG_ID)) {
        return ERR_INVALID_IMGID;
    }

    // Open the imgFS file
    struct imgfs_file imgfs_file;
    if (do_open(imgFS_filename, "rb+", &imgfs_file) != ERR_NONE) {
        return ERR_IO;
    }

    // Perform the delete operation
    int result = do_delete(imgID, &imgfs_file);

    // Cleanup
    do_close(&imgfs_file);

    return result;
}

