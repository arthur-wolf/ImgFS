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
 * Creates a new name for the image.
 **********************************************************************/
static void create_name(const char* img_id, int resolution, char** new_name)
{
    if (img_id == NULL) return;

    const char* resolution_suffix = NULL;

    // Determine the appropriate suffix based on the resolution
    switch (resolution) {
    case ORIG_RES:
        resolution_suffix = "_orig";
        break;
    case SMALL_RES:
        resolution_suffix = "_small";
        break;
    case THUMB_RES:
        resolution_suffix = "_thumb";
        break;
    default:
        // Unknown resolution
        return;
    }

    // Calculate the length of the new string (+4 for ".jpg" and +1 for the null terminator)
    size_t new_name_length = strlen(img_id) + strlen(resolution_suffix) + 5;

    // Allocate memory for the new string
    *new_name = (char*)calloc(1, new_name_length * sizeof(char));
    if (*new_name == NULL) return;

    // Construct the new filename
    sprintf(*new_name, "%s%s.jpg", img_id, resolution_suffix);
}

/**********************************************************************
 * Writes the image to the disk.
 **********************************************************************/
static int write_disk_image(const char *filename, const char *image_buffer, uint32_t image_size)
{
    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(image_buffer);

    // Open the file for writing in binary mode
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) return ERR_IO;

    // Write the image buffer to the file
    size_t written = fwrite(image_buffer, sizeof(char), image_size, fp);
    fclose(fp);  // Always close the file descriptor

    // Check if the write operation wrote the entire buffer
    if (written != image_size) {
        return ERR_IO;
    }

    return ERR_NONE;
}

/**********************************************************************
 * Reads the image from the disk.
 **********************************************************************/
static int read_disk_image(const char *path, char **image_buffer, uint32_t *image_size)
{
    M_REQUIRE_NON_NULL(path);
    M_REQUIRE_NON_NULL(image_buffer);
    M_REQUIRE_NON_NULL(image_size);

    // Open the file for reading in binary mode
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) return ERR_IO;

    // Get the size of the file
    fseek(fp, 0, SEEK_END);
    *image_size = (uint32_t)ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate memory for the image buffer
    *image_buffer = (char*)calloc(1, *image_size * sizeof(char));
    if (*image_buffer == NULL) {
        fclose(fp);
        return ERR_OUT_OF_MEMORY;
    }

    // Read the image from the file
    size_t read = fread(*image_buffer, sizeof(char), *image_size, fp);
    fclose(fp);  // Always close the file descriptor

    // Check if we read the entire file
    if (read != *image_size) {
        free(*image_buffer);
        return ERR_IO;
    }

    return ERR_NONE;
}

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
    puts("  read   <imgFS_filename> <imgID> [original|orig|thumbnail|thumb|small]:");
    puts("      read an image from the imgFS and save it to a file.");
    puts("      default resolution is \"original\".");
    puts("  insert <imgFS_filename> <imgID> <filename>: insert a new image in the imgFS.");
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

/**********************************************************************
 * Reads an image from the imgFS and saves it to a file.
 **********************************************************************/
int do_read_cmd(int argc, char **argv)
{
    M_REQUIRE_NON_NULL(argv);
    if (argc != 2 && argc != 3) return ERR_NOT_ENOUGH_ARGUMENTS;

    const char * const img_id = argv[1];

    const int resolution = (argc == 3) ? resolution_atoi(argv[2]) : ORIG_RES;
    if (resolution == -1) return ERR_RESOLUTIONS;

    struct imgfs_file myfile;
    zero_init_var(myfile);
    int error = do_open(argv[0], "rb+", &myfile);
    if (error != ERR_NONE) return error;

    char *image_buffer = NULL;
    uint32_t image_size = 0;
    error = do_read(img_id, resolution, &image_buffer, &image_size, &myfile);
    do_close(&myfile);
    if (error != ERR_NONE) {
        return error;
    }

    // Extracting to a separate image file.
    char* tmp_name = NULL;
    create_name(img_id, resolution, &tmp_name);
    if (tmp_name == NULL) return ERR_OUT_OF_MEMORY;
    error = write_disk_image(tmp_name, image_buffer, image_size);
    free(tmp_name);
    free(image_buffer);

    return error;
}

/**********************************************************************
 * Inserts an image into the imgFS.
 **********************************************************************/
int do_insert_cmd(int argc, char **argv)
{
    M_REQUIRE_NON_NULL(argv);
    if (argc != 3) return ERR_NOT_ENOUGH_ARGUMENTS;

    struct imgfs_file myfile;
    zero_init_var(myfile);
    int error = do_open(argv[0], "rb+", &myfile);
    if (error != ERR_NONE) return error;

    char *image_buffer = NULL;
    uint32_t image_size;

    // Reads image from the disk.
    error = read_disk_image(argv[2], &image_buffer, &image_size);
    if (error != ERR_NONE) {
        do_close(&myfile);
        return error;
    }

    error = do_insert(image_buffer, image_size, argv[1], &myfile);
    free(image_buffer);
    do_close(&myfile);
    return error;
}
