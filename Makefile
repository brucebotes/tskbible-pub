# ----- CONFIG -----
SRC_DIR := src
SRC_BUILD_DIR := src_build
LIB_DIR := my_lib
ORG_DIR := org_mode
BUILD_DIR := build
ASSETS_DIR := assets
GRAMM_DIR := grammar
TARGET := tskjv

# ----- ENVIRONMENTAL LOGGING -----
ENV_LOG := $(BUILD_DIR)/build_env.txt

# Capture the exact compiler path and version string cleanly
GCC_VERSION := $(shell $(CC) -v 2>&1 | tail -n 1)
ARCH_INFO   := $(shell uname -m)


# ----- EXTEND CONFIG -----
TEST_DIR   := src_build
TEST_EXEC  := $(BUILD_DIR)/test_lib

# --- Add to the existing RULES block ---
.PHONY: all clean test

CC := gcc -std=c23
# -I$(SRC_DIR) and -I$(LIB_DIR) allow #include "header.h" for both directories
CPPFLAGS := -I$(SRC_DIR) -I$(LIB_DIR) -I.

# --- MODE HANDLING ---
MODE ?= debug
# Strip any accidental whitespace introduced by shell exports
MODE := $(strip $(MODE))

CFLAGS_BASE := -Wall -Wextra

ifeq ($(MODE), release)
    CFLAGS   := $(CFLAGS_BASE) -O3 -DNDEBUG
    LDFLAGS  := 
else ifeq ($(MODE), debug)
    CFLAGS   := $(CFLAGS_BASE) -O0 -g3 -DDEBUG
    LDFLAGS  := 
else ifeq ($(MODE), asan)
    CFLAGS   := $(CFLAGS_BASE) -O0 -g3 -fsanitize=address -fno-omit-frame-pointer
    LDFLAGS  := -fsanitize=address
else
    $(error Unknown MODE: $(MODE). Valid modes are: release, debug, asan)
endif

# --- SOURCE & OBJECT GENERATION ---
# Gather all .c files from both directories
SRCS := $(wildcard $(SRC_DIR)/*.c) $(wildcard $(LIB_DIR)/*.c)

# Map src/main.c -> build/src/main.o and my_lib/utils.c -> build/my_lib/utils.o
# This prevents filename collisions between src/ and my_lib/
OBJS := $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS := $(OBJS:.o=.d)
BIBLE := $(BUILD_DIR)/$(SRC_DIR)/parse_bible.o $(BUILD_DIR)/$(SRC_DIR)/kjv.o $(BUILD_DIR)/$(SRC_DIR)/tsk.o
HTML_EXT := $(BUILD_DIR)/$(SRC_DIR)/cgi_html_page.o $(BUILD_DIR)/$(SRC_DIR)/qs_parse.o $(BUILD_DIR)/$(SRC_DIR)/html_verse.o


# ----- RULES -----

# Default rule
all:	$(ENV_LOG) \
	$(BUILD_DIR)/tskjv \
	$(BUILD_DIR)/serve_tskjv_gemtext.cgi \
	$(BUILD_DIR)/serve_tskjv_html.cgi \
	$(BUILD_DIR)/serve_tskjv_html.fcgi \
	$(BUILD_DIR)/c-org-expand \
	$(BUILD_DIR)/c-org-collapse \
	$(BUILD_DIR)/tskjvg

assets: $(BUILD_DIR)/bin2c \
	$(SRC_DIR)/kjv.c \
	$(SRC_DIR)/tsk.c

# Include dependency files (.d) if they exist
-include $(DEPS)

# --------------------- DIRENV driven builds -------------------------------------
$(ENV_LOG):
	$(call dump_env_log)

# If the build_env.txt exists, check if the recorded MODE matches the current MODE.
# If they don't match, force a clean build automatically!
ifneq ($(wildcard $(ENV_LOG)),)
    PREV_MODE := $(shell grep "Build MODE:" $(ENV_LOG) | awk '{print $$NF}')
    ifneq ($(PREV_MODE),$(MODE))
        $(shell rm -rf $(BUILD_DIR))
    endif
endif


# --------------------------------------------------------
# Link step
$(BUILD_DIR)/tskjv: $(BUILD_DIR)/$(SRC_DIR)/tskjv.o $(BIBLE) $(BUILD_DIR)/$(SRC_DIR)/text_verse.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/serve_tskjv_gemtext.cgi: $(BUILD_DIR)/$(SRC_DIR)/serve_tskjv_gemtext.o $(BIBLE) $(BUILD_DIR)/$(SRC_DIR)/gemtext_verse.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/serve_tskjv_html.cgi: $(BUILD_DIR)/$(SRC_DIR)/serve_tskjv_html.o $(BIBLE) $(HTML_EXT)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/serve_tskjv_html.fcgi: $(BUILD_DIR)/$(SRC_DIR)/serve_tskjv_html_fastcgi.o $(BIBLE) $(HTML_EXT)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lfcgi

$(BUILD_DIR)/tskjvg: $(BUILD_DIR)/$(GRAMM_DIR)/tsk-grammar.o $(BUILD_DIR)/$(GRAMM_DIR)/buf.o $(BIBLE) $(BUILD_DIR)/$(LIB_DIR)/log.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lcurl

$(BUILD_DIR)/c-org-expand: $(BUILD_DIR)/$(ORG_DIR)/c-org-expand.o $(BIBLE)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/c-org-collapse: $(BUILD_DIR)/$(ORG_DIR)/c-org-collapse.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# --------------------------------------------------------
# Pattern rule for src/ folder -> build/src/
$(BUILD_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -MT $@ -MF $(@:.o=.d) -c $< -o $@

# Pattern rule for my_lib/ folder -> build/my_lib/
$(BUILD_DIR)/$(LIB_DIR)/%.o: $(LIB_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -MT $@ -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/$(ORG_DIR)/%.o: $(ORG_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -MT $@ -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/$(GRAMM_DIR)/%.o: $(GRAMM_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -MT $@ -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/bin2c: $(SRC_BUILD_DIR)/bin2c.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@

$(SRC_DIR)/kjv.c: $(BUILD_DIR)/bin2c $(ASSETS_DIR)/kjv.txt
	@mkdir -p $(dir $@)
	$(BUILD_DIR)/bin2c kjv_data $(ASSETS_DIR)/kjv.txt $(SRC_DIR)/kjv.c

$(SRC_DIR)/tsk.c: $(BUILD_DIR)/bin2c $(ASSETS_DIR)/tsk.txt
	@mkdir -p $(dir $@)
	$(BUILD_DIR)/bin2c tsk_data $(ASSETS_DIR)/tsk.txt $(SRC_DIR)/tsk.c


# Cleanup
clean:
	rm -rf $(BUILD_DIR)

# ---------------------- TEST RUNS ------------------------------------
# Dedicated test suite execution pipeline
test: $(TEST_EXEC)
	@mkdir -p $(TEST_LOG_DIR)
	@echo "Executing test target profile inside isolated environment..."
	@# Run the binary and pipe outputs to the local direnv directory log
	@./$(TEST_EXEC) 2>&1 | tee $(TEST_LOG_DIR)/run_latest.log

# Compile and Link the test binary smoothly
# Merges your testing rules with your underlying 'my_lib' components
$(TEST_EXEC): $(TEST_DIR)/test_lib.c $(wildcard $(LIB_DIR)/*.c)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

# ------------------ SHELL MACROS -----------------------------------

# A macro command to dump our environmental vars into the build log file
define dump_env_log
	@mkdir -p $(BUILD_DIR)
	@echo "=== TSKJV2 Build Environment Log ===" > $(ENV_LOG)
	@echo "Timestamp:    $(shell date)" >> $(ENV_LOG)
	@echo "Build Host:   $(shell hostname)" >> $(ENV_LOG)
	@echo "Architecture: $(ARCH_INFO)" >> $(ENV_LOG)
	@echo "Build MODE:   $(MODE)" >> $(ENV_LOG)
	@echo "Compiler CC:  $(CC)" >> $(ENV_LOG)
	@echo "Compiler Ver: $(GCC_VERSION)" >> $(ENV_LOG)
	@echo "CFLAGS:       $(CFLAGS)" >> $(ENV_LOG)
	@echo "CPPFLAGS:     $(CPPFLAGS)" >> $(ENV_LOG)
	@echo "====================================" >> $(ENV_LOG)
endef

