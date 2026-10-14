/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "color.h"
#include <stdio.h>
#include <stdarg.h>

#include <asv-common.h>
#include <asv-arena.h>
#include <asv-sv.h>
#include <asv-parsers.h>
#include <asv-slots.h>
#include <asv-rex.h>

#include "bible.h"
//#include "llm_config.h"

// Macro to check if a string consists only of digits
#define IS_ALL_DIGITS(str) ({				\
      bool all_digits = true;				\
      for (int i = 0; (str)[i] != '\0'; i++) {		\
        if (!isdigit((unsigned char)(str)[i])) {	\
	  all_digits = false;				\
	  break;					\
        }						\
      }							\
      all_digits;					\
    })

// Forward declararions
extern bool parse_bible_asset_files(void);
extern void pretty_print_book_names(void);
extern void pretty_print_references(StringView ref);
extern void pretty_print_verses(StringView ref);

ChunkedArena global_arena;
ChunkedArena scratch_arena;
ConfigMap app_config;

// Loops through argv[] looking for flags formatted as '--key=value'
void merge_command_line_flags(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    // Wrap the raw argv string into a temporary StringView window
    StringView arg = {.data = argv[i], .len = strlen(argv[i])};

    // We only care about formal arguments starting with our flag prefix '--'
    if (sv_starts_with(arg, SV("--"))) {
      sv_take(&arg, 2); // Drop the leading "--" prefix text

      // Chop by '=' to split the flag name from its data parameter
      // Example: "port=9090" -> key: "port", arg: "9090"
      StringView key = sv_chop_by_delim(&arg, '=');
      key = sv_trim(key);

      if (arg.len > 0) {
        // Scenario A: --port=9090
        StringView value = sv_trim(arg);
        register_parameter(key, value);
      } else if (i + 1 < argc) {
        // Scenario B: --port 9090 (value is in the next argv index)
        StringView value = {.data = argv[i + 1], .len = strlen(argv[i + 1])};
        value = sv_trim(value);

        // Skip checking if the next argument is another flag
        if (!sv_starts_with(value, SV("--"))) {
          register_parameter(key, value);
          i++; // Advance past the value argument index
        }
      }
    }
  }
}

bool read_load_and_merge_config(int argc, char *argv[argc + 1]){
    // 1. One-shot load and parse the disk configuration file contents
  StringView raw_file = sv_arena_load_file(&global_arena, "config.txt");
  if (raw_file.len > 0) {
    parse_advanced_config(raw_file.data, raw_file.len);
  } else {
    return false;
  }

  // 2. Run the argv parser. Any CLI match here overwrites the file
  // configuration keys!
  merge_command_line_flags(argc, argv);
  return true;
}

void print_usage(int argc, char *argv[]);

int main(int argc, char *argv[argc + 1]) {
  if (argc < 2) {
    print_usage(argc, argv);
  }

  // Generate a large pre-allocated scracth area and reset it for scratch
  // purposes
  arena_alloc(&global_arena, 1024*1024*9);
  arena_reset(&global_arena);
  arena_alloc(&scratch_arena, 1024*32);
  arena_reset(&scratch_arena);

  /*
  if (!read_load_and_merge_config(argc, argv)) {
    fprintf(stderr, "Failed to read the config file!");
  }
  */
  merge_command_line_flags(argc, argv);

  parse_bible_asset_files();
  
  StringView references = map_get(&app_config, SV("r"));
  if (references.len > 0)
    with_lifetime(&scratch_arena) { pretty_print_references(references); }

  StringView verses = map_get(&app_config, SV("v"));
  if (verses.len > 0)
    with_lifetime(&scratch_arena) { pretty_print_verses(verses); }

  StringView book_names = map_get(&app_config, SV("b"));
  if (book_names.len > 0) {
    bool print = false;
    if (sv_to_bool(book_names, &print) && print == true)
      pretty_print_book_names();
  }


#ifdef DEBUG
  printf("\nScratch ");arena_stats(&scratch_arena);
  printf("Global ");arena_stats(&global_arena);
#endif     
  arena_free_all(&global_arena);
  arena_free_all(&scratch_arena);
  return 0;
}

void print_usage(int argc, char *argv[]){
  printf("\nusage: %s --command=value\n", argv[0]);
  printf("\tcommand\tdescription\n");
  printf(
	 "\t--v\tVerses, ie:\n"
	 "\t\t\t\"joh 1:1\" or \"joh 1:1,12\" or joh 1:1-12\" or \"joh 1:1;joh 1:2-11;joh 1:12\"\n"
	 "\t--r\tVerse and TSK references, ie:\n"
	 "\t\t\t\"joh 1:1\" or \"joh 1:1,12\" or joh 1:1-12\" or \"joh 1:1;joh 1:2-11;joh 1:12\"\n"
	 "\t--b\tList Bible books, ie:\n"
	 "\t\t\ttrue or 1\n");
}
