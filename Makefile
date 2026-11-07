THEME_DIR ?= $(CURDIR)/themes

CC = cc
PKG_CONFIG ?= pkg-config

NCURSES_PACKAGE := $(shell $(PKG_CONFIG) --exists ncursesw 2>/dev/null && echo ncursesw || echo ncurses)
NCURSES_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(NCURSES_PACKAGE) 2>/dev/null)
NCURSES_LIBS := $(shell $(PKG_CONFIG) --libs $(NCURSES_PACKAGE) 2>/dev/null || echo -lncurses)

CPPFLAGS += -DHCKR_THEME_DIR='"$(THEME_DIR)"' -Isrc -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 $(NCURSES_CFLAGS)
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
HTTP_CFLAGS := $(shell $(PKG_CONFIG) --cflags libcurl 2>/dev/null || curl-config --cflags)
HTTP_LIBS := $(shell $(PKG_CONFIG) --libs libcurl 2>/dev/null || curl-config --libs)
XML_CFLAGS := $(shell $(PKG_CONFIG) --cflags libxml-2.0 2>/dev/null || xml2-config --cflags | sed -E 's@/include(/libxml2)?@/include/libxml2@g')
XML_LIBS := $(shell $(PKG_CONFIG) --libs libxml-2.0 2>/dev/null || xml2-config --libs)
CPPFLAGS += $(HTTP_CFLAGS) $(XML_CFLAGS)
LDLIBS += $(NCURSES_LIBS) $(HTTP_LIBS) $(XML_LIBS)
ifeq ($(OS),Windows_NT)
LDLIBS += -lshell32 -lole32
endif

SOURCES := src/main.c src/core/config.c src/core/color.c src/core/sections.c src/core/http.c src/core/feed.c src/core/html.c src/core/discussion.c src/platform/browser.c src/tui/tui.c src/tui/theme.c src/tui/input.c src/tui/mouse.c src/tui/discussion.c
OBJECTS := $(SOURCES:src/%.c=build/%.o)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all run clean test
all: build/hckr

build/hckr: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

run: build/hckr
	./build/hckr

build/feed_test: tests/feed_test.c build/core/http.o build/core/feed.o build/core/html.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/browser_test: tests/browser_test.c build/platform/browser.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/discussion_test: tests/discussion_test.c build/core/discussion.o build/core/feed.o build/core/html.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

INPUT_TEST_LIBS := -pthread
ifeq ($(shell uname -s),Linux)
INPUT_TEST_LIBS += -lutil
endif

build/input_test: tests/input_test.c build/tui/input.o build/tui/mouse.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) $(INPUT_TEST_LIBS) -o $@

build/mouse_test: tests/mouse_test.c build/tui/mouse.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@

build/config_test: tests/config_test.c build/core/config.o build/core/color.o build/core/sections.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@

build/theme_test: tests/theme_test.c build/tui/theme.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test: build/theme_test build/config_test build/hckr build/feed_test build/browser_test build/discussion_test build/input_test build/mouse_test
	./build/theme_test
	./build/mouse_test
	./build/input_test
	./build/discussion_test
	python3 tests/config_test.py
	python3 tests/feed_test.py
	python3 tests/browser_test.py

clean:
	rm -rf build

-include $(DEPS)
