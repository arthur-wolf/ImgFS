#include "image_dedup.h"

/**
 * @brief Does image deduplication.
 *
 * @param imgfs_file The main in-memory structure
 * @param index The order number in the metadata array
 * @return Some error code. 0 if no error.
 */
int do_name_and_content_dedup(struct imgfs_file* imgfs_file, uint32_t index) {
    M_REQUIRE_NON_NULL(imgfs_file);
    M_REQUIRE_NON_NULL(imgfs_file->metadata);

    if (index >= imgfs_file->header.max_files) {
        return ERR_INVALID_ARGUMENT;
    }

    if (imgfs_file->metadata[index].is_valid == 0) {
        return ERR_INVALID_ARGUMENT;
    }

    int8_t has_duplicate = 0;

    for (uint32_t i = 0; i < imgfs_file->header.nb_files; ++i) {
        if (i != index && imgfs_file->metadata[i].is_valid == 1) {
            if (strncmp(imgfs_file->metadata[i].img_id, imgfs_file->metadata[index].img_id,
                        sizeof (char[MAX_IMG_ID + 1])) == 0) {
                return ERR_DUPLICATE_ID;
            } else if (imgfs_file->metadata[i].SHA == imgfs_file->metadata[index].SHA) {
                memcpy(imgfs_file->metadata[index].size, imgfs_file->metadata[i].size, sizeof(uint32_t) * NB_RES);
                memcpy(imgfs_file->metadata[index].offset, imgfs_file->metadata[i].offset, sizeof(uint32_t) * NB_RES);
                imgfs_file->metadata[index].is_valid = 0;
                has_duplicate = 1;
                break;
            }
        }
    }

    if (!has_duplicate) {
        imgfs_file->metadata[index].orig_res[0] = 0;
        imgfs_file->metadata[index].orig_res[1] = 0;
    }

    return ERR_NONE;

    /*for (uint32_t i = 0; i < imgfs_file->header.nb_files; ++i) {
        if (i != index && imgfs_file->metadata[i].is_valid == 1)
            if (imgfs_file->metadata[i].size == imgfs_file->metadata[index].size) { TODO <-- might be important?
                if (strncmp(imgfs_file->metadata[i].img_id, imgfs_file->metadata[index].img_id,
                            sizeof (char[MAX_IMG_ID + 1])) == 0) {
                    return ERR_DUPLICATE_ID;
                }
                    imgfs_file->metadata[index].is_valid = 0;
                    return ERR_NONE;
            }
        }
    } */


    return ERR_NONE;
}
