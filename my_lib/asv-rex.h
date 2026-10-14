/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef _REX_H_
#define _REX_H_

#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "asv-common.h"
#include "asv-arena.h"

// -------------------------------- BEGIN: work with ArenaMacthes -----------------------------
// Initialize the matches structure cleanly
static inline void rex_matches_init(ArenaMatches *ms, ChunkedArena *arena) {
    ms->arena = arena;
    ms->head = NULL;
    ms->current = NULL;
    ms->current_count = 0;
    ms->total_count = 0;
}

static inline void rex_matches_reset(ArenaMatches *ms) {
    // Rewind back to the first block, leaving the chain intact
    ms->current = ms->head; 
    ms->current_count = 0;
    ms->total_count = 0;
}

static inline void rex_matches_append(ArenaMatches *ms, StringView sv) {
    if (!ms->arena) return;

    if (ms->head == NULL) {
        MatchesBlock *new_block = (MatchesBlock *)arena_alloc(ms->arena, sizeof(MatchesBlock));
        if (!new_block) return;
        new_block->next = NULL;
        ms->head = new_block;
        ms->current = new_block;
        ms->current_count = 0;
    }
    else if (ms->current_count >= MATCHES_BLOCK_SIZE) {
        // Recycle the next block if it already exists from a prior line run
        if (ms->current->next != NULL) {
            ms->current = ms->current->next;
        } else {
            // New peak high-water mark; grow the chain by 1 block
            MatchesBlock *new_block = (MatchesBlock *)arena_alloc(ms->arena, sizeof(MatchesBlock));
            if (!new_block) return;
            new_block->next = NULL;
            ms->current->next = new_block;
            ms->current = new_block;
        }
        ms->current_count = 0;
    }

    ms->current->items[ms->current_count] = sv;
    ms->current_count++;
    ms->total_count++;
}

// 🎯 THE INDEX ACCESSOR: Allows you to fetch items like an array safely
static inline StringView rex_matches_get(ArenaMatches *ms, size_t index) {
    if (index >= ms->total_count) return (StringView){0};

    size_t block_offset = index / MATCHES_BLOCK_SIZE;
    size_t item_offset = index % MATCHES_BLOCK_SIZE;

    MatchesBlock *b = ms->head;
    for (size_t i = 0; i < block_offset && b != NULL; i++) {
        b = b->next;
    }

    if (b == NULL) return (StringView){0};
    return b->items[item_offset];
}

// Internal helper to get a pointer to the very last inserted StringView
static inline StringView* rex_matches_last(ArenaMatches *ms) {
    if (ms->current == NULL || ms->current_count == 0) return NULL;
    return &ms->current->items[ms->current_count - 1];
}

static inline void rex_match_begin(ArenaMatches *ms, const StringView *sv) {
    if (ms == NULL) return;
    StringView g;
    g.data = sv->data;
    g.len = 0; // Starts at size zero until match_end is triggered
    rex_matches_append(ms, g);
}

static inline void rex_match_end(ArenaMatches *ms, const StringView *sv) {
    if (ms == NULL) return;
    StringView *last_match = rex_matches_last(ms);
    if (last_match != NULL) {
        // Calculate the exact captured token character distance span cleanly
        last_match->len = sv->data - last_match->data;
    }
}
// ------------------------------ END: work with ArenaMacthes -----------------------------

// get character at cursor
static inline char rex_curr(StringView *sv) {
  if (sv->len == 0)
    return 0;
  return *sv->data;
}

// return the current character and step cursor to next
static inline char rex_curr_step_to_next(StringView *sv) {
  if (sv->len == 0)
    return 0;
  char x = *sv->data;
  sv->data += 1;
  sv->len -= 1;
  return x;
}

// look ahead of cursor and return one character.
static inline char rex_peek(StringView *sv) {
  if (sv->len == 0)
    return 0;
  return *(sv->data + 1);
}

// get current char at cursor and then move cursor back by one in SV
static inline char rex_curr_and_backtrack(StringView *sv) {
  if (sv->len == 0)
    return 0;
  char x = *sv->data;
  sv->data -= 1;
  sv->len += 1;
  return x;
}

static inline bool rex_check_for_symbol_and_advance(StringView *sv,
                                                    StringView *symbol) {
  if (sv->len == 0)
    return false;
  if (sv->len < symbol->len && symbol->len > 0)
    return false;
  if (memcmp(sv->data, symbol->data, symbol->len) == 0) {
    sv->data += symbol->len;
    sv->len -= symbol->len;
    return true;
  } else {
    return false;
  }
}


static inline char rex_next_chop(StringView *sv, char delim) {
  if (sv->len == 0)
    return 0;
  size_t i = 0;
  while (i < sv->len && sv->data[i] != delim) {
    i += 1;
  }
  char x = *sv->data;
  sv->len -= i;
  sv->data += i;
  return x;
}

static inline bool rex_slice_with_delim_into_references(StringView *sv, ArenaMatches *ms, char delim) {
  do {
    rex_match_begin(ms, sv);
    rex_next_chop(sv, delim);
    rex_match_end(ms, sv);
    if (rex_curr(sv) == delim)
      rex_curr_step_to_next(sv);
  } while (sv->len > 0);
  assert(sv->len == 0);
  return true;
}

static inline bool rex_consume_whitespace(StringView *sv) {
  char c = rex_curr_step_to_next(sv);
  if (c == 0)
    return false;
  while (c != 0 && isspace(c)) {
    // putchar(c);
    c = rex_curr_step_to_next(sv);
  }
  if (c != 0)
    rex_curr_and_backtrack(sv);
  return true;
}

static inline bool rex_compare_case_insensitive(ChunkedArena *sa, const char *prop, StringView *sv,
                                    uint16_t len) {
  if (sv->len < len)
    return false;
  // create an UPPER CASE version of sv.
  char *dup = arena_strndup(sa, sv->data, sv->len);
  for (size_t i = 0; i < sv->len; ++i)
    dup[i] = toupper(dup[i]);

  int res = memcmp(prop, dup, len);

  return res == 0;
}

#endif // _rex_h_
