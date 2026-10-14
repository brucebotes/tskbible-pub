/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef CHUNKED_ARENA_H_
#define CHUNKED_ARENA_H_

#include "asv-common.h"
#include <assert.h>
#include <stdckdint.h>
#include <stdio.h>
#include <string.h>

// Create a new chunk
static inline ArenaChunk *arena_new_chunk(size_t capacity) {
  size_t bytes = capacity < CHUNK_SIZE ? CHUNK_SIZE : capacity;

  // Hardening: Protect the header structure + payload memory size from
  // overflowing
  size_t total_allocation_bytes = {};
  if (ckd_add(&total_allocation_bytes, sizeof(ArenaChunk), bytes)) {
    printf(
        "[Fatal Overflow] Requested chunk size breaks architecture limits.\n");
    return nullptr;
  }

  auto chunk = (ArenaChunk *)malloc(total_allocation_bytes);
  if (!chunk)
    return nullptr;

  chunk->next = nullptr;
  chunk->size = bytes;
  chunk->used = 0;
  return chunk;
}

// Allocate memory out of the arena
static inline void *arena_alloc(ChunkedArena *arena, size_t size) {
  if (arena == nullptr || size == 0)
    return nullptr;

  // Align allocations to 8 bytes for performance/CPU architecture requirements
  // C23 Hardening Check: Ensure bitwise alignment calculations don't overflow
  size_t aligned_size = {};
  if (ckd_add(&aligned_size, size, (size_t)7)) {
    return nullptr;
  }
  aligned_size &= ~7;

  if (!arena->current ||
      arena->current->used + aligned_size > arena->current->size) {
    // check if we already have a chunk - could have executed a rewind
    // and additional chunks have  been allocate before
    if (arena->current && arena->current->next != nullptr) {
      arena->current = arena->current->next;
    } else {
      auto new_chunk = arena_new_chunk(aligned_size);
      if (!new_chunk)
        return nullptr;

      if (!arena->head) {
        arena->head = new_chunk;
      } else {
        arena->current->next = new_chunk;
      }
      arena->current = new_chunk;
    }
  }

  // 🎯 THE CRITICAL HARDENING BOUNDS PASS
  // Instead of doing raw math: 'arena->current->used + aligned_size'
  // We force the CPU to validate that the new tracking boundary pointer is
  // completely safe.
  size_t next_used_offset = {};
  if (ckd_add(&next_used_offset, arena->current->used, aligned_size)) {
    printf("[Security Warning] Prevented an interior chunk allocation "
           "overflow.\n");
    return nullptr;
  }

  // Carve the memory out of the current active chunk using our validated safe
  // offset
  void *ptr = &arena->current->data[arena->current->used];
  arena->current->used = next_used_offset;
  return ptr;
}

// Free absolutely everything in O(1) time
static inline void arena_free_all(ChunkedArena *arena) {
  ArenaChunk *chunk = arena->head;
  while (chunk) {
    ArenaChunk *next = chunk->next;
    free(chunk); // Freeing the whole chunk shell + internal data at once
    chunk = next;
  }
  arena->head = nullptr;
  arena->current = nullptr;
}

// Reset the arena for reuse without freeing the underlying heap blocks
static inline void arena_reset(ChunkedArena *arena) {
  ArenaChunk *chunk = arena->head;
  while (chunk) {
    chunk->used = 0;
    chunk = chunk->next;
  }
  // Set the active allocation point back to the very first chunk
  arena->current = arena->head;
}

static inline void arena_stats(ChunkedArena *arena) {
  size_t ts = 0, tu = 0, ch = 0;
  ArenaChunk *c = arena->head;
  do {
    //printf("-->> size: %zu, used: %zu\n", c->size, c->used);
    ts += c->size;
    tu += c->used;
    ch += 1;
    c = c->next;
  } while (c != nullptr);
  printf("Tot: %zu, size: %zu, used: %zu\n", ch, ts, tu);
}

static inline char *arena_strdup(ChunkedArena *arena, const char *cstr) {
  size_t n = strlen(cstr);
  char *result = (char *)arena_alloc(arena, n + 1);
  assert(result != nullptr && "Increase SCRATCH_CAPACITY");
  memcpy(result, cstr, n);
  result[n] = '\0';
  return result;
}

static inline char *arena_strndup(ChunkedArena *arena, const char *s,
                                  size_t n) {
  char *r = (char *)arena_alloc(arena, n + 1);
  assert(r != nullptr && "Increase SCRATCH_CAPACITY");
  memcpy(r, s, n);
  r[n] = '\0';
  return r;
}

// Capture the exact current memory footprint of the arena
static inline ArenaMarker arena_mark(ChunkedArena *arena) {
  return (ArenaMarker){.active_chunk = arena->current,
                       .used_bytes = arena->current ? arena->current->used : 0};
}

// Reset the arena back to a captured marker (frees nothing to the OS, just
// moves the pointer back)
static inline void arena_rewind(ChunkedArena *arena, ArenaMarker marker) {

  arena->current = marker.active_chunk; // default initialisation could be NULL
                                        // which inferes a reset
  if (arena->current) {
    // check if marker is valid
    ArenaChunk *find = arena->head;
    while (find != marker.active_chunk && find != nullptr) {
      find = find->next;
    }
    assert(find != nullptr);
    // valid marker
    arena->current->used = marker.used_bytes;

    // Zero out any chunks that came after this one so they are clean for reuse
    ArenaChunk *next_chunk = arena->current->next;
    while (next_chunk) {
      next_chunk->used = 0;
      next_chunk = next_chunk->next;
    }
  } else {
    // If we rewound to the absolute beginning, reset the whole chain
    arena_reset(arena);
  }
}

// Macro: Creates a safe local lifetime block linked to a specific arena
#define with_lifetime(arena_ptr)                                               \
  for (                                                                        \
      struct {                                                                 \
        ArenaMarker marker;                                                    \
        int run;                                                               \
      } _guard = {arena_mark(arena_ptr), 1};                                   \
      _guard.run; _guard.run = 0, arena_rewind((arena_ptr), _guard.marker))

#endif
