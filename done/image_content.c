#include "image_content.h"

/**
 * @brief Resize the image to the given resolution, if it does not already
 * exists, and updates the metadata on the disk.
 *
 * @param resolution
 * @param imgfs_file The main in-memory structure
 * @param index The index of the image in the metadata array
 * @return Some error code. 0 if no error.
 */
int lazily_resize(int resolution, struct imgfs_file* imgfs_file, size_t index) {

    return ERR_NONE;
}