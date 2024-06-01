#include "imgfs.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

#include <json-c/json.h>

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
    case JSON: {
        json_object* jobj = json_object_new_object();
        if (jobj == NULL) return ERR_RUNTIME;

        json_object* jarray = json_object_new_array();;
        if (jarray == NULL) {
            json_object_put(jobj);
            return ERR_RUNTIME;
        }

        for (uint32_t i = 0; i < imgfs_file->header.max_files; ++i) {
            // Add only valid metadata
            if (imgfs_file->metadata[i].is_valid == NON_EMPTY) {
                json_object* jstring = json_object_new_string(imgfs_file->metadata[i].img_id);
                if (jstring == NULL) {
                    json_object_put(jobj);  // Free the JSON object
                    json_object_put(jarray);  // Free the JSON array
                    return ERR_RUNTIME;
                }
                json_object_array_add(jarray, jstring);
            }
        }

        json_object_object_add(jobj, "Images", jarray);
        
        // Duplicate the JSON string to return
        const char* json_str = json_object_to_json_string(jobj);
        *json = strdup(json_str);  // Duplicate the JSON string

        // Free the JSON object (as well as the array and strings within it)
        json_object_put(jobj);
        
        if (*json == NULL) return ERR_RUNTIME;  // Check if strdup succeeded

        break;
    }
    default:
        return ERR_INVALID_ARGUMENT;
    }

    return ERR_NONE;
}
