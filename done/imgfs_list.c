#include "imgfs.h"

#include <stdio.h>

//TODO CHECK CORRECTNESS
int do_list(const struct imgfs_file* imgfs_file,
            enum do_list_mode output_mode, char** json)
{

    M_REQUIRE_NON_NULL(imgfs_file);

    if (output_mode == STDOUT) {
        if(imgfs_file->header->nb_files <= 0) {
            printf("<< empty imgFS >>\n");
        } else {
            print_metadata(imgfs_file->metadata);
        }
    } else if (output_mode == JSON) {
        if (json == NULL) {
            return ERR_INVALID_ARGUMENT
        }
        TO_BE_IMPLEMENTED();
    } else {
        return ERR_INVALID_ARGUMENT;
    }

    return ERR_NONE;
}


/* int do_list(const struct imgfs_file* imgfs_file, enum do_list_mode output_mode, char** json) {
    if (imgfs_file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    if (output_mode == JSON) {
        if (json == NULL) {
            return ERR_INVALID_ARGUMENT;
        }
        *json = NULL;
    }

    if (imgfs_file->metadata == NULL) {
        return ERR_IMGFS_FULL;
    }

    if (output_mode == STDOUT) {
        printf("*****************************************\n ");
        printf("********** IMGFS METADATA START ********\n");
        for (uint32_t i = 0; i < imgfs_file->header.nb_files; ++i) {
            print_metadata(&imgfs_file->metadata[i]);
        }
printf("*********** IMGFS METADATA END *********\n");
        printf("*****************************************\n");
    } else if (output_mode == JSON) {
        toBeImplemented();
    }

    return ERR_NONE;

} */