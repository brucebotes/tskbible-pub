/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include <stdlib.h>
#include <stdint.h>

// ---------------------------------
constexpr size_t CHUNK_SIZE = 4096;  // 4KB chunks are standard and cache-friendly
constexpr size_t SCRATCH_CHUNK_SIZE = 4096;
  
typedef struct ArenaChunk {
    struct ArenaChunk *next;
    size_t size;
    size_t used;
    uint8_t data[]; // Flexible array member for the raw storage
} ArenaChunk;

typedef struct {
    ArenaChunk *head;
    ArenaChunk *current;
} ChunkedArena;

typedef struct {
    ArenaChunk *active_chunk;
    size_t used_bytes;
} ArenaMarker;
// ----------------------------------
typedef struct {
    const char *data;
    size_t len;
} StringView;

typedef struct {
    StringView view;
    uint32_t hash;
} HashedToken;
// ---------------------------------
typedef struct {
    ChunkedArena *arena;
    char *buffer;
    size_t length;
    size_t capacity;
    ArenaMarker start_marker; // Remembers where this builder started
} ArenaStringBuilder;
// --------------------------------
constexpr size_t  HASH_MAP_CAPACITY = 1024; // Must be a power of 2 for fast wrapping

typedef struct {
    StringView key;
    StringView value;
    bool occupied; // Tracks if this hash bucket is taken
} ConfigEntry;

typedef struct {
    ConfigEntry buckets[HASH_MAP_CAPACITY];
    size_t count;
} ConfigMap;
// ------------------------------
struct StringItems {
  char **items;
  size_t count;
  size_t capacity;
};

typedef struct StringItems ManyReferences;
// -------------------------------------
constexpr size_t  MATCHES_BLOCK_SIZE  = 16; // Number of capture groups per block segment

typedef struct MatchesBlock {
    StringView items[MATCHES_BLOCK_SIZE];
    struct MatchesBlock *next;
} MatchesBlock;

typedef struct {
    ChunkedArena *arena;       // The master arena handling the allocations
    MatchesBlock *head;        // The first block of capture groups
    MatchesBlock *current;     // The active block we are filling up
    size_t current_count;      // How many slots are used in the active block
    size_t total_count;        // Global count across all blocks
} ArenaMatches;
// -------------------------------
constexpr size_t  SLOT_BLOCK_SIZE  = 16; // Number of SV slosts per block segment

typedef struct SVSlotBlock {
    StringView items[SLOT_BLOCK_SIZE];
    struct SVSlotBlock *next;
} SVSlotBlock;

typedef struct {
    ChunkedArena *arena;       // The master arena handling the allocations
    SVSlotBlock *head;        // The first block of SV slots
    SVSlotBlock *current;     // The active block we are filling up
    size_t current_count;      // How many slots are used in the active block
    size_t total_count;        // Global count across all blocks
} ArenaSVSlots;
// -------------------------------
#endif
