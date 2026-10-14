/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "color.h"
#include "bible.h"
#include <stdbool.h>
#include <asv-common.h>
#include <asv-arena.h>

ChunkedArena global_arena;
ChunkedArena scratch_arena;

extern void pretty_print_gemtext_references(StringView ref);
extern bool parse_bible_asset_files(void);

int main(void) {
  // Generate a large pre-allocated scracth area and reset it for scratch
  // purposes
  arena_alloc(&global_arena, 1024*1024*9);
  arena_reset(&global_arena);
  arena_alloc(&scratch_arena, 1024*32);
  arena_reset(&scratch_arena);

  parse_bible_asset_files();

  char *query = getenv("QUERY_STRING");

  if (query != NULL && strlen(query) > 0) {
    printf("20 text/gemini; lang=en\r\n");
    printf("## Treasury of Scripture Knowledge\r\n");
    printf("Book Name Abbreviations:\r\n"
	   "ge, ex, le, nu, de, jos, jud, ru, 1sa, 2sa, 1ki, 2ki, 1ch, 2ch, ezr, ne,"
	   " es, job, ps, pr, ec, so, isa, jer, la, eze, da, ho, joe, am, ob, jon, mic,"
	   " na, hab, zep, hag, zec, mal, mt, mr, lu, joh, ac, ro, 1co, 2co, ga, eph,"
	   " php, col, 1th, 2th, 1ti, 2ti, tit, phm, heb, jas, 1pe, 2pe, 1jo, 2jo, 3jo,"
	   " jude, re\r\n");
    printf("=> /tsk/   Get another verse\r\n\r\n");
    printf("Your reference was: %s\r\n\r\n", query);
    StringView ref = (StringView){
      .data = query, .len = strlen(query)};
    pretty_print_gemtext_references(ref);
  } else {
    // Tell the browser to prompt for a verse 
    printf("10 Enter your Bible verse\r\n");
  }

#ifdef DEBUG
  printf("\nScratch ");arena_stats(&scratch_arena);
  printf("Global ");arena_stats(&global_arena);
#endif  
  arena_free_all(&global_arena);
  arena_free_all(&scratch_arena);
  return 0;
}



