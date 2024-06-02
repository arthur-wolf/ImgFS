#include "imgfs.h"
#include "util.h"
#include "image_dedup.h"
#include "image_content.h"

#include <string.h>
#include <openssl/sha.h>

/**
 * @brief Insert image in the imgFS file
 *
 * @param buffer Pointer to the raw image content
 * @param size Image size
 * @param img_id Image ID
 * @return Some error code. 0 if no error.
 */
int do_insert(const char* image_buffer, size_t image_size,
              const char* img_id, struct imgfs_file* imgfs_file)
{
    M_REQUIRE_NON_NULL(image_buffer);
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(imgfs_file);

    if (imgfs_file->header.nb_files >= imgfs_file->header.max_files) {
        return ERR_IMGFS_FULL;
    }

    //-----------------------------------------------------------------
    // Check if there is an image with the same ID
    //-----------------------------------------------------------------
    for (uint32_t i = 0; i < imgfs_file->header.max_files; ++i) {
        if (imgfs_file->metadata[i].is_valid == NON_EMPTY &&
            strncmp(imgfs_file->metadata[i].img_id, img_id, MAX_IMG_ID) == 0) {
            return ERR_DUPLICATE_ID;
        }
    }

    //-----------------------------------------------------------------
    //              Find a free position in the index
    //-----------------------------------------------------------------
    size_t index = imgfs_file->header.max_files;
    for (uint32_t i = 0; i < imgfs_file->header.max_files; ++i) {
        // Find empty metadata
        if (imgfs_file->metadata[i].is_valid == EMPTY) {
            index = i;
            break;
        }
    }

    // No empty metadata found
    if (index == imgfs_file->header.max_files) return ERR_IMGFS_FULL;

    // -----------------------------------------------------------------
    //                     Update metadata fields
    // -----------------------------------------------------------------

    unsigned char sha_digest[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*) image_buffer, image_size, sha_digest);
    memcpy(imgfs_file->metadata[index].SHA, sha_digest, SHA256_DIGEST_LENGTH);

    strncpy(imgfs_file->metadata[index].img_id, img_id, MAX_IMG_ID);

    imgfs_file->metadata[index].size[ORIG_RES] = (uint32_t)image_size;

    uint32_t width, height;
    int resolution_error = get_resolution(&height, &width, image_buffer, image_size);
    if (resolution_error != ERR_NONE) return resolution_error;
    imgfs_file->metadata[index].orig_res[0] = width;
    imgfs_file->metadata[index].orig_res[1] = height;

    //-----------------------------------------------------------------
    //                      Image deduplication
    //-----------------------------------------------------------------

    int dedup_error = do_name_and_content_dedup(imgfs_file, (uint32_t)index);
    if (dedup_error != ERR_NONE) return dedup_error;

    //-----------------------------------------------------------------
    //                Writing the image to the disk
    //-----------------------------------------------------------------

    // Write the image to the disk if it does not exist yet
    if (imgfs_file->metadata[index].offset[ORIG_RES] == 0) {
        if (fseek(imgfs_file->file, 0, SEEK_END) != 0) return ERR_IO;
        long file_offset = ftell(imgfs_file->file);
        if (fwrite(image_buffer, image_size, 1, imgfs_file->file) != 1) return ERR_IO;

        imgfs_file->metadata[index].offset[ORIG_RES] = (uint64_t)file_offset;
        imgfs_file->metadata[index].size[ORIG_RES] = (uint32_t)image_size;

        imgfs_file->metadata[index].offset[THUMB_RES] = 0;
        imgfs_file->metadata[index].size[THUMB_RES] = 0;
        imgfs_file->metadata[index].offset[SMALL_RES] = 0;
        imgfs_file->metadata[index].size[SMALL_RES] = 0;
    }

    imgfs_file->metadata[index].is_valid = NON_EMPTY;

    //-----------------------------------------------------------------
    //                  Update image database data
    //-----------------------------------------------------------------

    imgfs_file->header.nb_files++;
    imgfs_file->header.version++;

    if (fseek(imgfs_file->file, 0, SEEK_SET) != 0) return ERR_IO;
    if (fwrite(&imgfs_file->header, sizeof(struct imgfs_header), 1, imgfs_file->file) != 1) return ERR_IO;

    if (fseek(imgfs_file->file, (long)(sizeof(struct imgfs_header) + index * sizeof(struct img_metadata)), SEEK_SET) != 0) return ERR_IO;
    if (fwrite(&imgfs_file->metadata[index], sizeof(struct img_metadata), 1, imgfs_file->file) != 1) return ERR_IO;

    return ERR_NONE;
}

