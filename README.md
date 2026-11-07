# hckr

A Hacker News terminal app in C. Browse live news, new, past, comments, ask, show, and jobs feeds. Story tabs show titles; the Comments tab shows single-line comment previews.

## Build and run

On macOS, install the Xcode Command Line Tools (`xcode-select --install`), then:

```sh
brew bundle
make
./build/hckr
```

`Brewfile` includes ncurses for terminal rendering, libcurl for HTTP, libxml2 for HTML parsing and entity decoding, and pkgconf for discovering compiler and linker flags. The Makefile also supports system libraries through curl-config and xml2-config when pkg-config metadata is unavailable. On other platforms, install a C11 compiler, make, and development headers for ncurses, libcurl (7.85 or newer), and libxml2.

Switch tabs with Left/Right, `h`/`l`, or Tab/Shift-Tab. Navigation wraps around and the selected tab is highlighted. On narrow terminals the tab bar scrolls to keep the selected tab visible. Press `q`, Escape, or Ctrl-C to exit. Resizing redraws the UI; normal terminal settings are restored on exit.

Use `j`/`k` or Up/Down to select an item and `r` to reload. The selection is highlighted and the list scrolls to keep it visible. Titles use consecutive rows without blank spacing. Press Enter to open the selected article in your default browser. Ask HN posts open their HN page; comments open their permalink. Missing or unsupported links show an error without leaving the list. Each tab fetches its page when selected. Loading and request errors appear in the content area; keyboard navigation remains available while fetching. Requests use a 10-second connection timeout, a 20-second total timeout, and a 2 MiB response limit. TLS certificates are verified.

Use `make run` to build and launch, `make test` to run checks (Python 3 required), or `make clean` to remove build output. The fetching tests use a temporary localhost HTTP server. Keyboard regression checks use temporary terminal pairs to cover standard and application-mode arrows, modified arrows, Shift-Enter, and Escape.

## Mouse

In terminals that support SGR or legacy xterm mouse reporting, click a tab to switch feeds and click a title to select it. Double-click a title to open it in your default browser. Shift-click opens its discussion. The wheel moves the selection in the list and scrolls comment text in the detail view. Right-click inside a discussion returns to the list.

Mouse reporting is disabled when the app exits. Hold your terminal's mouse-selection modifier (often Shift) when you want to select and copy text instead of sending clicks to the app.

## Homebrew formula

`Formula/hckr.rb` builds the development version from the `main` branch of `https://github.com/kzzzld/hckr`. That repository must exist and contain the code before installation can work. There is no tagged release yet, so use `--HEAD`.

Once the repository is available, create a local tap and copy the formula into it:

```sh
brew tap-new kzzzld/local
cp Formula/hckr.rb "$(brew --repository kzzzld/local)/Formula/hckr.rb"
brew install --HEAD kzzzld/local/hckr
brew test kzzzld/local/hckr
```

Run the copy command from this checkout. The formula installs `hckr` into Homebrew's binary directory and an example `config.ini` into its shared-data directory. Your personal configuration is created on first launch. The formula test runs offline and verifies startup and configuration creation. On Linux, opening articles also requires the desktop's `xdg-open` command.

## Article details

Press Shift-Enter on a selected item to read its Hacker News discussion inside the app. `c` opens the same view in any terminal. Shift-Enter supports Kitty's enhanced keyboard protocol and the xterm modified-key sequence; terminals that send the same bytes for Enter and Shift-Enter require the `c` fallback or a terminal key mapping.

The detail view shows points, story text when available, and comments with author names and reply indentation. Separate upvote and downvote totals are not public; the UI labels them as unavailable. Comment totals describe the current page, and a notice identifies discussions with more pages available on Hacker News.

Scroll with `j`/`k` or Up/Down, reload with `r`, and press `q` or Escape to return to the original tab and selection. Returning during loading cancels the detail request. Enter still opens the selected article in your default browser. At the home list, `q` exits the app.

## Opening articles

Browser launching uses `open` on macOS, `xdg-open` on Linux and other Unix desktops, and `ShellExecuteW` on Windows. Linux requires xdg-utils and a configured default browser in a graphical desktop session. Browser URLs are passed directly, without a shell, and only HTTP/HTTPS links are accepted. Launcher errors appear in the footer; move the selection to clear them.

The browser adapter includes Windows support, but the existing app still uses POSIX terminal and configuration APIs. A full native Windows build and Windows/Linux desktop behavior have not been verified here.

## Configuration

Settings are loaded from `~/.config/hckr/config.ini`, or `$XDG_CONFIG_HOME/hckr/config.ini` when `XDG_CONFIG_HOME` is set to an absolute path. On startup, missing directories and `config.ini` are created automatically with built-in defaults. Existing files are preserved. Creation failures stop startup with a clear error. The file uses owner-only permissions (0600), and new directories use 0700.

```ini
[hackernews]
base_url = https://news.ycombinator.com

[ui]
initial_tab = new
```

An example is included in `config/config.ini`. Supported initial tabs are news, new, past, comments, ask, show, and jobs. Environment variable `HCKR_HN_URL` overrides the file's base URL. Precedence is environment, configuration file, then defaults.

Use unquoted `key = value` entries under the sections above. Blank lines and whole-line comments starting with `#` or `;` are accepted. Surrounding whitespace is ignored; inline comments and quoted values are not supported. Duplicate keys use the last value. Unknown sections or keys, empty values, and invalid initial tabs produce an error with the filename and line number. Base URLs may contain up to 2047 bytes. Each request uses this base URL plus the selected tab's relative path.

Configuration owns its strings and does not depend on the terminal UI.

## Layout

- `src/core`: shared configuration and UI-independent application logic.
- `src/core/http.c`: asynchronous HTTP requests and cancellation.
- `src/core/feed.c`: feed items and article/discussion links.
- `src/core/discussion.c`: points, story text, and threaded comment parsing.
- `src/core/html.c`: shared HTML text extraction.
- `src/core/sections.c`: shared tab labels and paths relative to the configured base URL.
- `src/tui`: ncurses rendering and keyboard input.
- `src/platform`: default-browser launching for each platform.
- `src/main.c`: application startup.

A future CLI can use `src/core` without linking ncurses.
