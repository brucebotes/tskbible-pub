/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef STRING_VIEW_H_
#define STRING_VIEW_H_

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "asv-common.h"
#include "asv-arena.h"

#define SV(lit) (StringView){ .data = (lit), .len = sizeof(lit) - 1 }
// Clean C23 constant
constexpr StringView SV_EMPTY = { .data = nullptr, .len = 0 };


static inline bool sv_equals(StringView a, StringView b) {
    if (a.len != b.len) return false;
    return memcmp(a.data, b.data, a.len) == 0;
}

static inline bool sv_equals_cstr(StringView sv, const char *cstr) {
    return sv_equals(sv, (StringView){.data = cstr, .len = strlen(cstr)});
}

// Check if a view starts with a specific prefix (great for flags like --)
static inline bool sv_starts_with(StringView sv, StringView prefix) {
    if (sv.len < prefix.len) return false;
    return memcmp(sv.data, prefix.data, prefix.len) == 0;
}

// 1. Advance the view past whitespace cleanly
static inline StringView sv_trim_left(StringView sv) {
    while (sv.len > 0 && (sv.data[0] == ' '  || sv.data[0] == '\t' || 
                          sv.data[0] == '\n' || sv.data[0] == '\r')) {
        sv.data++;
        sv.len--;
    }
    return sv;
}

// 2. Chop a string view by a delimiter character (e.g., split by space or '=')
// Returns the left side of the chop, and modifies 'sv' to be the remaining right side.
static inline StringView sv_chop_by_delim(StringView *sv, char delim) {
    size_t i = 0;
    while (i < sv->len && sv->data[i] != delim) {
        i++;
    }
    
    StringView result = { .data = sv->data, .len = i };
    
    if (i < sv->len) {
        sv->data += i + 1;
        sv->len  -= i + 1;
    } else {
        sv->data += i;
        sv->len  -= i;
    }
    
    return result;
}

// 3. Print a StringView using standard printf safely
// Because StringViews are not null-terminated ('\0'), you use the "%.*s" specifier.
static inline void sv_print(StringView sv) {
    printf("%.*s", (int)sv.len, sv.data);
}


// Trims spaces, tabs, and newlines from BOTH ends of a view
static inline StringView sv_trim(StringView sv) {
    // Left trim
    while (sv.len > 0 && (sv.data[0] == ' '  || sv.data[0] == '\t' || 
                          sv.data[0] == '\n' || sv.data[0] == '\r')) {
        sv.data++;
        sv.len--;
    }
    // Right trim
    while (sv.len > 0 && (sv.data[sv.len - 1] == ' '  || sv.data[sv.len - 1] == '\t' || 
                          sv.data[sv.len - 1] == '\n' || sv.data[sv.len - 1] == '\r')) {
        sv.len--;
    }
    return sv;
}

static inline bool sv_to_int(StringView sv, int *out_result) {
    if (sv.len == 0 || out_result == nullptr) return false;

    int result = 0;
    int sign = 1;
    size_t i = 0;

    // Handle negative numbers if your config needs them
    if (sv.data[0] == '-') {
        sign = -1;
        i++;
        if (sv.len == 1) return false; // Just a minus sign is invalid
    } else if (sv.data[0] == '+') {
        i++;
        if (sv.len == 1) return false;
    }

    for (; i < sv.len; i++) {
        char c = sv.data[i];
        if (c < '0' || c > '9') {
            return false; // Found a non-numeric character, invalid integer
        }
        result = (result * 10) + (c - '0');
    }

    *out_result = result * sign;
    return true;
}

static inline bool sv_to_bool(StringView sv, bool *out_result) {
    sv = sv_trim(sv); // Clean up any trailing space padding
    
    if (sv_equals_cstr(sv, "true") || sv_equals_cstr(sv, "yes") || sv_equals_cstr(sv, "1")) {
        *out_result = true;
        return true;
    }
    if (sv_equals_cstr(sv, "false") || sv_equals_cstr(sv, "no") || sv_equals_cstr(sv, "0")) {
        *out_result = false;
        return true;
    }
    return false; // String text didn't match a recognized boolean variant
}

// Slices out everything between start_sym and end_sym, updating the main config view
static inline StringView sv_chop_between_symbols(StringView *config, char start_sym, char end_sym) {
    *config = sv_trim(*config);
    if (config->len == 0 || config->data[0] != start_sym) {
        return (StringView){0}; // Invalid or missing starting symbol
    }
    
    config->data++; // Skip past the start symbol
    config->len--;
    
    size_t i = 0;
    while (i < config->len && config->data[i] != end_sym) {
        i++;
    }
    
    StringView inside = { .data = config->data, .len = i };
    
    if (i < config->len) {
        config->data += i + 1; // Advance past the end symbol
        config->len  -= i + 1;
    } else {
        config->data += i;
        config->len  -= i;
    }
    return inside;
}



// Find the first occurrence of a substring inside a StringView
// Returns an index offset, or sv.len if not found
static inline size_t sv_find(StringView sv, StringView target) {
    if (target.len == 0 || sv.len < target.len) return sv.len;
    
    for (size_t i = 0; i <= sv.len - target.len; i++) {
        if (memcmp(&sv.data[i], target.data, target.len) == 0) {
            return i;
        }
    }
    return sv.len;
}

// Slurps a precise number of bytes off the front of a view
static inline StringView sv_take(StringView *sv, size_t count) {
    if (count > sv->len) count = sv->len;
    StringView result = { .data = sv->data, .len = count };
    sv->data += count;
    sv->len  -= count;
    return result;
}


// Parses a key enclosed in brackets and hashes it AT THE SAME TIME
static inline HashedToken sv_chop_and_hash_key(StringView *config) {
    *config = sv_trim(*config);
    HashedToken result = {0};

    if (config->len == 0 || config->data[0] != '[') return result;

    config->data++; // Skip opening '['
    config->len--;

    size_t i = 0;
    uint32_t current_hash = 5381; // Initialize our DJB2 seed right here

    // Single pass: Look for the closing bracket AND compute the hash simultaneously
    while (i < config->len && config->data[i] != ']') {
        char c = config->data[i];
        
        // Accumulate DJB2 hash on the fly
        current_hash = ((current_hash << 5) + current_hash) + c;
        
        i++;
    }

    // Set up the resulting token window
    result.view.data = config->data;
    result.view.len = i;
    result.hash = current_hash; // The final hash value is ready instantly!

    // Advance the main config past the closing ']'
    if (i < config->len) {
        config->data += i + 1;
        config->len  -= i + 1;
    } else {
        config->data += i;
        config->len  -= i;
    }

    return result;
}

// Finds a multi-line Here-Doc value by scanning for a line that matches target_hash
// DJB2 - hash macthing
static inline StringView sv_find_heredoc_value(StringView *config, uint32_t target_hash, StringView fence_name) {
    StringView value_start = *config; // Remember where the data starts
    size_t accumulated_len = 0;

    while (config->len > 0) {
        // 1. Peek at the current position. Is it a blank line or padding? Skip it.
        *config = sv_trim_left(*config);
        
        // Save the start coordinates of this specific text line
        StringView current_line = *config; 
        
        // 2. Chop off exactly one line up to the newline character
        StringView line_content = sv_chop_by_delim(config, '\n');
        line_content = sv_trim(line_content); // Clean padding from the fence line

        // 3. Incrementally calculate the DJB2 hash of ONLY this line
        uint32_t line_hash = 5381;
        for (size_t i = 0; i < line_content.len; i++) {
            line_hash = ((line_hash << 5) + line_hash) + line_content.data[i];
        }

        // 4. Compare Integers! (Costs 1 CPU Clock Cycle)
        if (line_hash == target_hash) {
            // Safety Check: In rare cases of hash collisions, verify the bytes match
            if (sv_equals(line_content, fence_name)) {
                // SUCCESS! We found the closing fence line.
                // Return everything from value_start up to the beginning of this fence line
                return (StringView){ .data = value_start.data, .len = accumulated_len };
            }
        }

        // If it wasn't the fence, add this line's length (plus the skipped newline) to our value size
        accumulated_len += (current_line.data - config->data);
    }

    return (StringView){0}; // Error: Hit end of file without finding the closing fence
}

static inline StringView sv_arena_concat(ChunkedArena *arena, StringView a, StringView b) {
    StringView result = {0};
    result.len = a.len + b.len;
    
    // Allocate exactly the combined byte count out of the arena
    char *buffer = (char *)arena_alloc(arena, result.len);
    if (!buffer) return result;

    // Copy the pieces sequentially into their permanent home
    memcpy(buffer, a.data, a.len);
    memcpy(buffer + a.len, b.data, b.len);

    result.data = buffer;
    return result;
}

static inline StringView sv_arena_from_int(ChunkedArena *arena, int value) {
    // 32 bytes is safely more than enough space to hold any 64-bit integer text
    char temp_buf[32]; 
    int len = snprintf(temp_buf, sizeof(temp_buf), "%d", value);

    StringView result = {0};
    if (len < 0) return result;

    // Allocate only the precise characters used
    char *arena_mem = (char *)arena_alloc(arena, (size_t)len);
    if (arena_mem) {
        memcpy(arena_mem, temp_buf, (size_t)len);
        result.data = arena_mem;
        result.len = (size_t)len;
    }
    return result;
}

// Loads a complete file from disk into your persistent arena block in one shot
static inline StringView sv_arena_load_file(ChunkedArena *arena, const char *file_path) {
    StringView result = {0};

    // 1. Open the file file descriptor safely
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        printf("[Error] Failed to open template file: %s\n", file_path);
        return result;
    }

    // 2. Query the operating system for the exact file size metrics
    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) {
        printf("[Error] Failed to query stats for file: %s\n", file_path);
        close(fd);
        return result;
    }

    // Check for empty files to prevent invalid arena operations
    if (file_stat.st_size == 0) {
        close(fd);
        return result;
    }

    size_t file_size = (size_t)file_stat.st_size;

    // 3. Allocate a perfectly sized chunk from your stable arena block
    char *buffer = (char *)arena_alloc(arena, file_size);
    if (!buffer) {
        printf("[Error] Out of memory allocation when loading file: %s\n", file_path);
        close(fd);
        return result;
    }

    // 4. Slurp the entire file block into RAM in a single system execution pass
    ssize_t bytes_read = read(fd, buffer, file_size);
    close(fd); // Instantly release the system file descriptor handle

    if (bytes_read < 0 || (size_t)bytes_read != file_size) {
        printf("[Error] Partial read error while processing file: %s\n", file_path);
        return result;
    }

    // 5. Package the buffer safely inside your read-only StringView wrapper
    result.data = buffer;
    result.len = file_size;
    return result;
}



#endif
