/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef STRING_BUILDER_H_
#define STRING_BUILDER_H_

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "asv-common.h"
#include "asv-arena.h"
#include "asv-sv.h"

// Initialize the builder by carving a warm staging buffer out of the arena
[[nodiscard]] 
static inline ArenaStringBuilder arena_sb_init(ChunkedArena *arena, size_t initial_capacity) {
    assert (arena != nullptr && "Arena is a null pionter!");
        
    ArenaStringBuilder sb = {};
    sb.arena = arena;
    sb.start_marker = arena_mark(arena); // Capture the entry baseline
    sb.capacity = initial_capacity;
    sb.buffer = (char *)arena_alloc(arena, initial_capacity);
    if (sb.buffer) {
        sb.buffer[0] = '\0';
    }
    return sb;
}

// Append a StringView to the builder channel
static inline void arena_sb_append_sv(ArenaStringBuilder *sb, StringView sv) {
    if (!sb->buffer || sv.len == 0) return;
    
    // If the string exceeds current capacity, grow it directly via the arena
    if (sb->length + sv.len >= sb->capacity) {
        size_t new_cap = sb->capacity * 2;
        if (new_cap < sb->length + sv.len + 1) {
            new_cap = sb->length + sv.len + 1;
        }
        
        char *new_buf = (char *)arena_alloc(sb->arena, new_cap);
        if (!new_buf) return;
        
        memcpy(new_buf, sb->buffer, sb->length);
        sb->buffer = new_buf;
        sb->capacity = new_cap;
    }
    
    memcpy(sb->buffer + sb->length, sv.data, sv.len);
    sb->length += sv.len;
    sb->buffer[sb->length] = '\0'; // Always keep it safely null-terminated
}

[[gnu::format(printf, 2, 3)]] 
static inline void arena_sb_append_fmt(ArenaStringBuilder *sb, const char *fmt, ...) {
    if (!sb->buffer) return;

    // 1. Loop and calculate space remaining inside the active builder cap
    size_t remaining = sb->capacity - sb->length;

    va_list args;
    va_start(args, fmt);
    
    // Test write to see how many bytes this format string actually demands
    va_list args_copy;
    va_copy(args_copy, args);
    int required = vsnprintf(sb->buffer + sb->length, remaining, fmt, args_copy);
    va_end(args_copy);

    if (required < 0) {
        va_end(args);
        return;
    }

    // 2. If it exceeds remaining space, grow the block via the ChunkedArena instantly
    if ((size_t)required >= remaining) {
        size_t new_cap = sb->capacity * 2 + required;
        char *new_buf = (char *)arena_alloc(sb->arena, new_cap);
        if (!new_buf) {
            va_end(args);
            return;
        }

        memcpy(new_buf, sb->buffer, sb->length);
        sb->buffer = new_buf;
        sb->capacity = new_cap;
        remaining = sb->capacity - sb->length;
        
        // Perform the formal write into the freshly carved arena tracks
        vsnprintf(sb->buffer + sb->length, remaining, fmt, args);
    }

    sb->length += required;
    sb->buffer[sb->length] = '\0'; // Lock in the trailing null terminator
    va_end(args);
}

// A very powerful function - used in splicer and uds-socket-pipe
// builds a string in a buffer and return the new string's contents
// as a StringView
static inline StringView arena_sb_replace_all(
    ArenaStringBuilder *sb, 
    StringView source, 
    StringView target, 
    StringView replacement
) {
    if (target.len == 0) return source;

    StringView current = source;
    
    while (true) {
        size_t index = sv_find(current, target);
        
        // If the target sequence isn't found, emit the remaining text block and break
        if (index == current.len) {
            arena_sb_append_sv(sb, current);
            break;
        }

        // 1. Append everything up to the match location coordinates
        StringView pre_match = sv_take(&current, index);
        arena_sb_append_sv(sb, pre_match);

        // 2. Append the replacement token piece
        arena_sb_append_sv(sb, replacement);

        // 3. Skip past the target match length to continue scanning the remainder string
        sv_take(&current, target.len);
    }

    return (StringView){ .data = sb->buffer, .len = sb->length };
}

#define ASB_NILL(sb)							\
  do {                                                                         \
    if ((sb)->length + 1 < (sb)->capacity)                                     \
      ((sb))->buffer[(sb)->length++] = '\0';                                   \
    else                                                                       \
      arena_sb_append_sv((sb), SV("\0")); 				       \
  } while (false)

#define ASB_NEWLINE(sb)							       \
  do {                                                                         \
    if ((sb)->length + 1 < (sb)->capacity)                                     \
      ((sb))->buffer[(sb)->length++] = '\n';                                   \
    else                                                                       \
      arena_sb_append_sv((sb), SV("\n"));				       \
  } while (false)

// --- 💎 THE AUTOMATED SCOPE MACRO ---
// Creates a local builder object variable, runs your block, and executes an automated rewind on exit!
#define with_builder(sb_var_name, arena_ptr, initial_cap) \
    for (struct { ArenaStringBuilder sb_var_name; int run; } _sb_guard = { arena_sb_init((arena_ptr), (initial_cap)), 1 }; \
         _sb_guard.run; \
         _sb_guard.run = 0, arena_rewind((arena_ptr), _sb_guard.sb_var_name.start_marker))

#endif
