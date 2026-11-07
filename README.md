# hckr

A Hacker News terminal app written in C, with keyboard and mouse navigation, threaded discussions, and INI color themes. Browse news, new, past, comments, ask, show, and jobs; open articles in your default browser or read their discussions in the terminal.

## Build and run

On macOS, install the Xcode Command Line Tools (`xcode-select --install`), then:

```sh
git clone https://github.com/kzzzld/hckr.git
cd hckr
brew bundle
make
./build/hckr
```

`Brewfile` includes ncurses for terminal rendering, libcurl for HTTP, libxml2 for HTML parsing and entity decoding, and pkgconf for discovering compiler and linker flags. The Makefile also supports system libraries through curl-config and xml2-config when pkg-config metadata is unavailable. On other platforms, install a C11 compiler, make, and development headers for ncurses, libcurl (7.85 or newer), and libxml2.

Switch tabs with Left/Right, `h`/`l`, or Tab/Shift-Tab. Navigation wraps around and the selected tab is highlighted. On narrow terminals the tab bar scrolls to keep the selected tab visible. Press `q`, Escape, or Ctrl-C to exit. Resizing redraws the UI; normal terminal settings are restored on exit.

Use `j`/`k` or Up/Down to select an item and `r` to reload. The selection is highlighted and the list scrolls to keep it visible. Each article shows its title followed by an indented points-and-comments line, with one blank row between articles. Unavailable counts are labelled instead of guessed. The Comments feed keeps its compact text previews. Press `c` to read text written directly in the Hacker News post, or Enter to open the selected article in your default browser. Ask HN posts open their HN page; comments open their permalink. Missing or unsupported links show an error without leaving the list. Each tab fetches its page when selected. Loading and request errors appear in the content area; keyboard navigation remains available while fetching. Requests use a 10-second connection timeout, a 20-second total timeout, and a 2 MiB response limit. TLS certificates are verified.

Use `make run` to build and launch, `make test` to run checks (Python 3 required), or `make clean` to remove build output. The fetching tests use a temporary localhost HTTP server. Keyboard regression checks use temporary terminal pairs to cover standard and application-mode arrows, modified arrows, Shift-Enter, and Escape.

## Mouse

In terminals that support SGR or legacy xterm mouse reporting, hover over tabs, titles, discussion text, or footer hints to underline them. Hover does not change your selected article. Click a tab to switch feeds and click a title or its statistics line to select it. Double-click a title to open it in your default browser. Shift-click opens its discussion. The wheel moves the selection in article and comment lists. Click a comment to select it, or double-click to open its permalink. Right-click inside a discussion returns to the list.

Mouse reporting is disabled when the app exits. Hold your terminal's mouse-selection modifier (often Shift) when you want to select and copy text instead of sending clicks to the app.

## Homebrew

Install the development version from the [remote tap](https://github.com/kzzzld/tap):

```sh
brew tap kzzzld/tap https://github.com/kzzzld/tap
brew install --HEAD kzzzld/tap/hckr
brew test kzzzld/tap/hckr
```

If Homebrew asks you to trust the formula, run `brew trust --formula kzzzld/tap/hckr`, then retry installation.

No local checkout or formula copying is required. There is no tagged release yet, so use `--HEAD` to build from `main`. To update later:

```sh
brew update
brew upgrade --fetch-HEAD kzzzld/tap/hckr
```

The formula installs `hckr` and its INI themes. Your personal configuration is created on first launch. The formula test runs offline and verifies startup, configuration creation, and bundled theme loading. On Linux, opening articles also requires the desktop's `xdg-open` command. The formula source is also kept in `Formula/hckr.rb` in this repository.

## Post text

Press `c` on a selected post to read its Hacker News text in a focused view, with paragraphs preserved. This shows text from Ask HN, Show HN, and other self-posts; it does not fetch the linked external article. Link-only posts show a message when no HN text is available.

Scroll with `j`/`k`, Up/Down, the wheel, or Page Up/Down. Enter opens the article in your browser, `d` switches to comments, and `q` or Escape returns to the list. Use `r` to reload the post. Within comments, `c` switches back to post text.

## Article details

Press Shift-Enter on a selected item to read its Hacker News discussion inside the app. `d` opens the same view in any terminal. Shift-Enter supports Kitty's enhanced keyboard protocol and the xterm modified-key sequence; terminals that send the same bytes for Enter and Shift-Enter require the `d` fallback or a terminal key mapping.

The detail view shows points, story text when available, and comments with author names and reply indentation. Separate upvote and downvote totals are not public; the UI labels them as unavailable. Comment totals describe the current page, and a notice identifies discussions with more pages available on Hacker News.

Select comments with `j`/`k`, Up/Down, the mouse wheel, or a click. The selected comment is highlighted and kept visible. Page Up/Down scroll through long comments without changing selection. Reload with `r`, and press `q` or Escape to return to the original tab and article selection. Returning during loading cancels the detail request. Enter or a double-click opens the selected comment’s permalink in your default browser; if there are no comments, Enter opens the article. At the home list, `q` exits the app.

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
; foreground = white
; background = black
```

An example is included in `config/config.ini`. Supported initial tabs are news, new, past, comments, ask, show, and jobs. Environment variable `HCKR_HN_URL` overrides the file's base URL. Precedence is environment, configuration file, then defaults.

Use unquoted `key = value` entries under the sections above. Blank lines and whole-line comments starting with `#` or `;` are accepted. Surrounding whitespace is ignored; inline comments and quoted values are not supported. Duplicate keys use the last value. Unknown sections or keys, empty values, and invalid initial tabs produce an error with the filename and line number. Base URLs may contain up to 2047 bytes. Each request uses this base URL plus the selected tab's relative path.

Choose a theme with `theme = nord` under `[ui]`. Available themes are `default`, `nord`, `catppuccin` (Mocha), `catppuccin-mocha`, `catppuccin-latte`, `catppuccin-frappe`, and `catppuccin-macchiato`.

```ini
[ui]
theme = nord
; Override either color independently:
foreground = #FFFFFF
; background = default
```

Explicit `foreground` and `background` settings override the theme regardless of their order in the file. Omit an override to inherit the theme color; setting it to `default` specifically restores the terminal's color. Values accept the eight standard names, palette indexes `0`–`255`, and unquoted `#RRGGBB` hex colors. Hex colors use dedicated RGB palette entries when the terminal supports redefining colors, preserving the theme’s colors. These entries are restored on exit. Terminals with fixed palettes use the nearest available color instead. Unsupported palette indexes produce a startup error. Restart after editing settings.

To add a scheme, create `themes/<name>.ini` containing `[ui]`, `foreground`, and `background`, then set `theme = <name>`. No C changes or rebuild are needed. Keep the commented choices in `config/config.ini` current. Theme files cannot include other themes or non-color settings.

Local builds read themes from this checkout's `themes/` directory. Homebrew installs them in its shared-data directory. Set `HCKR_THEME_DIR` to use another directory, or set `THEME_DIR` when building to change the installed lookup path. Missing or malformed themes report their path and line number where applicable. Existing personal config files are preserved.

Palette references: [Nord](https://www.nordtheme.com/docs/colors-and-palettes/) and [Catppuccin](https://github.com/catppuccin/catppuccin).

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
