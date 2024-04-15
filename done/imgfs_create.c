#include "imgfs.h"
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

    // Open or create the file, overwriting if it exists
    FILE *file = fopen(imgfs_filename, "wb");
    if (!file) {
        return ERR_IO;
    }

    // Assign the database name and other constants to the header
    strncpy(imgfs_file->header.name, CAT_TXT, MAX_IMGFS_NAME);
    imgfs_file->header.version = 0; // Initial version
    imgfs_file->header.nb_files = 0; // No files initially
    // max_files and resized_res are already set

    // Write the header to the file and close it if there is an error
    if (fwrite(&(imgfs_file->header), sizeof(struct imgfs_header), 1, file) != 1) {
        fclose(file);
        return ERR_IO;
    }

    // Initialize and write the metadata array
    int written_items = 1; // header is already written
    struct img_metadata empty_metadata = {0}; // Initialize the metadata to zero
    for (uint32_t i = 0; i < imgfs_file->header.max_files; i++) {
        if (fwrite(&empty_metadata, sizeof(struct img_metadata), 1, file) != 1) {
            fclose(file);
            return ERR_IO;
        }
        written_items++;
    }

    // Close the file
    fclose(file);

    // Output the number of items written
    printf("%d item(s) written\n", written_items);

    return ERR_NONE;
}
