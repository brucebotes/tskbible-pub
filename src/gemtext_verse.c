/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "bible.h"
#include <stdbool.h>
#include <asv-common.h>
#include <asv-slots.h>
#include <asv-rex.h>
#include <asv-sv.h>
#include <asv-sb.h>

extern bool get_many_references(StringView sv, ArenaMatches *many);
extern ChunkedArena scratch_arena;

void print_verse_with_emphasis_markup(BibleEntry e) {
  ArenaStringBuilder sb = arena_sb_init(&scratch_arena, 512);
  StringView verse = e.value;
  ArenaSVSlots svs = {};

  if( e.tsk_refs == nullptr) return;
  svs = *e.tsk_refs;
  // every tsk entry occupies 3 SVSlots
  assert(svs.total_count % 3 == 0);
  for (size_t i = 0; i < svs.total_count; i = i + 3) {
    StringView tsk_token = slots_get(&svs, i + 1);
    arena_sb_emphasis_first_occurance(&sb, &verse, tsk_token, SV("*"), SV("*"));
  }
  // append the remainder
  arena_sb_append_sv(&sb, (StringView){.data=verse.data, .len=verse.len});
  sv_print(e.key);putchar(' ');sv_print((StringView){.data=sb.buffer, .len=sb.length});putchar('\n');
}

// Simple way to print verse contents from SV parts
#define PRINT_VERSE_ENTRY(e)                                                   \
  sv_print((e).key);                                                           \
  putchar(' ');                                                                \
  sv_print((e).value);                                                         \
  putchar('\n')

void print_gemtext_verse(StringView v) {
  BibleEntry e = {};
  ArenaSVSlots svs = {};
  ChunkedArena *scratch = &scratch_arena;

  e = bible_get(&bible, v);
  // 1. Print the marked up verse
  if (e.tsk_refs == nullptr) {
    PRINT_VERSE_ENTRY(e);
    return;
  }
  print_verse_with_emphasis_markup(e);
  svs = *e.tsk_refs;
  // 2. Print the tsk_refeneced_word and all the verses reference by tsk
  // every tsk entry occupies 3 SVSlots - order, tsk_refernce_word and the references
  assert(svs.total_count % 3 == 0);
  for (size_t i = 0; i < svs.total_count; i=i+3) {
    printf("* *");
    StringView tsk_phrase = slots_get(&svs, i+1);    
    sv_print(tsk_phrase);
    printf("*: ");
    StringView tsk_refs = slots_get(&svs, i+2);
    sv_print( tsk_refs );
    putchar('\n');

    // now print the verses by expanding the tsk_references references
    with_lifetime(scratch) {
      ArenaMatches reffs = {};
      rex_matches_init(&reffs, scratch);
      if(get_many_references(tsk_refs, &reffs)) {
	for (size_t j = 0; j < reffs.total_count; ++j) {
	  BibleEntry be = bible_get(&bible, rex_matches_get(&reffs, j));
	  printf("* ");PRINT_VERSE_ENTRY(be);
	}
      }
    }
  }
}

void pretty_print_gemtext_references(StringView ref) {
  ChunkedArena *scratch = &scratch_arena;

  with_lifetime(scratch) {
    ArenaMatches reffs = {};
    rex_matches_init(&reffs, scratch);
    if(get_many_references(ref, &reffs)) {
      for (size_t j = 0; j < reffs.total_count; ++j) {
        with_lifetime(scratch) {
          print_gemtext_verse(rex_matches_get(&reffs, j));
        }
      }
    }
  }
}
  
