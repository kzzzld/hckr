CC = cc
PKG_CONFIG ?= pkg-config

NCURSES_PACKAGE := $(shell $(PKG_CONFIG) --exists ncursesw 2>/dev/null && echo ncursesw || echo ncurses)
NCURSES_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(NCURSES_PACKAGE) 2>/dev/null)
NCURSES_LIBS := $(shell $(PKG_CONFIG) --libs $(NCURSES_PACKAGE) 2>/dev/null || echo -lncurses)

CPPFLAGS += -Isrc -D_POSIX_C_SOURCE=200809L $(NCURSES_CFLAGS)
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
HTTP_CFLAGS := $(shell $(PKG_CONFIG) --cflags libcurl 2>/dev/null || curl-config --cflags)
HTTP_LIBS := $(shell $(PKG_CONFIG) --libs libcurl 2>/dev/null || curl-config --libs)
XML_CFLAGS := $(shell $(PKG_CONFIG) --cflags libxml-2.0 2>/dev/null || xml2-config --cflags | sed -E 's@/include(/libxml2)?@/include/libxml2@g')
XML_LIBS := $(shell $(PKG_CONFIG) --libs libxml-2.0 2>/dev/null || xml2-config --libs)
CPPFLAGS += $(HTTP_CFLAGS) $(XML_CFLAGS)
LDLIBS += $(NCURSES_LIBS) $(HTTP_LIBS) $(XML_LIBS)

SOURCES := src/main.c src/core/config.c src/core/sections.c src/core/http.c src/core/feed.c src/tui/tui.c
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

build/feed_test: tests/feed_test.c build/core/http.o build/core/feed.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test: build/hckr build/feed_test
	python3 tests/config_test.py
	python3 tests/feed_test.py

clean:
	rm -rf build

-include $(DEPS)
