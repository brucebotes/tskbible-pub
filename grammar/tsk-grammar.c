/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */


#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#define JSMN_STATIC
#include <asv-arena.h>
#include <asv-common.h>
#include <asv-hash-map.h>
#include <asv-parsers.h>
#include <asv-sb.h>
#include <asv-sv.h>
#include <log.h>

#include "bible.h"
#include "buf.h"
#include "jsmn.h"

ChunkedArena global_arena = {};
ChunkedArena scratch_arena = {};
ConfigMap app_config = {};

#define BUFFER_SIZE 32768
#define SPINNER SPINNER

// Global variable to control the spinner thread
int process_complete = 0;
// forward declarations
void usleep_alternative(long microseconds);
void *spinner_animation(void *arg);
bool read_response_and_print(const char *jsonn, unsigned long jsonn_len);
static size_t fetch_data(void *buffer, size_t size, size_t nmemb, void *userp);
char *query_llm(const char *json_query);
extern bool parse_bible_asset_files(void);


bool get_grammar_for_verse(void) {
#ifdef SPINNER
  pthread_t spinner_thread;

  // Start the spinner thread
  if (pthread_create(&spinner_thread, NULL, spinner_animation, NULL) != 0) {
    fprintf(stderr, "Error creating thread\n");
    return false;
  }
#endif

  StringView llm_query = map_get(&app_config, SV("llm_query"));
  ArenaStringBuilder sb = arena_sb_init(&scratch_arena, 1024 * 4);
  arena_sb_render_template(&sb, llm_query, &app_config);
  ASB_NILL(&sb);
#ifdef DEBUG
  printf("\n LLM query : \n^^^^^^\n%s\n^^^^^^\n", sb.buffer);
#endif
  char *json_result = query_llm(sb.buffer);

#ifdef DEBUG
  printf("\n Response : \n^^^^^^\n%s\n^^^^^^\n", json_result);
#endif
  if (json_result != NULL) {
    read_response_and_print((const char *)json_result, strlen(json_result));
  }
#ifdef SPINNER
  // Signal the spinner thread to stop
  process_complete = 1;

  // Wait for the spinner thread to finish
  pthread_join(spinner_thread, NULL);
#endif

  return true;
}

//---------------------------------------------------
// Note: The curl function use malloc and realloc via the buf.c functions
// allocates the resulting json string on the heap
// caller should free the string when not used
char *query_llm(const char *json_query) {
  CURL *curl;
  CURLcode res;
  char *json_str = NULL;

  /* In Windows, this inits the Winsock stuff */
  res = curl_global_init(CURL_GLOBAL_ALL);
  if (res != CURLE_OK) {
    log_die("curl_global_init() failed: %s\n", curl_easy_strerror(res));
  }

  curl = curl_easy_init();
  if (curl) {
    struct curl_slist *headers = NULL;
    // Set the Content-Type header
    headers = curl_slist_append(headers, "Content-Type: application/json");

    StringView ollama_server = map_get(&app_config, SV("ollama_server"));
    if( ollama_server.len >512 )
      log_die("ollam_server url too long, expect < 512, have %d\n", (int)ollama_server.len);
    char server[512] = {0};
    strncpy(server, ollama_server.data, ollama_server.len);
    server[ollama_server.len] = '\0';
				       
    // Set the URL
    curl_easy_setopt(curl, CURLOPT_URL, server);
    // Set the HTTP headers
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // Specify the POST data (libcurl doesn't copy this data by default,
    // so it must remain valid until curl_easy_perform() returns)
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_query);
    // Explicitly set the request to POST
    curl_easy_setopt(curl, CURLOPT_POST, 1L); // This option is redundant
                                                      // when CURLOPT_POSTFIELDS is used

    buf_t *buf = buf_size(NULL, BUFFER_SIZE);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fetch_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
    // Perform the request
    res = curl_easy_perform(curl);

    // Check for errors
    if (res != CURLE_OK) {
      log_die("curl_easy_perform failed: %s", curl_easy_strerror(res));
     }

    // Always cleanup
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers); // Free the header list
    json_str = arena_strndup(&scratch_arena, (const char *)buf->data, buf->len);
    free(buf->data);
    free(buf);
  }
  curl_global_cleanup();

  return json_str;
}

// concat curl chunks into buffer - NOTE: uses malloc and realloc!!
static size_t fetch_data(void *buffer, size_t size, size_t nmemb, void *userp) {
  buf_t *buf = (buf_t *)userp;
  size_t total = size * nmemb;

  if (buf->limit - buf->len < total) {
    buf = buf_size(buf, buf->limit + total);
    log_null(buf);
  }

  buf_concat(buf, buffer, total);

  return total;
}

// Function to manually parse and print escape sequences
static inline void parse_and_print(const char *input, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (input[i] == '\\') {
      // Handle common escape sequences
      switch (input[i + 1]) {
      case 'n':
        putchar('\n');
        i++; // Skip the 'n'
        break;
      case 't':
        putchar('\t');
        i++; // Skip the 't'
        break;
      case '\\':
        putchar('\\');
        i++; // Skip the second backslash
        break;
      case '"':
        putchar('"');
        i++; // Skip the quote
        break;
      // Add more cases for other escape sequences if needed
      default:
        // If not a recognized escape sequence, print the backslash and the next
        // character
        putchar(input[i]);
        putchar(input[i + 1]);
        i++;
        break;
      }
    } else {
      // Print normal characters as is
      putchar(input[i]);
    }
  }
}

static inline void print_duration(const char input[], size_t len) {
  if (len < 10) {
    printf("less than 1");
    return;
  }
  for (size_t i = 0; i < len - 9; ++i) {
    putchar(input[i]);
  }
}

static inline int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

bool read_response_and_print(const char *jsonn, unsigned long jsonn_len) {
  int i;
  int r;
  jsmn_parser p;
  jsmntok_t t[128]; /* We expect no more than 128 tokens */

  jsmn_init(&p);
  r = jsmn_parse(&p, (const char *)jsonn, jsonn_len, t,
                 sizeof(t) / sizeof(t[0]));
  if (r < 0) {
    printf("Failed to parse json: %d\n", r);
    return false;
  }

  /* Assume the top-level element is an object */
  if (r < 1 || t[0].type != JSMN_OBJECT) {
    printf("Object expected\n");
    return false;
  }

  printf("\n");
  /* Loop over all keys of the root object */
  for (i = 1; i < r; i++) {
    if (jsoneq((const char *)jsonn, &t[i], "model") == 0) {
      /* We may use strndup() to fetch string value */
      printf("  Model: %.*s\n", t[i + 1].end - t[i + 1].start,
             jsonn + t[i + 1].start);
      i++;
    } else if (jsoneq((const char *)jsonn, &t[i], "total_duration") == 0) {
      /* We may want to do strtol() here to get numeric value */
      printf("  Duration: ");
      print_duration(jsonn + t[i + 1].start, t[i + 1].end - t[i + 1].start);
      printf(" sec\n");
      i++;
    } else if (jsoneq((const char *)jsonn, &t[i], "message") == 0) {
      int j;
      if (t[i + 1].type != JSMN_OBJECT) {
        continue;
      }
      for (j = 0; j < t[i + 1].size; j++) {
        jsmntok_t *g = &t[i + j * 2 + 2];
        if (jsoneq((const char *)jsonn, g, "role") == 0) {
          jsmntok_t *h = &t[i + j * 2 + 3];
          printf("  role:  %.*s\n", h->end - h->start, jsonn + h->start);
        } else if (jsoneq((const char *)jsonn, g, "content") == 0) {
          jsmntok_t *h = &t[i + j * 2 + 3];
          printf("  content start: \n------------------------------------\n");
          parse_and_print(jsonn + h->start, h->end - h->start);
          printf("\n------------------------------------\n  content end\n");
        }
#ifdef DEBUG
        else if (jsoneq((const char *)jsonn, g, "thinking") == 0) {
          jsmntok_t *h = &t[i + j * 2 + 3];
          printf("  thinking start: \n------------------------------------\n");
          parse_and_print(jsonn + h->start, h->end - h->start);
          printf("\n------------------------------------\n  thinking end\n");
        }
        else {
          printf("Unexpected content token : %.*s\n", g->end - g->start,
                 jsonn + g->start);
        }
#endif
      }
      i += t[i + 1].size * 2 +
           1; // each element in an object consists of two tokens
    } else {
#ifdef DEBUG
      printf("Unexpected key: %.*s\n", t[i].end - t[i].start,
             jsonn + t[i].start);
#endif
      i++;
    }
  }
  return true;
}


#ifdef SPINNER

#if defined(__APPLE__) && defined(__MACH__)
// MUST be the very first line before including <time.h>
#define _POSIX_C_SOURCE 199309L 
#include <time.h>
void delay_microseconds(long microseconds) {
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000;
    ts.tv_nsec = (microseconds % 1000000) * 1000;
    
    nanosleep(&ts, NULL); // Will now compile with -std=c23
}
#else
#include <time.h>
#include <threads.h> // Native C standard threads

void delay_microseconds(long microseconds) {
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000;
    ts.tv_nsec = (microseconds % 1000000) * 1000;
    
    // Pure C23 standard high-resolution sleep
    thrd_sleep(&ts, NULL); 
}

#endif
#pragma GCC diagnostic ignored "-Wunused-parameter"
void *spinner_animation(void *arg) {
  // Change to an array of string pointers using double quotes
  const char *frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
  
  // Calculate the actual number of frames dynamically (in this case, 10)
  int num_frames = sizeof(frames) / sizeof(frames[0]);
  int i = 0;
  
  // Hide the cursor
  printf("\e[?25l");
  while (!process_complete) {
    printf("\rProcessing... %s", frames[i++ % num_frames]);
    fflush(stdout);             
    delay_microseconds(100000); 
  }
  // Restore the cursor
  printf("\e[?25h");
  printf("\rProcessing... Done!  \n"); 
  return NULL;
}
#endif

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

bool read_load_and_merge_config(int argc, char *argv[argc + 1]) {
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
    return -1;
  }
  if (!read_load_and_merge_config(argc, argv)) {
    fprintf(stderr, "Unable to load 'config.txt' file\n");
    return -1;
  }
  
  parse_bible_asset_files();

  StringView verse = map_get(&app_config, SV("v"));
  if (verse.len > 0)
    with_lifetime(&scratch_arena) {
      ArenaStringBuilder sb = arena_sb_init(&scratch_arena, 1024);

      // Build the verse ( ref + text ) and stick it into the app_config
      // for global access. Used to populate the llm_query template
      // that has a tag {{ VERSE_PLACEHOLDER }}
      StringView verse_text = get_verse(verse);
      arena_sb_append_sv(&sb, verse);
      arena_sb_append_fmt(&sb, " ");
      arena_sb_append_sv(&sb, verse_text);
      StringView llm_verse = (StringView){.data = sb.buffer, .len = sb.length};
      map_insert(&app_config, SV("VERSE_PLACEHOLDER"), llm_verse);

      printf("Grammar for:\n");
      sv_print(llm_verse);
      putchar('\n');
      get_grammar_for_verse();
    }
#ifdef DEBUG
  printf("\nScratch ");
  arena_stats(&scratch_arena);
  printf("Global ");
  arena_stats(&global_arena);
#endif
  arena_free_all(&global_arena);
  arena_free_all(&scratch_arena);

  return 0;
}

void print_usage(int argc, char *argv[]){
  printf("\nusage: %s --command=value\n", argv[0]);
  printf("\tcommand\t\tdescription\n");
  printf("\t--v\t\tVerse, ie: \"joh 1:1\"\n"
	 "\t--model\t\toverride the llm model defined in config.txt\n"
	 "\t--temperature\toverride the temperature setting in config.txt\n");
}
