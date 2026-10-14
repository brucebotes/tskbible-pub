/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <asv-common.h>
#include <asv-slots.h>
#include <asv-rex.h>
#include <asv-sv.h>
#include <asv-sb.h>
#include "bible.h"

extern bool get_many_references(StringView sv, ArenaMatches *many);
extern void html_printf(const char *format, ...);
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
    arena_sb_emphasis_first_occurance(&sb, &verse, tsk_token, SV("<em>"), SV("</em>"));
  }
  // append the remainder
  arena_sb_append_sv(&sb, (StringView){.data=verse.data, .len=verse.len});
   html_printf("<p class=\"_verse\">\n");
   html_printf("<span class=\"_verse_ref\">%.*s</span> %.*s", (int)e.key.len, e.key.data, (int)sb.length, sb.buffer);
   html_printf("</p>\n");
}

void print_html_verse(StringView v) {
  BibleEntry e = {};
  ArenaSVSlots svs = {};
  ChunkedArena *scratch = &scratch_arena;

  e = bible_get(&bible, v);
  // 1. Print the marked up verse
  if (e.tsk_refs == nullptr) {
    html_printf("<p class=\"_verse\">\n");
    html_printf("<span class=\"_verse_ref\">%.*s</span> %.*s", (int)(e).key.len,
                (e).key.data, (int)(e).value.len, (e).value.data);
    html_printf("</p>\n");
    return;
  }
  print_verse_with_emphasis_markup(e);
  svs = *e.tsk_refs;
  // 2. Print the tsk_refeneced_word and all the verses reference by tsk
  // every tsk entry occupies 3 SVSlots - order, tsk_refernce_word and the
  // references
  assert(svs.total_count % 3 == 0);

  html_printf("<ul class=\"_ul _tsk_refs\">\n");
  for (size_t i = 0; i < svs.total_count; i = i + 3) {
    StringView tsk_phrase = slots_get(&svs, i + 1);
    StringView tsk_refs = slots_get(&svs, i + 2);
    html_printf("<li class=\"_li\">\n");
    html_printf("<p class=\"_tsk_refs_item\"><span "
                "class=\"_tsk_key\"><em>%.*s</em></span> %.*s</p>\n",
                (int)tsk_phrase.len, tsk_phrase.data, (int)tsk_refs.len,
                tsk_refs.data);
    html_printf("</li>\n");

    // now print the verses by expanding the tsk_references references
    with_lifetime(scratch) {
      ArenaMatches reffs = {};
      rex_matches_init(&reffs, scratch);
      if (get_many_references(tsk_refs, &reffs)) {
        html_printf("<ul class=\"_ul _tsk_refs_list\">\n");
        for (size_t j = 0; j < reffs.total_count; ++j) {
          BibleEntry be = bible_get(&bible, rex_matches_get(&reffs, j));
          html_printf("<li class=\"_li\">\n");
          html_printf("<p class=\"_verse\"><span "
                      "class=\"_verse_ref\">%.*s</span> %.*s</p>\n",
                      (int)be.key.len, be.key.data, (int)be.value.len,
                      be.value.data);
          html_printf("</li>\n");
        }
        html_printf("</ul>\n");
      }
    }
  }
  html_printf("</ul>\n");
}

void pretty_print_html_references(StringView ref) {
  ChunkedArena *scratch = &scratch_arena;

  with_lifetime(scratch) {
    ArenaMatches reffs = {};
    rex_matches_init(&reffs, scratch);
    if(get_many_references(ref, &reffs)) {
      for (size_t j = 0; j < reffs.total_count; ++j) {
        with_lifetime(scratch) {
          print_html_verse(rex_matches_get(&reffs, j));
        }
      }
    }
  }
}
