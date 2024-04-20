#include "imgfs.h"

#include <stdlib.h>
#include <string.h>

/**
 * @brief Creates the imgFS called imgfs_filename. Writes the header and the
 *        preallocated empty metadata array to imgFS file.
 *
 * @param imgfs_filename Path to the imgFS file
 * @param imgfs_file In memory structure with header and metadata.
 *
 * Note that the header of the imgfs_file contains ONLY max_files and resized_res
 */
int do_create(const char* imgfs_filename, struct imgfs_file* imgfs_file) {
    M_REQUIRE_NON_NULL(imgfs_filename);
    M_REQUIRE_NON_NULL(imgfs_file);

    // Open the file for writing, creates it if it does not exist
    imgfs_file->file = fopen(imgfs_filename, "wb");
    if (imgfs_file->file == NULL) {
        return ERR_IO;
    }
    // Assign the database name and other constants to the header
    // max_files and resized_res are already set
    strncpy(imgfs_file->header.name, CAT_TXT, MAX_IMGFS_NAME);
    imgfs_file->header.version = 0; // Initial version
    imgfs_file->header.nb_files = 0; // No files initially

    // Write the header to the file and close it if there is an error
    if (fwrite(&(imgfs_file->header), sizeof(struct imgfs_header), 1, imgfs_file->file) != 1) {
        do_close(imgfs_file);
        return ERR_IO;
    }

    // Initialize and write the metadata array
    imgfs_file->metadata = calloc(imgfs_file->header.max_files, sizeof(struct img_metadata));
    if (imgfs_file->metadata == NULL) {
        do_close(imgfs_file);
        return ERR_OUT_OF_MEMORY;
    }

    size_t max_files = imgfs_file->header.max_files;
    if(fwrite(imgfs_file->metadata, sizeof(struct img_metadata), max_files, imgfs_file->file) != max_files) {
        do_close(imgfs_file);
        return ERR_IO;
    }

    // Output the number of items written (max_files + 1 to account for the header)
    printf("%zu item(s) written\n", max_files + 1);

    return ERR_NONE;
}
