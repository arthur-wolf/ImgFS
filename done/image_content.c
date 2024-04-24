#include "imgfs.h"
#include "error.h"
#include <vips/vips.h>

/**
 * @brief Resize the image to the given resolution, if it does not already
 * exists, and updates the metadata on the disk.
 *
 * @param resolution Internal code for resolution (THUMB_RES, SMALL_RES, ORIG_RES)
 * @param imgfs_file The main in-memory structure
 * @param index The index of the image in the metadata array
 * @return Some error code. 0 if no error (ERR_NONE).
 */
int lazily_resize(int resolution, struct imgfs_file* imgfs_file, size_t index) {
    // Validate the parameters
    if (!imgfs_file || (index >= imgfs_file->header.max_files)) { //todo : nb_files or max_files ?
        return ERR_INVALID_ARGUMENT;
    }
    if (resolution != THUMB_RES && resolution != SMALL_RES && resolution != ORIG_RES) {
        return ERR_RESOLUTIONS;
    }

    // No action needed if resizing to original resolution
    if (resolution == ORIG_RES) {
        return ERR_NONE;
    }

    // todo : need this ??
    // Check if the image is already resized
    if (imgfs_file->metadata[index].size[resolution] != 0) {
        return ERR_NONE;
    }

    return ERR_NONE;
}
