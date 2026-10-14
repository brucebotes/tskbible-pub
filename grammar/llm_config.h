/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef LLM_CONFIG_H_
#define LLM_CONFIG_H_

#include <stdbool.h>
#define NOB_STRIP_PREFIX
#include "nob.h"
#include "jimp.h"

typedef struct {
    const char *role;
    const char *content;
} Role;

typedef struct {
    Role *items;
    size_t count;
    size_t capacity;
} Roles;

typedef struct {
    bool stream;
    const char *model;
    double temperature;
    Roles *messages;
} LLM_Query;

typedef struct {
    LLM_Query * llm_query;
    const char * default_model;
    double default_temperature;
} Config;

bool parse_config(Jimp *jimp, Config *c);
bool load_config();

#ifdef CONFIG_IMPLEMENTATION

bool parse_role(Jimp *jimp, Role *r)
{
    if (!jimp_object_begin(jimp)) return false;
    while (jimp_object_member(jimp)) {
        if (strcmp(jimp->string, "role") == 0) {
            if (!jimp_string(jimp)) return false;
            r->role = temp_strdup(jimp->string);
        } else if (strcmp(jimp->string, "content") == 0) {
            if (!jimp_string(jimp)) return false;
            r->content = temp_strdup(jimp->string);
        }
    }
    return jimp_object_end(jimp);
}

bool parse_llm_query(Jimp *jimp, LLM_Query *q)
{
    static Roles messages = {0};

    if (!jimp_object_begin(jimp)) return false;
    while (jimp_object_member(jimp)) {
        if (strcmp(jimp->string, "model") == 0) {
            if (!jimp_string(jimp)) return false;
            q->model = temp_strdup(jimp->string);
        } else if (strcmp(jimp->string, "stream") == 0) {
            if (!jimp_bool(jimp)) return false;
            q->stream = jimp->boolean;
        } else if (strcmp(jimp->string, "temperature") == 0) {
            if (!jimp_number(jimp)) return false;
            q->temperature = jimp->number;
        } else if (strcmp(jimp->string, "messages") == 0) {
            if (!jimp_array_begin(jimp)) return false;
            Role r = {0};
            while (jimp_array_item(jimp)) {
                if(!parse_role(jimp, &r)) break;
                da_append(&messages, r);
            }
            if (!jimp_array_end(jimp)) { return false; }
            q->messages = &messages;
        } else {
            jimp_unknown_member(jimp);
            return false;
        }
    }
    return jimp_object_end(jimp);
}

bool parse_config(Jimp *jimp, Config *c)
{
    static  LLM_Query llm = {0};
    if (!jimp_object_begin(jimp)) return false;
    while (jimp_object_member(jimp)) {
        if (strcmp(jimp->string, "default_model") == 0) {
            if (!jimp_string(jimp)) return false;
            c->default_model = temp_strdup(jimp->string);
        } else if (strcmp(jimp->string, "default_temperature") == 0) {
            if (!jimp_number(jimp)) return false;
            c->default_temperature = jimp->number;
        } else if (strcmp(jimp->string, "llm_query") == 0) {
            if(!parse_llm_query(jimp, &llm)) return false;
            c->llm_query = &llm;
        } else {
            jimp_unknown_member(jimp);
            return false;
        }
    }
    return jimp_object_end(jimp);
}

#endif // CONFIG_IMPLEMENTATION
#endif // CONFIG_H_
