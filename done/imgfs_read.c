#include "imgfs.h"
#include "image_content.h"

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

    // Find the image in the index
    int index = -1;
    for (int i = 0; i < imgfs_file->header.max_files; ++i) {
        if (strncmp(img_id, imgfs_file->metadata[i].img_id, MAX_IMG_ID) == 0) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        return ERR_INVALID_IMGID;
    }

    // Check if the image already exists in the requested resolution
    // If not, resize it to the resolution
    if (imgfs_file->metadata[index].size[resolution] != resolution) {
        lazily_resize(resolution, imgfs_file, index);
    }

    // TODO : FINISH


    return ERR_NONE;
}
