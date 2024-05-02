#include "imgfs.h"
#include "error.h"
#include <vips/vips.h>

int lazily_resize(int resolution, struct imgfs_file* imgfs_file, size_t index)
{
    void *buffer = NULL;
    VipsImage *original = NULL, *resized = NULL;
    void* resized_buffer = NULL;
    size_t resized_size = 0;

    int result = ERR_NONE;  // Default to no error

    M_REQUIRE_NON_NULL(imgfs_file);
    M_REQUIRE_NON_NULL(imgfs_file->file);
    M_REQUIRE_NON_NULL(imgfs_file->metadata);

    // --------------------------------------------------------------------------------------------
    //                                       PARAMETER CHECKS
    // --------------------------------------------------------------------------------------------

    // Check if the index is withind bounds and if it is valid
    if (index >= imgfs_file->header.max_files || imgfs_file->metadata[index].is_valid == 0) {
        return ERR_INVALID_IMGID;
    }

    // Check if the resolution is valid
    if (resolution != THUMB_RES && resolution != SMALL_RES && resolution != ORIG_RES) {
        return ERR_RESOLUTIONS;
    }

    // Check if the asked resolution is already available and it has not already been resized
    if (resolution == ORIG_RES || imgfs_file->metadata[index].size[resolution] != 0) {
        return ERR_NONE;  // No need to free anything yet
    }

    // --------------------------------------------------------------------------------------------
    //                                       RESIZE IMAGE
    // --------------------------------------------------------------------------------------------

    // Allocate memory for the original image
    buffer = malloc(imgfs_file->metadata[index].size[ORIG_RES]);
    if (!buffer) {
        result = ERR_OUT_OF_MEMORY;
        goto cleanup;
    }

    // Move to the beginning of the original image
    if (fseek(imgfs_file->file, (long)imgfs_file->metadata[index].offset[ORIG_RES], SEEK_SET) != 0) {
        result = ERR_IO;
        goto cleanup;
    }

    // Read the original image into the buffer
    if (fread(buffer, imgfs_file->metadata[index].size[ORIG_RES], 1, imgfs_file->file) != 1) {
        result = ERR_IO;
        goto cleanup;
    }

    // Load the original image into a VipsImage
    if (vips_jpegload_buffer(buffer, imgfs_file->metadata[index].size[ORIG_RES], &original, NULL) != 0) {
        result = ERR_IO;
        goto cleanup;
    }

    // Resize the image
    if (vips_thumbnail_image(original, &resized, imgfs_file->header.resized_res[2 * resolution], "height", imgfs_file->header.resized_res[2 * resolution + 1], NULL) != 0) {
        result = ERR_IO;
        goto cleanup;
    }

    // Save the resized image to a buffer
    if (vips_jpegsave_buffer(resized, &resized_buffer, &resized_size, NULL) != 0) {
        result = ERR_IO;
        goto cleanup;
    }

    // --------------------------------------------------------------------------------------------
    //                                       WRITE RESIZED IMAGE
    // --------------------------------------------------------------------------------------------

    // Move to the end of the file
    if (fseek(imgfs_file->file, 0, SEEK_END) != 0) {
        result = ERR_IO;
        goto cleanup;
    }

    // Write the resized image to the file
    if (fwrite(resized_buffer, resized_size, 1, imgfs_file->file) != 1) {
        result = ERR_IO;
        goto cleanup;
    }

    // Update the metadata
    imgfs_file->metadata[index].offset[resolution] = (size_t)ftell(imgfs_file->file) - resized_size;
    imgfs_file->metadata[index].size[resolution] = (uint32_t)resized_size;

    // Move back to the metadata
    if (fseek(imgfs_file->file, (long)(sizeof(struct imgfs_header) + index * sizeof(struct img_metadata)), SEEK_SET) != 0) {
        result = ERR_IO;
        goto cleanup;
    }

    // Write the updated metadata
    if (fwrite(&imgfs_file->metadata[index], sizeof(struct img_metadata), 1, imgfs_file->file) != 1) {
        result = ERR_IO;
        goto cleanup;
    }

    // --------------------------------------------------------------------------------------------
    //                                       CLEANUP
    // --------------------------------------------------------------------------------------------

    /** EXPLANATION OF THE USE OF GOTO STATEMENTS FOR CLEANUP :
    *   We have decided to use goto statements because it allows us to avoid repeating the same
    *   cleanup code multiple times in case of errors.
    *   This way, we can simply jump to the cleanup label, free the memory that has been allocated and
    *   return the error code.
    *   At the beginning, we have some parameters checks that return directly and do not need any cleanup,
    *   which is why they don't have a goto statement.
    */

cleanup:
    if (buffer) free(buffer);
    if (original) g_object_unref(VIPS_OBJECT(original));
    if (resized) g_object_unref(VIPS_OBJECT(resized));
    if (resized_buffer) free(resized_buffer);

    return result;
}

/**
* @brief Get the resolution of an image.
*
* @param height Pointer to the height of the image.
* @param width Pointer to the width of the image.
* @param image_buffer Buffer containing the image.
* @param image_size Size of the image buffer.
* @return Some error code. 0 if no error.
*/
int get_resolution(uint32_t *height, uint32_t *width,
                   const char *image_buffer, size_t image_size)
{
    M_REQUIRE_NON_NULL(height);
    M_REQUIRE_NON_NULL(width);
    M_REQUIRE_NON_NULL(image_buffer);

    VipsImage* original = NULL;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    const int err = vips_jpegload_buffer((void*) image_buffer, image_size,
                                         &original, NULL);
#pragma GCC diagnostic pop
    if (err != ERR_NONE) return ERR_IMGLIB;
    
    *height = (uint32_t) vips_image_get_height(original);
    *width  = (uint32_t) vips_image_get_width (original);
    
    g_object_unref(VIPS_OBJECT(original));
    return ERR_NONE;
}
