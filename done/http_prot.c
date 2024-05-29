#include "imgfs.h"
#include "http_prot.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Checks whether the `message` URI starts with the provided `target_uri`.
 *
 * Returns: 1 if it does, 0 if it does not.
 *
 */
int http_match_uri(const struct http_message *message, const char *target_uri)
{
    M_REQUIRE_NON_NULL(message);
    M_REQUIRE_NON_NULL(target_uri);

    // Check if the URI is long enough to contain the target URI
    size_t target_len = strlen(target_uri);
    if (message->uri.len < target_len) {
        return 0;
    }

    // Check if the URI starts with the target URI
    if (strncmp(message->uri.val, target_uri, target_len) != 0) {
        return 0;
    }

    return 1;
}

/**
 * @brief Compare method with verb and return 1 if they are equal, 0 otherwise
 */
int http_match_verb(const struct http_string* method, const char* verb)
{
    M_REQUIRE_NON_NULL(method);
    M_REQUIRE_NON_NULL(verb);

    if (method->len != strlen(verb)) {
        return 0;
    }

    // Check if the method value matches the verb up to method->len chars
    if (strncmp(method->val, verb, method->len) != 0) {
        return 0;
    }

    return 1;
}

/**
 * @brief Writes the value of parameter `name` from URL in message to buffer out.
 *
 * Return the length of the value.
 * 0 or negative return values indicate an error.
 */
int http_get_var(const struct http_string* url, const char* name, char* out, size_t out_len)
{
    M_REQUIRE_NON_NULL(url);
    M_REQUIRE_NON_NULL(name);
    M_REQUIRE_NON_NULL(out);

    // Create the parameter search string
    char param_name[256];  // Assuming the parameter name won't exceed 255 characters
    snprintf(param_name, sizeof(param_name), "%s=", name);

    // Find the start of the parameters in the URL
    const char* query_start = strchr(url->val, '?');
    if (query_start == NULL) {
        return 0;  // No parameters found in the URL
    }
    query_start++; // Move past the '?'

    // Find the parameter in the URL
    const char* param_start = strstr(query_start, param_name);
    while (param_start != NULL) {
        // Ensure the parameter is either at the start or follows '&'
        if (param_start == query_start || *(param_start - 1) == '&') {
            break;
        }
        param_start = strstr(param_start + 1, param_name);
    }

    if (param_start == NULL) {
        return 0;  // Parameter not found
    }

    param_start += strlen(param_name); // Move past the parameter name and '='

    // Find the end of the parameter value
    const char* param_end = strchr(param_start, '&');
    if (param_end == NULL) {
        param_end = url->val + url->len;  // End of the URL
    }

    // Calculate the length of the parameter value
    size_t value_len = (size_t)(param_end - param_start);
    if (value_len >= out_len) {
        return ERR_RUNTIME;  // Value is too long for the output buffer
    }

    // Copy the value into the output buffer and null-terminate it
    strncpy(out, param_start, value_len);
    out[value_len] = '\0';

    return (int)value_len;
}

/**
 * @brief Accepts a potentially partial TCP stream and parses an HTTP message.
 *
 * Assumes that all characters of stream that are not filled by reading are set to 0.
 *
 * Places the complete HTTP message in out.
 * Also writes the content of header "Content Length" to content_len upon parsing the header in the stream.
 * content_len can be used by the caller to allocate memory to receive the whole HTTP message.
 *
 * Returns:
 *  a negative int if there was an error
 *  0 if the message has not been received completely (partial treatment)
 *  1 if the message was fully received and parsed
 */
int http_parse_message(const char *stream, size_t bytes_received, struct http_message *out, int *content_len)
{
    M_REQUIRE_NON_NULL(stream);
    M_REQUIRE_NON_NULL(out);
    M_REQUIRE_NON_NULL(content_len);

    // Ensure we have received the complete headers
    const char *headers_end = strstr(stream, HTTP_HDR_END_DELIM);
    if (headers_end == NULL) {
        return 0; // Incomplete headers
    }

    // Initialize the output structure
    memset(out, 0, sizeof(struct http_message));

    // Parse the first line
    const char *line_start = stream;
    const char *line_end = strstr(line_start, HTTP_LINE_DELIM);
    if (line_end == NULL) return ERR_RUNTIME; // Invalid HTTP request line

    // Extract the method
    line_start = get_next_token(line_start, " ", &out->method);
    if (line_start == NULL) return ERR_RUNTIME; // Invalid method token

    // Extract the URI
    line_start = get_next_token(line_start, " ", &out->uri);
    if (line_start == NULL) return ERR_RUNTIME; // Invalid URI token

    // Skip the HTTP version (third token)
    line_start = get_next_token(line_start, HTTP_LINE_DELIM, NULL);
    if (line_start == NULL) return ERR_RUNTIME; // Invalid HTTP version token

    // Parse all the headers
    const char *headers_start = line_start;
    const char *body_start = http_parse_headers(headers_start, out);
    if (body_start == NULL) return ERR_RUNTIME; // Invalid headers

    // Get the "Content-Length" value
    *content_len = 0; // Default to no content
    for (size_t i = 0; i < out->num_headers; ++i) {
        if (strncmp(out->headers[i].key.val, "Content-Length", out->headers[i].key.len) == 0) {
            *content_len = atoi(out->headers[i].value.val);
            break;
        }
    }

    // If there is a body, ensure we have received all of it
    size_t header_len = (size_t)(headers_end + strlen(HTTP_HDR_END_DELIM) - stream);
    if (*content_len > 0) {
        if (bytes_received < header_len + (size_t)*content_len) {
            return 0; // Incomplete body
        }
        out->body.val = body_start;
        out->body.len = (size_t)*content_len;
    }

    return 1; // Fully received and parsed
}

/**
 * @brief Parses a line of an HTTP message.
 *
 * Output may be NULL, in which case the line is simply skipped and the message is not stored.
 *
 * Returns the position of the first character after the line.
 */
static_unless_test const char* get_next_token(const char* message, const char* delimiter, struct http_string* output)
{
    if (message == NULL || delimiter == NULL) {
        return NULL;  // Invalid input
    }

    const char* ptr = strstr(message, delimiter);

    if (output != NULL) {
        if (ptr == NULL) {
            output->val = message;
            output->len = strlen(message);
        } else {
            output->val = message;
            output->len = (size_t)(ptr - message);
        }
    }

    if (ptr == NULL) return NULL;  // Delimiter not found

    return ptr + strlen(delimiter);
}

/**
 * @brief Parses the headers of an HTTP message and fills the key-value pairs in the output.
 *
 * Returns the position right after the last header line.s
 */
static_unless_test const char* http_parse_headers(const char* header_start, struct http_message* output)
{
    if (header_start == NULL || output == NULL) {
        return NULL;  // Invalid input
    }

    const char* start = header_start;
    const char* end = strstr(header_start, HTTP_LINE_DELIM);

    while (end != NULL && strncmp(start, HTTP_LINE_DELIM, strlen(HTTP_LINE_DELIM)) != 0) {
        if (output->num_headers >= MAX_HEADERS) return NULL;  // Too many headers

        const char* next_token = get_next_token(start, HTTP_HDR_KV_DELIM, &output->headers[output->num_headers].key);
        if (next_token == NULL) return NULL;  // Invalid header key

        next_token = get_next_token(next_token, HTTP_LINE_DELIM, &output->headers[output->num_headers].value);
        if (next_token == NULL) return NULL;  // Invalid header value

        start = next_token;
        end = strstr(start, HTTP_LINE_DELIM);

        output->num_headers++;
    }

    // Return the position right after the last header line
    return start + strlen(HTTP_LINE_DELIM);
}
