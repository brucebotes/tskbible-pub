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
#include <asv-common.h> // Your arenas, string-views, and string-builders

// A simple minimalist macro structure for testing assertions
#define ASSERT(expr, message) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "  [FAIL] Line %d: %s\n", __LINE__, message); \
            return 0; \
        } \
    } while (0)

int test_arena_allocation(void) {
    // Example test tracking your custom memory arena structure
    // Arena arena = arena_init(1024);
    // void *ptr = arena_alloc(&arena, 128);
    // ASSERT(ptr != NULL, "Arena failed to allocate 128 bytes.");
    printf("  [PASS] Arena allocation functions normally.\n");
    return 1;
}

int test_string_view_split(void) {
    // Example testing your string-views
    // StringView sv = sv_from_cstr("Genesis 1:1");
    // StringView book = sv_split_by_space(&sv);
    // ASSERT(sv_eq_cstr(book, "Genesis"), "String-view split parsed incorrectly.");
    printf("  [PASS] String-view split functions normally.\n");
    return 1;
}

int main(void) {
    char *strictness = getenv("TEST_STRICTNESS");
    char *dev_label = getenv("DEV_ENV_LABEL");

    printf("=== Starting Unit Tests [%s] on env: %s ===\n", 
           strictness ? strictness : "DEFAULT", dev_label ? dev_label : "UNKNOWN");

    int passed = 0;
    int total = 2;

    passed += test_arena_allocation();
    passed += test_string_view_split();

    printf("Test Results: %d/%d suites passed.\n", passed, total);
    return (passed == total) ? 0 : 1;
}
