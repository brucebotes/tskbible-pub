/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef SV_PARSERS_H_
#define SV_PARSERS_H_

#include <stdio.h>
#include "asv-common.h"
#include "asv-hash-map.h"

// Helper to push a parsed pair permanently into the arena and map
static inline void register_parameter(StringView key, StringView value) {
    // 1. Bake the key bytes into the permanent chunked arena
    StringView stable_key;
    stable_key.len = key.len;
    stable_key.data = (char *)arena_alloc(&global_arena, key.len);
    memcpy((void*)stable_key.data, key.data, key.len);

    // 2. Bake the value bytes into the permanent chunked arena
    StringView stable_value;
    stable_value.len = value.len;
    stable_value.data = (char *)arena_alloc(&global_arena, value.len);
    memcpy((void*)stable_value.data, value.data, value.len);
     
    // 3. Register into the map. If the key already exists, it is overwritten!
    map_insert(&app_config, stable_key, stable_value);
}

// parse command line options
static inline void parse_arguments(int argc, char **argv) {
    // Start at 1 to skip the program name
    for (int i = 1; i < argc; i++) {
        StringView arg = { .data = argv[i], .len = strlen(argv[i]) };

        if (sv_starts_with(arg, SV("--config"))) {
            // If the argument has an '=', we can chop it!
            // Example: --config=settings.txt
            StringView key = sv_chop_by_delim(&arg, '=');
            
            if (arg.len > 0) { // The remaining part after '=' is the value
	        printf("Config path via %.*s '=': %.*s\n", (int)key.len, key.data, (int)arg.len, arg.data);
            } else if (i + 1 < argc) { // Otherwise look at the next standalone argv slot
                StringView val = { .data = argv[++i], .len = strlen(argv[i]) };
                printf("Config path via next arg: %.*s\n", (int)val.len, val.data);
            }
        } 
        else if (sv_equals_cstr(arg, "--verbose") || sv_equals_cstr(arg, "-v")) {
            printf("Verbose mode activated!\n");
        }
    }
}

// parse a key/value pair config file
static inline void parse_config_buffer(const char *file_contents, size_t file_size) {
    StringView config = { .data = file_contents, .len = file_size };

    while (config.len > 0) {
        // 1. Grab the key wrapped in symbols (e.g., [port] or <port>)
        StringView key = sv_chop_between_symbols(&config, '[', ']');
        if (key.len == 0) break;

        // 2. Chop everything up to the '=' separator
        sv_chop_by_delim(&config, '=');

        // 3. Grab the value wrapped in symbols (e.g., "value" or {value})
        // This easily handles multiline values because the closing '}' can be lines away!
        StringView value = sv_chop_between_symbols(&config, '"', '"');

#ifdef DEBUG
        // Output results cleanly
        printf("KEY  : %.*s\n", (int)key.len, key.data);
        printf("VALUE: %.*s\n", (int)value.len, value.data);
        printf("------------------------------------\n");
#endif        
        config = sv_trim(config);
	register_parameter(key, value);	
    }
}

// parse config files with multi-line values
// values are marked as heredoc's - the heredoc
// symbol used  to mark the fence is '@'
static inline void parse_advanced_config(const char *file_contents, size_t file_size) {
    StringView config = { .data = file_contents, .len = file_size };

    while (config.len > 0) {
        config = sv_trim(config);
        if (config.len == 0) break;

        // 1. Slurp the Key: e.g. [default_html]
        StringView key = sv_chop_between_symbols(&config, '[', ']');
        if (key.len == 0) break;

        // 2. Consume the '=' separator
        sv_chop_by_delim(&config, '=');
        config = sv_trim(config);

        StringView value = {0};

        // 3. Determine Value Parsing Strategy
        if (config.len > 0 && config.data[0] == '"') {
            // Strategy A: Standard single-line or basic quoted block
            value = sv_chop_between_symbols(&config, '"', '"');
        } 
        else if (config.len > 0 && config.data[0] == '@') {
            // Strategy B: Dynamic Fenced Here-Doc Block!
            sv_take(&config, 1); // Skip the initial '@'
            
            // Read the fence tag up to the first newline (e.g., "HTML_END")
            StringView fence = sv_chop_by_delim(&config, '\n');
            fence = sv_trim(fence);
            
            // Look ahead to find where this exact fence string appears next
            size_t end_offset = sv_find(config, fence);
            
            if (end_offset < config.len) {
                // Slice out the value text sitting between the two fences
                value = sv_take(&config, end_offset);
                
                // Skip past the closing fence string itself
                sv_take(&config, fence.len);

		value = sv_trim(value);
		if (value.len > 0 && value.data[value.len - 1] == '@') {
		  value.len--; // Pop the leaking tail symbol safely
		}
            } else {
	      fprintf(stderr, "Error parsing key '%.*s': Missing closing fence '%.*s'\n", 
                       (int)key.len, key.data, (int)fence.len, fence.data);
                break;
            }
        } else {
          fprintf(
              stderr,
              "Error parsing value. Value not bounded by \" or heredoc @\n");
	  return;
        }
	
#ifdef DEBUG
        printf("KEY  : %.*s\n", (int)key.len, key.data);
        printf("VALUE:\n%.*s\n", (int)value.len, value.data);
        printf("====================================\n");
#endif
        // Clean up text padding on the extracted fields
        value = sv_trim(value);
        register_parameter(key, value);
    }
}

#endif
