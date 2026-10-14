/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */


/*
// map use example
void example(void) {
  StringView target_path = map_get(&app_config, SV("template_path"));
  if (target_path.len > 0) {
    // Render the nested file path contents directly!
  }
  // If [server_port] is "8080" in your config, port becomes 8080.
  // If the key is missing or corrupted, it cleanly falls back to 80.
  int port = MAP_GET_INT(&app_config, SV("server_port"), 80);

  // Combine strings safely with ZERO leak possibilities
  StringView folder = SV("templates/");
  StringView file   = SV("index.html");
  StringView full_path = sv_arena_concat(&global_arena, folder, file);

  // Format a number into the arena instantly
  StringView user_count_view = sv_arena_from_int(&global_arena, 142);
  map_insert(&app_config, SV("active_users"), user_count_view);

  // Run your template injector loop across an entire document view
  StringView html_template = SV("<div>Welcome back! Active connections: {{ active_users }}</div>");
  sv_render_template(html_template, &app_config);
}
*/

#ifndef _SV_HASH_MAP_H_
#define _SV_HASH_MAP_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "asv-common.h"
#include "asv-sv.h"
#include "asv-sb.h"

extern ChunkedArena global_arena;
extern ConfigMap app_config;

// 1. The DJB2 Hashing Function adapted for bounded StringViews
static inline uint32_t hash_string_view(StringView sv) {
    uint32_t hash = 5381;
    for (size_t i = 0; i < sv.len; i++) {
        hash = ((hash << 5) + hash) + sv.data[i]; // hash * 33 + c
    }
    return hash;
}

// 2. Insert a Key/Value pair into the Hash Map
static inline bool map_insert(ConfigMap *map, StringView key, StringView value) {
    if (map->count >= HASH_MAP_CAPACITY) return false; // Map is completely full

    uint32_t hash = hash_string_view(key);
    // Fast bitwise modulo operation (only works if CAPACITY is a power of 2)
    uint32_t index = hash & (HASH_MAP_CAPACITY - 1);

    while (map->buckets[index].occupied) {
        // If the key already exists, overwrite the value and return
        if (sv_equals(map->buckets[index].key, key)) {
            map->buckets[index].value = value;
            return true;
        }
        // Linear probing: collision occurred, try the next index slot
        index = (index + 1) & (HASH_MAP_CAPACITY - 1);
    }

    // Found an empty slot! Save the window structures cleanly
    map->buckets[index].key = key;
    map->buckets[index].value = value;
    map->buckets[index].occupied = true;
    map->count++;
    return true;
}

// 3. Lookup a key in O(1) constant time
static inline StringView map_get(ConfigMap *map, StringView key) {
    uint32_t hash = hash_string_view(key);
    uint32_t index = hash & (HASH_MAP_CAPACITY - 1);
    uint32_t start_index = index;

    while (map->buckets[index].occupied) {
        if (sv_equals(map->buckets[index].key, key)) {
            return map->buckets[index].value; // Success! Key found
        }
        index = (index + 1) & (HASH_MAP_CAPACITY - 1);
        
        // Prevent infinite loops if we circle entirely back to the start
        if (index == start_index) break; 
    }

    return (StringView){0}; // Return an empty view if key doesn't exist
}

static inline int map_get_int(ConfigMap *map_ptr, StringView sv_key, int default_val) {
  StringView val = map_get(map_ptr, sv_key);
    int result = default_val;
    if (val.len > 0) {
        sv_to_int(val, &result);
    }
    return result;
}

static inline void on_key_value_found(StringView parsed_key, StringView parsed_value) {
    // 1. Permanently copy the string bytes into the safe arena chunk
    StringView permanent_key;
    permanent_key.len = parsed_key.len;
    permanent_key.data = arena_alloc(&global_arena, parsed_key.len);
    memcpy((void*)permanent_key.data, parsed_key.data, parsed_key.len);

    StringView permanent_value;
    permanent_value.len = parsed_value.len;
    permanent_value.data = arena_alloc(&global_arena, parsed_value.len);
    memcpy((void*)permanent_value.data, parsed_value.data, parsed_value.len);

    // 2. Safely register them into the lookup table
    map_insert(&app_config, permanent_key, permanent_value);
}

//  An absolute winner - zero alloacations method using SV only
// to build strings
static inline void sv_render_template(StringView template_text, ConfigMap *map) {
    StringView stream = template_text;

    while (stream.len > 0) {
        // 1. Find the next opening mustache placeholder '{{'
        size_t open_bracket = sv_find(stream, SV("{{"));
        
        // Print everything up to the placeholder instantly
        StringView text_before = sv_take(&stream, open_bracket);
        printf("%.*s", (int)text_before.len, text_before.data);

        if (stream.len == 0) break; // Finished parsing the stream

        sv_take(&stream, 2); // Drop the opening "{{"

        // 2. Find the closing mustache placeholder '}}'
        size_t close_bracket = sv_find(stream, SV("}}"));
        if (close_bracket == stream.len) {
            printf("[Syntax Error] Unclosed template variable bracket!\n");
            break;
        }

        StringView key = sv_take(&stream, close_bracket);
        key = sv_trim(key); // Clean spaces inside: {{ user_name }} -> "user_name"
        
        sv_take(&stream, 2); // Drop the closing "}}"

        // 3. Constant-time lookup from your pre-hashed map!
        StringView variables_value = map_get(map, key);
        if (variables_value.len > 0) {
            printf("%.*s", (int)variables_value.len, variables_value.data);
        } else {
          // Fallback gracefully if the variable doesn't exist in config
          printf("[MISSING: %.*s]", (int)key.len, key.data);
        }
    }
    printf("\n");
}

static inline void arena_sb_render_template(ArenaStringBuilder *sb,
                                            StringView template_text,
                                            ConfigMap *map) {
  StringView stream = template_text;

  while (stream.len > 0) {
    // 1. Find the next opening mustache placeholder '{{'
    size_t open_bracket = sv_find(stream, SV("{{"));

    // Print everything up to the placeholder instantly
    StringView text_before = sv_take(&stream, open_bracket);
    arena_sb_append_fmt(sb, "%.*s", (int)text_before.len, text_before.data);

    if (stream.len == 0)
      break; // Finished parsing the stream

    sv_take(&stream, 2); // Drop the opening "{{"

    // 2. Find the closing mustache placeholder '}}'
    size_t close_bracket = sv_find(stream, SV("}}"));
    if (close_bracket == stream.len) {
      fprintf(stderr, "[Syntax Error] Unclosed template variable bracket!\n");
      break;
    }

    StringView key = sv_take(&stream, close_bracket);
    key = sv_trim(key); // Clean spaces inside: {{ user_name }} -> "user_name"

    sv_take(&stream, 2); // Drop the closing "}}"

    // 3. Constant-time lookup from your pre-hashed map!
    StringView variables_value = map_get(map, key);
    if (variables_value.len > 0) {
      arena_sb_append_fmt(sb, "%.*s", (int)variables_value.len,
                          variables_value.data);
    } else {
      // Fallback gracefully if the variable doesn't exist in config
      fprintf(stderr, "[MISSING: %.*s]", (int)key.len, key.data);
    }
  }
}


// Sweeps the table linearly to collect or process keys matching a specific prefix
static inline void map_find_by_prefix(ConfigMap *map, StringView prefix) {
    for (size_t i = 0; i < HASH_MAP_CAPACITY; i++) {
        // If the bucket is occupied, check if its safe arena-backed key starts with our prefix
        if (map->buckets[i].occupied && sv_starts_with(map->buckets[i].key, prefix)) {
            
            // Print or route the found matches directly!
            printf("Found Match -> Key: %.*s, Value: %.*s\n", 
                   (int)map->buckets[i].key.len, map->buckets[i].key.data,
                   (int)map->buckets[i].value.len, map->buckets[i].value.data);
                   
        }
    }
}

#endif

