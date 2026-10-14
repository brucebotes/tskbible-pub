/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "bible.h"
#include "asv-common.h"
#include "asv-arena.h"
#include "asv-sv.h"
#include "asv-sb.h"
#include "asv-rex.h"

ChunkedArena global_arena;
ChunkedArena scratch_arena;

extern bool parse_bible_asset_files(void);
extern bool get_many_references(StringView ref, ArenaMatches *many);

struct trow {
  StringView reference;
  bool has_prefix;     // does the reference start with '\sim'
  bool has_postfix;    // does the reference end with '\sim'
  uint16_t r_column;   // where is the reference in this table row?
  bool found_r_column; // we have a reference somewhere
  StringView structure; // record at least one outline structure
  bool found_s_column; // does this row contain a column which defines the
                       // bullinger structure
  uint16_t s_column;   // where is this column
  uint16_t num_columns;
};
typedef struct trow Trow;

static inline bool build_reference(StringView *sv, StringView *ref) {
  char c;

  c = rex_curr(sv);
  if (isdigit(c)) {
    ref->data = sv->data; // mark start
    c = rex_curr_step_to_next(sv);
    while (c != 0 && (isdigit(c) || c == ',' || c == '-')) {
      ref->len++;
      c = rex_curr_step_to_next(sv);
    }
    rex_curr_and_backtrack(sv);
  } else {
    // error case --what do we do??
    return false;
  }
  return true;
}

// Take the List of SV representing regex styled capture groups
// and process these into statics for each '|' row parsed
Trow analyse_table_row(ArenaMatches *ms) {
  Trow ret = {0};

  ret.reference = (StringView){0};
  ret.num_columns = ms->total_count;

  for (size_t i = 0; i < ms->total_count; ++i) {
    StringView sv = rex_matches_get(ms, i);

    // exit if blank column;
    if (!rex_consume_whitespace(&sv))
      continue;

    char c = rex_curr(&sv);
    if (c == 0)
      continue; // exit

    // case 1: check for reference column
    if (isdigit((unsigned char) c) != 0 || c == '\\') {
      ret.r_column = i;
      ret.found_r_column = true;
      StringView sim = SV("\\sim");

      // check for leading '\sim'
      if (rex_check_for_symbol_and_advance(&sv, &sim)) {
        ret.has_prefix = true;
      }
      if (!build_reference(&sv, &ret.reference))
        continue;
      // check for trailing '\sim'
      if (rex_check_for_symbol_and_advance(&sv, &sim)) {
        ret.has_postfix = true;
      }
    } else {
      // case two - we check if the column has text (emacs org mode styled text)
      StringView copy = (StringView){.data = sv.data, .len = sv.len};
      while (c != 0) {
        // check for Alpha character in column -> s_column
        if (isalpha(c)) {
	  if (!ret.found_s_column){ // grab the closest column to the reference
	                            // column but not thereafter
            ret.s_column = i;
            ret.found_s_column = true;
	    ret.structure = copy;
	  }
	  break;
	}
	c = rex_curr_step_to_next(&sv);
      }
    }
  }

  return ret;
}


void print_org_mode_verse_ref(StringView ref, int pre_sim, int post_sim) {
  StringView verse = get_verse(ref);
  StringView oref = (StringView){.data = ref.data, .len = ref.len};
  rex_next_chop(&oref, ':');
  rex_curr_step_to_next(&oref); // move past ':'
  printf("%s%.*s%s. %.*s", pre_sim > 0 ? "\\sim" : "", (int)oref.len, oref.data,
         post_sim > 0 ? "\\sim" : "", (int)verse.len, verse.data);
}


// dump the Bible verse content that is referenced by book/chapter/tr->reference
void print_verses(const char *book, const char *chapter, Trow tr,
                  ChunkedArena *scratch_arena) {
  assert(scratch_arena != nullptr);
  
  ArenaMatches m = {0};
  rex_matches_init(&m, scratch_arena);
  ArenaStringBuilder sb = arena_sb_init(scratch_arena, 256);

  arena_sb_append_fmt(&sb, "%s %s:", book, chapter);
  arena_sb_append_sv(&sb, tr.reference);
  assert(sb.length > 0 && sb.buffer);

  StringView sv = {.data = sb.buffer, .len = sb.length};
  // WARNING: this function leaks memory 
  if (!get_many_references(sv, &m))
    return;

  for (size_t j = 0; j < m.total_count; ++j) {
    int column = 0;
    for (int i = 0; i < tr.r_column; ++i) {
      putchar('|');
      putchar(' ');
      ++column;
    }
    putchar('|');
    ++column;
    print_org_mode_verse_ref(rex_matches_get(&m, j), j == 0 && tr.has_prefix,
                      j == m.total_count - 1 && tr.has_postfix);
    for (int i = column; i < tr.num_columns; ++i) {
      putchar('|');
    }
    putchar('\n');
  }
}

// process rows that start with ':'
static inline bool parse_colon_property(StringView *sv, ArenaMatches *ms) {
  assert(sv && ms);

  if (!rex_slice_with_delim_into_references(sv, ms, ':'))
    return false;
  return true;
}

// process rows that start with '|'
static inline bool parse_pipe_table_row(StringView *sv, ArenaMatches *ms) {
  assert(sv && ms);

  if (!rex_slice_with_delim_into_references(sv, ms, '|'))
    return false;
  return true;
}

// place holders for the current active bible book and chapter
static char _book[10] = {0};
static char _chapter[10] = {0};

// Parse line in buffer - ending in '/0' or EOF
static inline bool parse_line(const char *buffer, ChunkedArena *scratch_arena) {
  assert( buffer != nullptr && scratch_arena != nullptr);
  StringView sv = (StringView){.data = buffer, .len = strlen(buffer) - 1};

  if (!rex_consume_whitespace(&sv))
    return false;
  char c = rex_curr(&sv);
  if (c == 0)
    return true;

  if (c == ':') {
    ArenaMatches ms; // Matches or like regex capture groups
    rex_matches_init(&ms, scratch_arena); 

    if (!rex_slice_with_delim_into_references(&sv, &ms, ':'))
      return false;

    // get the property and value from the Matches (Capture Group) 
    StringView property = sv_trim(rex_matches_get(&ms, 1));
    StringView value = sv_trim(rex_matches_get(&ms, 2));

    if (rex_compare_case_insensitive(scratch_arena, "BOOK", &property, 4)) {
      memcpy(_book, value.data, value.len + 1);
      _book[value.len] = '\0';
    } else if (rex_compare_case_insensitive(scratch_arena, "CHAPTER", &property, 7)) {
      memcpy(_chapter, value.data, value.len + 1);
      _chapter[value.len] = '\0';
    }
  } else if (c == '|') {
    ArenaMatches ms;
    rex_matches_init(&ms, scratch_arena);

    if (!parse_pipe_table_row(&sv, &ms))
      return false;

    // process the Matches into meaningful '|' Row stats
    Trow r = analyse_table_row(&ms);
    if (r.found_r_column)
      print_verses(_book, _chapter, r, scratch_arena);
  }
  return true;
}

int main(int argc, char *argv[]) {
  FILE *input = stdin;
  parse_bible_asset_files();

  if (argc > 1) {
    input = fopen(argv[1], "r");
    if (input == NULL) {
      perror("Error opening file");
      return EXIT_FAILURE;
    }
  }

  // Prepare a scratch memory space for our processing
  ChunkedArena scratch_arena = {0};

  char buffer[1024];
  while (fgets(buffer, sizeof(buffer), input) != NULL) {
    printf("%s", buffer);
    parse_line(buffer, &scratch_arena);
    arena_reset(&scratch_arena); // reset and re-use the memory space
  }

  arena_free_all(&scratch_arena); // return memory to system
  arena_free_all(&global_arena); // return memory to system

  if (input != stdin) {
    fclose(input);
  }
  return EXIT_SUCCESS;
}
