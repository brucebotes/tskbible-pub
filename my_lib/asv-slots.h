/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef ASV_SLOTS_H_
#define ASV_SLOTS_H_

#include <asv-common.h>
#include <asv-arena.h>

static inline void slots_init(ArenaSVSlots *svs, ChunkedArena *arena) {
    svs->arena = arena;
    svs->head = NULL;
    svs->current = NULL;
    svs->current_count = 0;
    svs->total_count = 0;
}

static inline void slots_reset(ArenaSVSlots *svs) {
    // 🎯 RECYCLING UNLOCKED: Retain svs->head and svs->current block chains intact!
    // Simply rewind the active pointers back to the very front node.
    svs->current = svs->head; 
    svs->current_count = 0;
    svs->total_count = 0;
}

static inline void slots_append(ArenaSVSlots *svs, StringView sv) {
    if (svs->arena == NULL) return; //

    // If this is the absolute first allocation ever run:
    if (svs->head == NULL) {
        SVSlotBlock *new_block = (SVSlotBlock *)arena_alloc(svs->arena, sizeof(SVSlotBlock));
        if (!new_block) return;
        new_block->next = NULL;
        svs->head = new_block;
        svs->current = new_block;
        svs->current_count = 0;
    }
    // If our current block slot space is full:
    else if (svs->current_count >= SLOT_BLOCK_SIZE) { //
        
        // 🔄 THE RECYCLING CHECK: 
        // If a next block was already allocated in a previous iteration, reuse it!
        if (svs->current->next != NULL) {
            svs->current = svs->current->next;
        } else {
            // Otherwise, we hit a new peak high-water mark; carve a fresh block
            SVSlotBlock *new_block = (SVSlotBlock *)arena_alloc(svs->arena, sizeof(SVSlotBlock));
            if (!new_block) return;
            new_block->next = NULL;
            svs->current->next = new_block;
            svs->current = new_block;
        }
        svs->current_count = 0;
    }

    // Safely overwrite or populate the warm slot index safely
    svs->current->items[svs->current_count] = sv; //
    svs->current_count++; //
    svs->total_count++; //
}


static inline StringView slots_get(ArenaSVSlots *svs, size_t index) {
    if (index >= svs->total_count) return (StringView){0};

    size_t block_offset = index / SLOT_BLOCK_SIZE;
    size_t item_offset = index % SLOT_BLOCK_SIZE;

    SVSlotBlock *b = svs->head;
    for (size_t i = 0; i < block_offset && b != NULL; i++) {
        b = b->next;
    }

    if (b == NULL) return (StringView){0};
    return b->items[item_offset];
}

static inline StringView* slots_last(ArenaSVSlots *svs) {
    if (svs->current == NULL || svs->current_count == 0) return NULL;
    return &svs->current->items[svs->current_count - 1];
}

#endif
