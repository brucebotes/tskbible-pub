/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <stdbool.h>
#include <stdlib.h>
// #include <stdio.h>
#include <asv-arena.h>
#include <asv-common.h>
#include <fcgi_stdio.h>

ChunkedArena global_arena;
ChunkedArena scratch_arena;

extern bool parse_bible_asset_files(void);
extern void display_page(const char *route);

// the one and only HTML output.
// it enables the routing to the correct stdout file
// which is different for CGI and fastCGI
// It depends if we include stdio.h or fcgi_stdio.h
void html_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stdout, format, args);
  va_end(args);
}

int main(void) {
  // Generate a large pre-allocated scracth area and reset it for scratch
  // purposes
  arena_alloc(&global_arena, 1024 * 1024 * 9);
  arena_reset(&global_arena);
  arena_alloc(&scratch_arena, 1024 * 32);
  arena_reset(&scratch_arena);

  parse_bible_asset_files();

  while (FCGI_Accept() >= 0) {

    display_page("ertsk");
#ifdef DEBUG
    printf("\nScratch ");
    arena_stats(&scratch_arena);
    printf("Global ");
    arena_stats(&global_arena);
#endif
    arena_reset(&scratch_arena);
  }

  arena_free_all(&global_arena);
  arena_free_all(&scratch_arena);

  return 0;
}
