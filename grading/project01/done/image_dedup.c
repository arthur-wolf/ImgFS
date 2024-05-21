#include "image_dedup.h"
#include <string.h> // for strncmp

/**
 * @brief Does image deduplication.
 *
 * @param imgfs_file The main in-memory structure
 * @param index The order number in the metadata array
 * @return Some error code. 0 if no error.
 */
int do_name_and_content_dedup(struct imgfs_file* imgfs_file, uint32_t index)
{
    M_REQUIRE_NON_NULL(imgfs_file);
    M_REQUIRE_NON_NULL(imgfs_file->metadata);

    // If index is too big or the image does not exist
    if ((index >= imgfs_file->header.max_files) || (imgfs_file->metadata[index].is_valid == EMPTY)) {
        return ERR_IMAGE_NOT_FOUND;
    }

    for (uint32_t i = 0; i < imgfs_file->header.max_files; ++i) {
        // If the image is not the same as the one we are looking at and it exists
        if (i != index && imgfs_file->metadata[i].is_valid == 1) { // grader: magic number -0.5
            // If the image has the same ID
            if (strncmp(imgfs_file->metadata[i].img_id, imgfs_file->metadata[index].img_id, MAX_IMG_ID) == 0) {
                return ERR_DUPLICATE_ID;
            } else if (memcmp(imgfs_file->metadata[i].SHA, imgfs_file->metadata[index].SHA, SHA256_DIGEST_LENGTH) == 0) { // grader: use of memcmp +1
                // If the image has the same SHA, copy the size and offset and invalidate the image
                memcpy(imgfs_file->metadata[index].size, imgfs_file->metadata[i].size, sizeof(uint32_t) * NB_RES); // grader: use of memcpy +1
                memcpy(imgfs_file->metadata[index].offset, imgfs_file->metadata[i].offset, sizeof(uint64_t) * NB_RES);
                imgfs_file->metadata[index].is_valid = 0;

                return ERR_NONE;
            }
        }
    }

    // If no duplicates were found, set the offset to 0
    imgfs_file->metadata[index].offset[ORIG_RES] = 0;

    return ERR_NONE;
}
