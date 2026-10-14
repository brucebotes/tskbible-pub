/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <ctype.h>
#include <stdio.h>

#include <asv-arena.h>
#include <asv-common.h>
#include <asv-hash-map.h>
#include <asv-rex.h>
#include <asv-sb.h>
#include <asv-slots.h>
#include <asv-sv.h>
#define BIBLE_IMPLEMENTATION
//#include "bible.c"
#include "bible.h"
#include "color.h"
//#include "main.h"
//#include "tsk_common.h"

static int __chapter = 0;
static int __book = 0;
// enum mode_t __mode = __full;
// enum out_t __out = __text_out;
// enum mode_llm_t __llm_mode = __llm_basic;
bool __color = true;
extern ChunkedArena global_arena;
extern ChunkedArena scratch_arena;
extern ConfigMap app_config;


constexpr size_t HASHED_VERSE_SIZE = 16;
constexpr size_t SB_BLOCK_SIZE = 16;


// Forward defitions
// static inline void print_verse(const char *ref);
void pretty_print_tsk_verses(const char *ref);

// 1|In the begining[2][|]
// ------------------^        case 1 = digit
// ---------------------^     case 2 = '|'
static inline bool match_verse(StringView *sv, ArenaMatches *ms) {
  char c = rex_curr(sv);
  if (c == 0 || !isdigit(rex_curr(sv)))
    goto fail;
  rex_match_begin(ms, sv);
  for (c = rex_curr(sv); c != 0 && isdigit(c); c = rex_curr_step_to_next(sv)) {
  };
  rex_curr_and_backtrack(sv);
  rex_match_end(ms, sv);

  c = rex_curr_step_to_next(sv);
  if (c == 0 || c != '|') {
    rex_curr_and_backtrack(sv);
    goto fail;
  }

  rex_match_begin(ms, sv);
  for (c = rex_curr(sv); c != 0 && c != '|' && !isdigit(c);
       c = rex_curr_step_to_next(sv)) {
  };
  rex_curr_and_backtrack(sv);
  rex_match_end(ms, sv);

  return true;
fail:
  return false;
}

// case chapter
// ||1
// --^
// case book
// |||1
// ---^
static inline bool match_chapter_or_book(StringView *sv, ArenaMatches *ms) {
  size_t i;
  char c = rex_curr(sv);
  if (c != '|')
    goto fail;
  rex_match_begin(ms, sv);
  for (i = 0, c = rex_curr(sv); c != 0 && c == '|';
       ++i, c = rex_curr_step_to_next(sv)) {
  };
  rex_curr_and_backtrack(sv);
  switch (i - 1) {
  case 2:
    ++__chapter;
    break;
  case 3:
    ++__book;
    __chapter = 1;
    break;
  default: {
    goto fail;
  }
  }
  rex_match_end(ms, sv);
  return true;
fail:
  return false;
}

// read the header line from TSK
static inline bool read_tsk_header(StringView *sv) {
  for (char c = rex_curr(sv); c != 0 && c != '\n';
       c = rex_curr_step_to_next(sv)) {
  };
  return true;
}

// Parses a TSV line
// book|chapter|verse|sort|key_phrase|refs
// 1\t1\t1\t2\tbeginning\tpr 8:22-24;pr 16:4;mr 13:19;joh 1:1-3;heb 1:10;1jo
// 1:1\n
static inline bool match_tsk_entry(StringView *sv, ArenaMatches *ms) {
  char c = rex_curr(sv);
  if (c == 0 || !isdigit(rex_curr(sv)))
    goto fail;
  // Read 4 numbers
  for (int j = 0; j < 4; ++j) {
    rex_match_begin(ms, sv);
    for (c = rex_curr(sv); c != 0 && isdigit(c);
         c = rex_curr_step_to_next(sv)) {
    };
    rex_curr_and_backtrack(sv);
    rex_match_end(ms, sv);

    c = rex_curr_step_to_next(sv);
    if (c == 0 || c != '\t') {
      rex_curr_and_backtrack(sv);
      goto fail;
    }
  }

  // read key_phrase
  rex_match_begin(ms, sv);
  for (c = rex_curr(sv); c != 0 && c != '\t'; c = rex_curr_step_to_next(sv)) {
  };
  rex_curr_and_backtrack(sv);
  rex_match_end(ms, sv);
  c = rex_curr_step_to_next(sv);
  if (c == 0 || c != '\t') {
    rex_curr_and_backtrack(sv);
    goto fail;
  }
  //
  // read refs
  rex_match_begin(ms, sv);
  for (c = rex_curr(sv); c != 0 && c != '\n'; c = rex_curr_step_to_next(sv)) {
  };
  rex_curr_and_backtrack(sv);
  rex_match_end(ms, sv);
  c = rex_curr_step_to_next(sv);
  if (c == 0)
    return true; // end of file
  if (c != '\n') {
    rex_curr_and_backtrack(sv);
    goto fail;
  }

  return true;
fail:
  return false;
}

static inline bool match_bible_hash_reference(StringView *sv,
                                              ArenaMatches *ms) {
  char c = rex_curr(sv);
  if (c == 0 || !isdigit(rex_curr(sv)))
    goto fail;
  // Read 3 numbers
  for (int j = 0; j < 3; ++j) {
    rex_match_begin(ms, sv);
    for (c = rex_curr(sv); c != 0 && isdigit(c);
         c = rex_curr_step_to_next(sv)) {
    };
    rex_curr_and_backtrack(sv);
    rex_match_end(ms, sv);

    c = rex_curr_step_to_next(sv);
    if (c == 0 && j == 2) {
      return true;
    } else if (c != '|') {
      rex_curr_and_backtrack(sv);
      goto fail;
    }
  }
  return true;
fail:
  return false;
}

// --------------------- BEGIN: work with verse ranges -------------------------------

static inline bool append_verses_range(StringView *sv, ArenaMatches *ms,
                                       size_t book, size_t chapter) {
  StringView from = *rex_matches_last(ms); // keep copy of the previous verse number
  char c = rex_curr_step_to_next(sv);

  if (!isdigit(rex_curr(sv)))
    return false;

  //  find last verse number
  StringView to = *sv; // mark start for stepping
  for (c = rex_curr(sv); c != 0 && isdigit(c); c = rex_curr_step_to_next(sv)) {
  };
  rex_curr_and_backtrack(sv);
  to.len = sv->data - to.data;

  // convert previous verse and last verse numbers to integers
  // so that we can generate a range of numbers
  int f = 1;
  if (!sv_to_int(from, &f))
    return false;
  int t = 2;
  if (!sv_to_int(to, &t))
    return false;
  for (int i = f+1; i <= t; ++i) {
    // first duplicate the existing book and chapter
    rex_matches_append(ms, rex_matches_get(ms, book));
    rex_matches_append(ms, rex_matches_get(ms, chapter));
    // create a string in the scatch and from that a SV and then append that
    ArenaStringBuilder sb = arena_sb_init(&scratch_arena, 16);
    arena_sb_append_fmt(&sb, "%d", i);
    rex_matches_append(ms, (StringView){.data = sb.buffer, .len = sb.length});
  }
  return true;
}

// Syntax parsed
// ps 1:1
// ps 1:1,3
// ps 1:3-6
// and combination ps 1:1,3-6
static inline bool expand_verse_numbers(StringView *sv, ArenaMatches *ms) {
  char c = rex_curr(sv);
  if (c == 0 || !isalnum(rex_curr(sv)))
    goto fail;

  // upto 3 alpha numerics for book
  rex_match_begin(ms, sv);
  for (c = rex_curr(sv); c != 0 && isalnum(c); c = rex_curr_step_to_next(sv)) {
  };
  rex_curr_and_backtrack(sv);
  rex_match_end(ms, sv);
  size_t book = ms->total_count - 1; // save reference for later

  // followed by space
  c = rex_curr_step_to_next(sv);
  if (c == 0 || c != ' ')
    goto fail;

  // followed by chapter - 3 digits
  rex_match_begin(ms, sv);
  for (c = rex_curr(sv); c != 0 && isdigit(c); c = rex_curr_step_to_next(sv)) {
  };
  rex_curr_and_backtrack(sv);
  rex_match_end(ms, sv);
  size_t chapter = ms->total_count - 1; // save reference for later

  // followed by :
  c = rex_curr_step_to_next(sv);
  if (c == 0 || c != ':')
    goto fail;

  // followed by verse - 3 digits
  rex_match_begin(ms, sv);
  for (c = rex_curr(sv); c != 0 && isdigit(c); c = rex_curr_step_to_next(sv)) {
  };
  rex_curr_and_backtrack(sv);
  rex_match_end(ms, sv);

  if (c == 0 || c == '\n') {
    return true;
  } else if (c == ',') {
    while (c == ',' && c != 0) {
      c = rex_curr_step_to_next(sv);

      if (!isdigit(rex_curr(sv)))
        goto fail;
      // first duplicate the existing book and chapter
      rex_matches_append(ms, rex_matches_get(ms, book));
      rex_matches_append(ms, rex_matches_get(ms, chapter));

      rex_match_begin(ms, sv);
      for (c = rex_curr(sv); c != 0 && isdigit(c);
           c = rex_curr_step_to_next(sv)) {
      };
      rex_curr_and_backtrack(sv);
      rex_match_end(ms, sv);

      c = rex_curr(sv);
      // cater for '-' range character witin ',' loop
      if (c == '-') {
        if (!append_verses_range(sv, ms, book, chapter)) {
          goto fail;
        }
        c = rex_curr_step_to_next(sv);
      }
    }
    if (c != 0)
      goto fail;
    return true;

  } else if (c == '-') { // process verse ranges
    if (!append_verses_range(sv, ms, book, chapter)) {
      goto fail;
    } else {
      return true;
    }
  }
fail:
  return false;
}

// given a verse range ge 1:1-10, return ge 1:1, ge 1:2, ......
static inline bool expand_verse_range(StringView *verse, ArenaMatches *verses) {
  if (verse->len == 0) return false;
  
  ArenaMatches _vv = {};
  rex_matches_init(&_vv, &scratch_arena);
  // Each reference can expand into multiple verses
  if (!expand_verse_numbers(verse, &_vv))
    return false;
  assert(_vv.total_count % 3 == 0);
  for (size_t i = 0; i < _vv.total_count; i = i + 3) {
    ArenaStringBuilder sb = arena_sb_init(&scratch_arena, SB_BLOCK_SIZE);
    arena_sb_append_sv(&sb, rex_matches_get(&_vv, i)); // book
    arena_sb_append_sv(&sb, SV(" "));
    arena_sb_append_sv(&sb, rex_matches_get(&_vv, i + 1)); // chapter
    arena_sb_append_sv(&sb, SV(":"));
    arena_sb_append_sv(&sb, rex_matches_get(&_vv, i + 2)); // verse
    rex_matches_append(verses, (StringView){.data = sb.buffer, .len = sb.length});
  }

  return true;
}

// --------------------- END: work with verse ranges -------------------------------

// ------------------  BEGIN : parse Bible ------------------
// creates references to verse in a globally assesed hash table
void place_verse(StringView verse, StringView value) {
  // Create the SB in scratch_arena - short liftime
  ArenaStringBuilder sb = arena_sb_init(&scratch_arena, HASHED_VERSE_SIZE);
  arena_sb_append_fmt(&sb, "%s %d:", book[__book], __chapter);
  arena_sb_append_sv(&sb, verse);
  // Duplicate into global_arena for program lifetime
  char *key = arena_strndup(&global_arena, sb.buffer, sb.length);
  StringView sv_key = {.data = key, .len = sb.length};
  BibleEntry entry = {.key = sv_key, .value = value, .tsk_refs = nullptr};

  bool t = bible_insert(&bible, sv_key, entry);
  assert(t == true);
}

bool load_bible() {
  ChunkedArena *scratch = &scratch_arena;
  // used for walking the bible
  StringView sv = {};
  sv.data = (const char *)kjv_data;
  sv.len = kjv_data_len;
  sv.len -= 1; // subtract 1 to trim linefeed from the compiled kjv
#ifdef DUMP_LOAD_ASSETS
  int cnt = 0;
#endif
  // we will return the memory to scratch after use
  __chapter = 1;
  __book = 1;

  while (isdigit(rex_curr(&sv)) && sv.len) {
    with_lifetime(scratch) {
      ArenaMatches ms = {};
      rex_matches_init(&ms, scratch);
      if (!match_verse(&sv, &ms)) {
        fprintf(stderr, "Expected a verse: %c\n", rex_curr(&sv));
        return false;
      }

#ifdef DUMP_LOAD_ASSETS
      printf(" ");
      sv_print(rex_matches_get(&ms, 0));
      printf("|");
      sv_print(rex_matches_get(&ms, 1));
      printf("\n");
#endif
      place_verse(rex_matches_get(&ms, 0), rex_matches_get(&ms, 1));
      rex_matches_reset(&ms);

      if (rex_curr(&sv) == '|') {
        if (!match_chapter_or_book(&sv, &ms)) { // re-use same ArenaMatches
          fprintf(stderr, "Expected a book or chapter token: %c\n",
                  rex_curr(&sv));
          return false;
        }
        rex_matches_reset(&ms);
        assert(isdigit(rex_curr(&sv)));

#ifdef DUMP_LOAD_ASSETS
        printf("\nBook: %d chapter %d\n\n", __book, __chapter);
#endif
      }
#ifdef DUMP_LOAD_ASSETS
      ++cnt;
#endif
    } // lifetime
  }
#ifdef DUMP_LOAD_ASSETS
  printf("Number of verses in bible: %d\n", cnt);
  BibleEntry e = {};

  e = bible_get(&bible, SV("ge 1:1"));
  printf(" => ");
  sv_print(e.key);
  printf("|");
  sv_print(e.value);
  putchar('\n');
  e = bible_get(&bible, SV("joh 1:1"));
  printf(" => ");
  sv_print(e.key);
  printf("|");
  sv_print(e.value);
  putchar('\n');
  e = bible_get(&bible, SV("re 22:21"));
  printf(" => ");
  sv_print(e.key);
  printf("|");
  sv_print(e.value);
  putchar('\n');
#endif
  return true;
}

void place_tsk(ArenaMatches *ms) {
  assert(ms->total_count == 6);
  StringView _book = rex_matches_get(ms, 0);
  StringView chapter = rex_matches_get(ms, 1);
  StringView verse = rex_matches_get(ms, 2);
  StringView sort = rex_matches_get(ms, 3);
  StringView key_phrase = rex_matches_get(ms, 4);
  StringView refs = rex_matches_get(ms, 5);

  int temp = 0;
  bool t = sv_to_int(_book, &temp);
  assert(t == true);
  ArenaStringBuilder sb = arena_sb_init(&scratch_arena, HASHED_VERSE_SIZE);
  arena_sb_append_fmt(&sb, "%s %.*s:%.*s", book[temp], (int)chapter.len,
                      chapter.data, (int)verse.len, verse.data);

  StringView sv_key = {.data = sb.buffer, .len = sb.length};
  //  printf("   --> key: ");sv_print(sv_key);putchar('\n');

  BibleEntry b = bible_get(&bible, sv_key);
  assert(sv_equals(b.key, sv_key) == true);

  if (b.tsk_refs == nullptr) {
    ArenaSVSlots *svs = arena_alloc(&global_arena, sizeof(ArenaSVSlots));
    memset(svs, 0, sizeof(ArenaSVSlots));
    slots_init(svs, &global_arena);
    slots_append(svs, sort);
    slots_append(svs, key_phrase);
    slots_append(svs, refs);
    b.tsk_refs = svs;
    bool t = bible_insert(&bible, sv_key, b);
    assert(t == true);
  } else {
    slots_append(b.tsk_refs, sort);
    slots_append(b.tsk_refs, key_phrase);
    slots_append(b.tsk_refs, refs);
  }
}

bool load_tsk() {
  ChunkedArena *scratch = &scratch_arena;
  StringView sv = {};

#ifdef DUMP_LOAD_ASSETS
  int cnt = 0;
#endif

  sv.data = (const char *)tsk_data;
  sv.len = tsk_data_len;

  if (!read_tsk_header(&sv)) {
    fprintf(stderr, "Expected a TSK file header: %c\n", rex_curr(&sv));
    return false;
  }
  while (isdigit(rex_curr(&sv)) && sv.len) {
    with_lifetime(scratch) {
      ArenaMatches ms = {};
      rex_matches_init(&ms, scratch);
      if (!match_tsk_entry(&sv, &ms)) {
        fprintf(stderr, "Expected a TSK entry: %c\n", rex_curr(&sv));
        return false;
      }

#ifdef DUMP_LOAD_ASSETS
      // print_tsk(&ms);
#endif
      place_tsk(&ms);
      rex_matches_reset(&ms);
#ifdef DUMP_LOAD_ASSETS
      ++cnt;
#endif
    } // lifetime
  }
#ifdef DUMP_LOAD_ASSETS
  printf("Number of tsk references: %d\n", cnt);
  BibleEntry e = {};
  ArenaSVSlots svs = {};

  e = bible_get(&bible, SV("ge 1:1"));
  printf(" => ");
  sv_print(e.key);
  printf("|");
  sv_print(e.value);
  putchar('\n');
  svs = *e.tsk_refs;
  for (size_t i = 0; i < svs.total_count; ++i) {
    printf("     > ");
    sv_print(slots_get(&svs, i));
    putchar('\n');
  }
  e = bible_get(&bible, SV("joh 1:1"));
  printf(" => ");
  sv_print(e.key);
  printf("|");
  sv_print(e.value);
  putchar('\n');
  svs = *e.tsk_refs;
  for (size_t i = 0; i < svs.total_count; ++i) {
    printf("     > ");
    sv_print(slots_get(&svs, i));
    putchar('\n');
  }
  e = bible_get(&bible, SV("re 22:21"));
  printf(" => ");
  sv_print(e.key);
  printf("|");
  sv_print(e.value);
  putchar('\n');
  svs = *e.tsk_refs;
  for (size_t i = 0; i < svs.total_count; ++i) {
    printf("     > ");
    sv_print(slots_get(&svs, i));
    putchar('\n');
  }
#endif

  return true;
}

bool parse_bible_asset_files(void) {
  if (!load_bible())
    return false;
  if (!load_tsk())
    return false;
  return true;
}

// ------------------  END: parse Bible ------------------

bool get_many_references(StringView ref, ArenaMatches *many) {
  ChunkedArena *scratch = &scratch_arena;

  // with_lifetime(scratch) {
  ArenaMatches reffs = {};
  rex_matches_init(&reffs, scratch);

  if (!rex_slice_with_delim_into_references(&ref, &reffs, ';'))
    return false;
  for (size_t i = 0; i < reffs.total_count; ++i) {
    StringView sv = rex_matches_get(&reffs, i);
    //with_lifetime(scratch) { // we cannot rewind here because the 'ArenaMatches* many' may expand into
                               //this scracth area while we add more items
      ArenaMatches verses = {};
      rex_matches_init(&verses, scratch);
      if (!expand_verse_range(&sv, &verses))
        return false;
      for (size_t j = 0; j < verses.total_count; ++j) {
	rex_matches_append(many, rex_matches_get(&verses, j));
      }
      //}
  }
  return true;
}

void pretty_print_book_names(void) {
  for (size_t i = _ge; i < _book_num; ++i) {
    printf("%s %zu, ", (char *)BOOK_NAME[i], i);
  }
  printf("\n");
}

