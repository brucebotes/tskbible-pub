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
#include <stdbool.h>
#include <asv-common.h>
#include <asv-arena.h>

ChunkedArena global_arena;
ChunkedArena scratch_arena;

extern bool parse_bible_asset_files(void);
extern void display_page(const char *route);

void html_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

int main(void) {
  // Generate a large pre-allocated scracth area and reset it for scratch
  // purposes
  arena_alloc(&global_arena, 1024*1024*9);
  arena_reset(&global_arena);
  //arena_alloc(&scratch_arena, 1024*32);
  //arena_reset(&scratch_arena);

  parse_bible_asset_files();

  display_page("ertsk");

#ifdef DEBUG
  printf("\nScratch ");arena_stats(&scratch_arena);
  printf("Global ");arena_stats(&global_arena);
#endif     
  arena_free_all(&global_arena);
  arena_free_all(&scratch_arena);
  return 0;
}

