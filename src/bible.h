/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef BIBLE_H_
#define BIBLE_H_

#include <stddef.h>
#include <asv-common.h>
#include <asv-hash-map.h>
#include <asv-sb.h>

enum boek { _ge=1, _ex, _le, _nu, _de, _jos, _jud, _ru, _1sa, _2sa, _1ki, _2ki, 
  _1ch, _2ch, _ezr, _ne, _es, _job, _ps, _pr, _ec, _so, _isa, _jer, 
  _la, _eze, _da, _ho, _joe, _am, _ob, _jon, _mic, _na, _hab, _zep, 
  _hag, _zec, _mal, _mt, _mr, _lu, _joh, _ac, _ro, _1co, _2co, _ga, 
  _eph, _php, _col, _1th, _2th, _1ti, _2ti, _tit, _phm, _heb, _jas, _1pe, 
  _2pe, _1jo, _2jo, _3jo, _jude, _re, _book_num};

// Here is the Compound literal macro (T) {INIT} with name BOOK_NAME
# define BOOK_NAME /**/                 \
    (char const*const[_book_num])   {   \
        [_ge] =  "ge",   				\
        [_ex] =  "ex",					\
        [_le] =  "le",					\
        [_nu] =  "nu",					\
        [_de] =  "de",					\
        [_jos] = "jos",					\
        [_jud] = "jud",					\
        [_ru] =  "ru",					\
        [_1sa] = "1sa",					\
        [_2sa] = "2sa",					\
        [_1ki] = "1ki",					\
        [_2ki] = "2ki",					\
        [_1ch] = "1ch",					\
        [_2ch] = "2ch",					\
        [_ezr] = "ezr",					\
        [_ne] =  "ne",					\
        [_es] =  "es",					\
        [_job] = "job",					\
        [_ps] =  "ps",					\
        [_pr] =  "pr",					\
        [_ec] =  "ec",					\
        [_so] =  "so",					\
        [_isa] = "isa",					\
        [_jer] = "jer",					\
        [_la] = "la",					\
        [_eze] = "eze",					\
        [_da] =  "da",					\
        [_ho] =  "ho",					\
        [_joe] = "joe",					\
        [_am] =  "am",					\
        [_ob] =  "ob",					\
        [_jon] = "jon",					\
        [_mic] = "mic",					\
        [_na] =  "na",					\
        [_hab] = "hab",					\
        [_zep] = "zep",					\
        [_hag] = "hag",  				\
        [_zec] = "zec",					\
        [_mal] = "mal",					\
        [_mt] =  "mt",					\
        [_mr] =  "mr",					\
        [_lu] =  "lu",					\
        [_joh] = "joh",					\
        [_ac] =  "ac",					\
        [_ro] =  "ro",					\
        [_1co] = "1co",					\
        [_2co] = "2co",					\
        [_ga] =  "ga",					\
        [_eph] = "eph",  				\
        [_php] = "php",					\
        [_col] = "col",					\
        [_1th] = "1th",					\
        [_2th] = "2th",					\
        [_1ti] = "1ti",					\
        [_2ti] = "2ti",					\
        [_tit] = "tit",					\
        [_phm] = "phm",					\
        [_heb] = "heb",					\
        [_jas] = "jas",					\
        [_1pe] = "1pe",					\
        [_2pe] = "2pe",  				\
        [_1jo] = "1jo",					\
        [_2jo] = "2jo",					\
        [_3jo] = "3jo",					\
        [_jude] = "jude", 				\
        [_re] =  "re",					\
    }

# define BOOK_NUMBER /**/               \
    (char const*const[_book_num])   {   \
        [_ge] =   "1", 				    \
        [_ex] =   "2",   			    \
        [_le] =   "3",   			    \
        [_nu] =   "4",   			    \
        [_de] =   "5",   			    \
        [_jos] =  "6",   			    \
        [_jud] =  "7",   			    \
        [_ru] =   "8",   			    \
        [_1sa] =  "9",   			    \
        [_2sa] =  "10",  			    \
        [_1ki] =  "11",  			    \
        [_2ki] =  "12",  			    \
        [_1ch] =  "13",  			    \
        [_2ch] =  "14",  			    \
        [_ezr] =  "15",  			    \
        [_ne] =   "16",  			    \
        [_es] =   "17",  			    \
        [_job] =  "18",  			    \
        [_ps] =   "19",  			    \
        [_pr] =   "20",  			    \
        [_ec] =   "21",  			    \
        [_so] =   "22",  			    \
        [_isa] =  "23",  			    \
        [_jer] =  "24",  			    \
        [_la] =   "25",  			    \
        [_eze] =  "26",  			    \
        [_da] =   "27",  			    \
        [_ho] =   "28",  			    \
        [_joe] =  "29",  			    \
        [_am] =   "30",  			    \
        [_ob] =   "31",  			    \
        [_jon] =  "32",  			    \
        [_mic] =  "33",  			    \
        [_na] =   "34",  			    \
        [_hab] =  "35",  			    \
        [_zep] =  "36",  			    \
        [_hag] =  "37",  			    \
        [_zec] =  "38",  			    \
        [_mal] =  "39",  			    \
        [_mt] =   "40",  			    \
        [_mr] =   "41",  			    \
        [_lu] =   "42",  			    \
        [_joh] =  "43",  			    \
        [_ac] =   "44",  			    \
        [_ro] =   "45",  			    \
        [_1co] =  "46",  			    \
        [_2co] =  "47",  			    \
        [_ga] =   "48",  			    \
        [_eph] =  "49",  			    \
        [_php] =  "50",  			    \
        [_col] =  "51",  			    \
        [_1th] =  "52",  			    \
        [_2th] =  "53",  			    \
        [_1ti] =  "54",  			    \
        [_2ti] =  "55",  			    \
        [_tit] =  "56",  			    \
        [_phm] =  "57",  			    \
        [_heb] =  "58",  			    \
        [_jas] =  "59",  			    \
        [_1pe] =  "60",  			    \
        [_2pe] =  "61",				    \
        [_1jo] =  "62",  			    \
        [_2jo] =  "63",  			    \
        [_3jo] =  "64",  			    \
        [_jude]=  "65",				    \
        [_re] =   "66",  			    \
    }

// -------------------------------------
constexpr size_t BIBLE_HASH_CAPACITY = 1024 * 32;

typedef struct {
    StringView key;
    StringView value;
    bool occupied; // Tracks if this hash bucket is taken
    ArenaSVSlots *tsk_refs;
} BibleEntry;

typedef struct {
    BibleEntry buckets[BIBLE_HASH_CAPACITY];
    size_t count;
} BibleMap;
// ---------------------------------

extern char const*const book[];
extern const unsigned char kjv_data[];
extern const size_t kjv_data_len;
extern const unsigned char tsk_data[];
extern const size_t tsk_data_len;

#ifdef BIBLE_IMPLEMENTATION
BibleMap bible = {};
#else
extern BibleMap bible;
#endif

static inline bool bible_insert(BibleMap *map, StringView key, BibleEntry value) {
    if (map->count >= BIBLE_HASH_CAPACITY) return false; // Map is completely full

    uint32_t hash = hash_string_view(key);
    // Fast bitwise modulo operation (only works if CAPACITY is a power of 2)
    uint32_t index = hash & (BIBLE_HASH_CAPACITY - 1);

    while (map->buckets[index].occupied) {
        // If the key already exists, overwrite the value and return
        if (sv_equals(map->buckets[index].key, key)) {
            map->buckets[index] = value;
            return true;
        }
        // Linear probing: collision occurred, try the next index slot
        index = (index + 1) & (BIBLE_HASH_CAPACITY - 1);
    }

    // Found an empty slot! Save the window structures cleanly
    map->buckets[index].key = key;
    map->buckets[index] = value;
    map->buckets[index].occupied = true;
    map->count++;
    return true;
}

static inline BibleEntry bible_get(BibleMap *map, StringView key) {
    uint32_t hash = hash_string_view(key);
    uint32_t index = hash & (BIBLE_HASH_CAPACITY - 1);
    uint32_t start_index = index;

    while (map->buckets[index].occupied) {
        if (sv_equals(map->buckets[index].key, key)) {
            return map->buckets[index]; // Success! Key found
        }
        index = (index + 1) & (BIBLE_HASH_CAPACITY - 1);
        
        // Prevent infinite loops if we circle entirely back to the start
        if (index == start_index) break; 
    }

    return (BibleEntry) {};
}

static inline StringView get_verse(StringView ref) {
  BibleEntry e = {};
  e = bible_get(&bible, ref);
  return e.value;
}

static inline StringView arena_sb_emphasis_first_occurance(
    ArenaStringBuilder *sb, 
    StringView *source, 
    StringView target, 
    StringView prefix_emphasis_marker,
    StringView postfix_emphasis_marker
) {
    if (target.len == 0) return *source;

    StringView current = *source;

    size_t index = sv_find(current, target);
        
    // If the target sequence isn't found, emit the remaining text block and break
    if (index == current.len) {
      arena_sb_append_sv(sb, current);
    } else {
      // 1. Append everything up to the match location coordinates
      StringView pre_match = sv_take(&current, index);
      arena_sb_append_sv(sb, pre_match);

      // 2. Append the replacement token piece
      arena_sb_append_sv(sb, prefix_emphasis_marker);
      arena_sb_append_sv(sb, target);
      arena_sb_append_sv(sb, postfix_emphasis_marker);

      // 3. Skip past the target match length to continue scanning the remainder string
      sv_take(&current, target.len);
      
      // 4. set the source start to our current end - ready for next markup
      *source = current;
    }

    return (StringView){ .data = sb->buffer, .len = sb->length };
}

#ifdef BIBLE_IMPLEMENTATION
char const*const book[_book_num] = {
    [_ge] =  "ge",   
    [_ex] =  "ex",
    [_le] =  "le",
    [_nu] =  "nu",
    [_de] =  "de",
    [_jos] = "jos",
    [_jud] = "jud",
    [_ru] =  "ru",
    [_1sa] = "1sa",
    [_2sa] = "2sa",
    [_1ki] = "1ki",
    [_2ki] = "2ki",
    [_1ch] = "1ch",
    [_2ch] = "2ch",
    [_ezr] = "ezr",
    [_ne] =  "ne",
    [_es] =  "es",
    [_job] = "job",
    [_ps] =  "ps",
    [_pr] =  "pr",
    [_ec] =  "ec",
    [_so] =  "so",
    [_isa] = "isa",
    [_jer] = "jer",
    [_la] = "la",
    [_eze] = "eze",
    [_da] =  "da",
    [_ho] =  "ho",
    [_joe] = "joe",
    [_am] =  "am",
    [_ob] =  "ob",
    [_jon] = "jon",
    [_mic] = "mic",
    [_na] =  "na",
    [_hab] = "hab",
    [_zep] = "zep",
    [_hag] ="hag",  
    [_zec] = "zec",
    [_mal] = "mal",
    [_mt] =  "mt",
    [_mr] =  "mr",
    [_lu] =  "lu",
    [_joh] = "joh",
    [_ac] =  "ac",
    [_ro] =  "ro",
    [_1co] = "1co",
    [_2co] = "2co",
    [_ga] =  "ga",
    [_eph] ="eph",  
    [_php] = "php",
    [_col] = "col",
    [_1th] = "1th",
    [_2th] = "2th",
    [_1ti] = "1ti",
    [_2ti] = "2ti",
    [_tit] = "tit",
    [_phm] = "phm",
    [_heb] = "heb",
    [_jas] = "jas",
    [_1pe] = "1pe",
    [_2pe] ="2pe",  
    [_1jo] = "1jo",
    [_2jo] = "2jo",
    [_3jo] = "3jo",
    [_jude] ="jude", 
    [_re] =  "re",
};

//static struct BookToNames  { char *key; char *value;} *book_to_names;
//static struct NamesToBook { char * key; char * value; } *names_to_book;


// this only works with c23 standard
// #ifdef EMBED_ASSETS
// const unsigned char kjv_data[] = {
//     #embed "../assets/kjv.txt"
// };
//
// const size_t kjv_data_len = sizeof(kjv_data);
//
// const unsigned char tsk_data[] = {
//     #embed "../assets/tsk.txt"
// };
//
// const size_t tsk_data_len = sizeof(tsk_data);
// #endif  // EMBED_ASSETS


#endif // BIBLE_IMPLEMENTATION
#endif // BIBLE_H_
