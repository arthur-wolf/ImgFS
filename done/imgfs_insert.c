#include "imgfs.h"

#include <stdlib.h>

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
    return ERR_NONE;
}
        
