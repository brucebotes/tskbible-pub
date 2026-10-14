/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper to parse and print x-www-form-urlencoded key-value pairs
void parse_and_print_params(char *data) {
    if (data == NULL || strlen(data) == 0) return;

    char *pair = strtok(data, "&");
    while (pair != NULL) {
        char *eq = strchr(pair, '=');
        if (eq != NULL) {
            *eq = '\0'; // Split into key and value
            char *key = pair;
            char *val = eq + 1;
            printf("Key: <b>%s</b>, Value: <b>%s</b><br>", key, val);
        }
        pair = strtok(NULL, "&");
    }
}

int main(void) {
    // FastCGI accept loop
    while (FCGI_Accept() >= 0) {
        printf("Content-type: text/html\r\n\r\n");
        printf("<h2>FastCGI Request Processor</h2>");

        char *method = getenv("REQUEST_METHOD");

        if (method != NULL && strcmp(method, "POST") == 0) {
            printf("<h3>Processing POST Data</h3>");
            
            // Read Content-Length
            char *content_length_str = getenv("CONTENT_LENGTH");
            if (content_length_str != NULL) {
                int content_length = atoi(content_length_str);
                
                if (content_length > 0) {
                    char *post_data = malloc(content_length + 1);
                    if (post_data != NULL) {
                        // Read the urlencoded body from stdin
                        int read_bytes = fread(post_data, 1, content_length, stdin);
                        post_data[read_bytes] = '\0'; // Null-terminate
                        
                        parse_and_print_params(post_data);
                        free(post_data);
                    }
                }
            }
        } else {
            printf("<h3>Processing GET (Query String)</h3>");
            char *query_string = getenv("QUERY_STRING");
            if (query_string != NULL) {
                // We must duplicate the string since strtok modifies it
                char *qs_copy = strdup(query_string);
                parse_and_print_params(qs_copy);
                free(qs_copy);
            }
        }
    }
    return 0;
}
