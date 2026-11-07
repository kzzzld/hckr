# hckr

A Hacker News terminal app in C. Browse live new, past, comments, ask, show, and jobs feeds. Story tabs show titles; the Comments tab shows single-line comment previews.

## Build and run

On macOS, install the Xcode Command Line Tools (`xcode-select --install`), then:

```sh
brew bundle
make
./build/hckr
```

`Brewfile` includes ncurses for terminal rendering, libcurl for HTTP, libxml2 for HTML parsing and entity decoding, and pkgconf for discovering compiler and linker flags. The Makefile also supports system libraries through curl-config and xml2-config when pkg-config metadata is unavailable. On other platforms, install a C11 compiler, make, and development headers for ncurses, libcurl (7.85 or newer), and libxml2.

Switch tabs with Left/Right, `h`/`l`, or Tab/Shift-Tab. Navigation wraps around and the selected tab is highlighted. On narrow terminals the tab bar scrolls to keep the selected tab visible. Press `q`, Escape, or Ctrl-C to exit. Resizing redraws the UI; normal terminal settings are restored on exit.

Use `j`/`k` or Up/Down to scroll and `r` to reload. Each tab fetches its page when selected. Loading and request errors appear in the content area; keyboard navigation remains available while fetching. Requests use a 10-second connection timeout, a 20-second total timeout, and a 2 MiB response limit. TLS certificates are verified.

Use `make run` to build and launch, `make test` to run checks (Python 3 required), or `make clean` to remove build output. The fetching tests use a temporary localhost HTTP server.

## Configuration

Settings are loaded from `~/.config/hckr/config.ini`, or `$XDG_CONFIG_HOME/hckr/config.ini` when `XDG_CONFIG_HOME` is set to an absolute path. On startup, missing directories and `config.ini` are created automatically with built-in defaults. Existing files are preserved. Creation failures stop startup with a clear error. The file uses owner-only permissions (0600), and new directories use 0700.

```ini
[hackernews]
base_url = https://news.ycombinator.com

[ui]
initial_tab = new
```

An example is included in `config/config.ini`. Supported initial tabs are new, past, comments, ask, show, and jobs. Environment variable `HCKR_HN_URL` overrides the file's base URL. Precedence is environment, configuration file, then defaults.

Use unquoted `key = value` entries under the sections above. Blank lines and whole-line comments starting with `#` or `;` are accepted. Surrounding whitespace is ignored; inline comments and quoted values are not supported. Duplicate keys use the last value. Unknown sections or keys, empty values, and invalid initial tabs produce an error with the filename and line number. Base URLs may contain up to 2047 bytes. Each request uses this base URL plus the selected tab's relative path.

Configuration owns its strings and does not depend on the terminal UI.

## Layout

- `src/core`: shared configuration and UI-independent application logic.
- `src/core/http.c`: asynchronous HTTP requests and cancellation.
- `src/core/feed.c`: HTML parsing and plain-text feed items.
- `src/core/sections.c`: shared tab labels and paths relative to the configured base URL.
- `src/tui`: ncurses rendering and keyboard input.
- `src/main.c`: application startup.

A future CLI can use `src/core` without linking ncurses.
