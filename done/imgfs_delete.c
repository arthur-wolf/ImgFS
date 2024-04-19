#include "imgfs.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

int do_delete(const char* img_id, struct imgfs_file* imgfs_file) {
    // Check if the input pointers are NULL
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(imgfs_file);

    // Find the image in the metadata
    int found_index = -1;
    for (int i = 0; i < imgfs_file->header.max_files; i++) {
        if (strcmp(imgfs_file->metadata[i].img_id, img_id) == 0 && imgfs_file->metadata[i].is_valid != EMPTY) {
            found_index = i;
            break;
        }
    }

    // The image does not exist or is already deleted
    if (found_index == -1) {
        return ERR_IMAGE_NOT_FOUND;
    }

    // Invalidate the metadata entry
    imgfs_file->metadata[found_index].is_valid = EMPTY;

    // Write the metadata changes to disk
    if(fseek(imgfs_file->file, sizeof(struct imgfs_header) + sizeof(struct img_metadata) * found_index, SEEK_SET) != 0) {
        return ERR_IO;
    }
    if(fwrite(&imgfs_file->metadata[found_index], sizeof(struct img_metadata), 1, imgfs_file->file) != 1) {
        return ERR_IO;
    }

    // Update the header
    imgfs_file->header.nb_files--;
    imgfs_file->header.version++;

    // Write the updated header to disk
    if(fseek(imgfs_file->file, 0, SEEK_SET) != 0) {
        return ERR_IO;
    }

    if(fwrite(&imgfs_file->header, sizeof(struct imgfs_header), 1, imgfs_file->file) != 1) {
        return ERR_IO;
    }

    return ERR_NONE;
}