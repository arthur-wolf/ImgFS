#include "imgfs.h"
#include "image_content.h"

#include <stdlib.h>
#include <string.h>

/**
 * @brief Reads the content of an image from a imgFS.
 *
 * @param img_id The ID of the image to be read.
 * @param resolution The desired resolution for the image read.
 * @param image_buffer Location of the location of the image content
 * @param image_size Location of the image size variable
 * @param imgfs_file The main in-memory data structure
 * @return Some error code. 0 if no error.
 */
int do_read(const char* img_id, int resolution, char** image_buffer,
            uint32_t* image_size, struct imgfs_file* imgfs_file)
{
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(image_buffer);
    M_REQUIRE_NON_NULL(image_size);
    M_REQUIRE_NON_NULL(imgfs_file);

    // Find the image with the right img_id
    int index = -1;
    for (int i = 0; i < imgfs_file->header.max_files; ++i) {
        if (strncmp(img_id, imgfs_file->metadata[i].img_id, MAX_IMG_ID) == 0) {
            index = i;
            break;
        }
    }

    // There is no image with the requested img_id
    if (index == -1) return ERR_IMAGE_NOT_FOUND;

    // Check if the image already exists in the requested resolution
    // If not, resize it to the resolution
    if (imgfs_file->metadata[index].size[resolution] == 0) {
        lazily_resize(resolution, imgfs_file, index);
    }

    // At this point, the position of the image in the file is known and so is its size
    size_t offset = imgfs_file->metadata[index].offset[resolution];
    size_t size = imgfs_file->metadata[index].size[resolution];

    // Allocate memory for the image content
    *image_buffer = calloc(1, size);
    if (*image_buffer == NULL) return ERR_OUT_OF_MEMORY;
    

    // Read the image content from the file
    // Move the file pointer to the right position
    if (fseek(imgfs_file->file, (long)offset, SEEK_SET) != 0) {
        free(*image_buffer);
        return ERR_IO;
    }

    // Read the image content into the buffer
    if (fread(*image_buffer, size, 1, imgfs_file->file) != 1) {
        free(*image_buffer);
        return ERR_IO;
    }

    // Update the output parameter
    *image_size = size;

    return ERR_NONE;
}
