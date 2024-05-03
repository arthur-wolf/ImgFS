#include "imgfs.h"

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
    //              Find a free position in the index
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //                      Image deduplication
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //                Writing the image to the disk
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //                  Update image database data
    //-----------------------------------------------------------------

    // TODO : FINISH

    return ERR_NONE;
}
        
