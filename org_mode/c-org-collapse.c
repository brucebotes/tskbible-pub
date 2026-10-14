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

// process rows that start with '|'
static inline bool parse_pipe_table_row(StringView *sv, ArenaMatches *ms) {
  assert(sv && ms);

  if (!rex_slice_with_delim_into_references(sv, ms, '|'))
    return false;
  return true;
}

// Parse line in buffer - ending in '/0' or EOF
static inline bool parse_line(const char *buffer, ChunkedArena *scratch_arena) {
  assert( buffer != nullptr && scratch_arena != nullptr);
  StringView sv = (StringView){.data = buffer, .len = strlen(buffer) - 1};

  if (!rex_consume_whitespace(&sv))
    return false;
  char c = rex_curr(&sv);
  if (c == 0)
    return true;

  if (c == '|') {
    ArenaMatches ms;
    rex_matches_init(&ms, scratch_arena);

    if (!parse_pipe_table_row(&sv, &ms))
      return true;
    // process the Matches into meaningful '|' Row stats
    Trow r = analyse_table_row(&ms);
    if (r.found_r_column && !r.found_s_column) {
      return false;
    } else {
      return true;
    }
  }
  return true;
}

int main(int argc, char *argv[]) {
  FILE *input = stdin;
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
    if(parse_line(buffer, &scratch_arena))
      printf("%s", buffer);
    arena_reset(&scratch_arena); // reset and re-use the memory space
  }

  arena_free_all(&scratch_arena); // return memory to system

  if (input != stdin) {
    fclose(input);
  }
  return EXIT_SUCCESS;
}
