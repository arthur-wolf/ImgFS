#include "imgfs.h"

#include <stdio.h>
#include "util.h"

/**
 * @brief Displays (on stdout) imgFS metadata.
 *
 * @param imgfs_file In memory structure with header and metadata.
 * @param output_mode What style to use for displaying infos.
 * @param json A pointer to a string containing the list in JSON format if output_mode is JSON.
 *      It will be dynamically allocated by the function. Ignored for other output modes.
 * @return some error code.
 */
int do_list(const struct imgfs_file* imgfs_file, enum do_list_mode output_mode, char** json)
{
    M_REQUIRE_NON_NULL(imgfs_file);

    switch (output_mode) {
    case STDOUT:
        print_header(&imgfs_file->header);
        if(imgfs_file->header.nb_files <= 0) {
            printf("<< empty imgFS >>\n");
        } else {
            for (uint32_t i = 0; i < imgfs_file->header.max_files; ++i) {
                // Print only valid metadata
                if (imgfs_file->metadata[i].is_valid == NON_EMPTY) {
                    print_metadata(&imgfs_file->metadata[i]);
                }
            }
        }
        break;
    case JSON:
        TO_BE_IMPLEMENTED();
        break;
    default:
        break;
    }

    return ERR_NONE;
}
