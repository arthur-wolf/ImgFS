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
int lazily_resize(int resolution, struct imgfs_file* imgfs_file, size_t index)
{
    // Validate the parameters
    M_REQUIRE_NON_NULL(imgfs_file);
    M_REQUIRE_NON_NULL(imgfs_file->file);
    M_REQUIRE_NON_NULL(imgfs_file->metadata);

    // ------------------------------------------------------------------------------------
    //                                parameters checks
    // ------------------------------------------------------------------------------------

    // Check if the index is not too big
    if (index >= imgfs_file->header.max_files) {
        return ERR_INVALID_IMGID;
    }

    // Check if the image exists
    if (imgfs_file->metadata[index].is_valid == 0) {
        return ERR_INVALID_IMGID;
    }

    // Check if the resolution is valid
    if (resolution != THUMB_RES && resolution != SMALL_RES && resolution != ORIG_RES) {
        return ERR_RESOLUTIONS;
    }

    // If the resolution is the original resolution, no need to resize
    if (resolution == ORIG_RES) {
        return ERR_NONE;
    }

    // If the image already exists at the given resolution, no need to resize
    if (imgfs_file->metadata[index].size[resolution] != 0) {
        return ERR_NONE;
    }

    // ------------------------------------------------------------------------------------
    //                                resizing
    // ------------------------------------------------------------------------------------

    // Allocate memory to store the image
    void * buffer = malloc(imgfs_file->metadata[index].size[ORIG_RES]);
    if (!buffer) {
        return ERR_OUT_OF_MEMORY;
    }

    // Find the offset of the image in the file
    if (fseek(imgfs_file->file, (long)imgfs_file->metadata[index].offset[ORIG_RES], SEEK_SET) != 0) {
        free(buffer);
        buffer = NULL;
        return ERR_IO;
    }

    // Read the image from the disk
    if (fread(buffer, imgfs_file->metadata[index].size[ORIG_RES], 1, imgfs_file->file) != 1) {
        free(buffer);
        buffer = NULL;
        return ERR_IO;
    }

    // Load the image from the buffer into a VipsImage
    VipsImage* original = NULL;
    if (vips_jpegload_buffer(buffer, imgfs_file->metadata[index].size[ORIG_RES], &original, NULL) != 0) {
        g_object_unref(VIPS_OBJECT(original));
        free(buffer);
        buffer = NULL;
        return ERR_IO;
    }

    // Resize the image
    VipsImage* resized = NULL;
    int width = imgfs_file->header.resized_res[2 * resolution];
    int height = imgfs_file->header.resized_res[2 * resolution + 1];

    if (vips_thumbnail_image(original, &resized, width, "height", height, NULL) != 0) {
        g_object_unref(VIPS_OBJECT(original));
        g_object_unref(VIPS_OBJECT(resized));
        free(buffer);
        buffer = NULL;
        return ERR_IO;
    }

    // Save resized image to buffer
    size_t resized_size = 0;
    void* resized_buffer = NULL;
    if (vips_jpegsave_buffer(resized, &resized_buffer, &resized_size, NULL) != 0) {
        g_object_unref(VIPS_OBJECT(original));
        g_object_unref(VIPS_OBJECT(resized));
        free(buffer);
        g_free(resized_buffer);
        return ERR_IO;
    }

    g_object_unref(VIPS_OBJECT(original));
    g_object_unref(VIPS_OBJECT(resized));

    // Move the file pointer to the end of the file
    if (fseek(imgfs_file->file, 0, SEEK_END) != 0) {
        free(buffer);
        g_free(resized_buffer);
        return ERR_IO;
    }

    // ------------------------------------------------------------------------------------
    //                                   ERRORS down here
    // ------------------------------------------------------------------------------------

    // Write the resized image to the disk
    size_t val = fwrite(resized_buffer, resized_size, 1, imgfs_file->file);
    if (val != 1) {
        free(buffer);
        free(resized_buffer);

        return ERR_IO;
    }

    // Update the metadata
    imgfs_file->metadata[index].offset[resolution] = ftell(imgfs_file->file) - resized_size;
    imgfs_file->metadata[index].size[resolution] = resized_size;

    // Move the file pointer to the metadata of the image
    if (fseek(imgfs_file->file, (long)(sizeof(struct imgfs_header) + index * sizeof(struct img_metadata)), SEEK_SET) != 0) {
        free(buffer);
        free(resized_buffer);
        return ERR_IO;
    }

    // Write the metadata to the disk
    if (fwrite(&imgfs_file->metadata[index], sizeof(struct img_metadata), 1, imgfs_file->file) != 1) {
        free(buffer);
        free(resized_buffer);
        return ERR_IO;
    }

    // Free resources
    free(buffer);
    free(resized_buffer);

    return ERR_NONE;
}
