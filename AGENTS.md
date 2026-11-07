# Project rules

- Build a Hacker News terminal app in C. Keep the current scope small; do not add news fetching or other features until requested.
- Keep shared application logic in `src/core`. It must not depend on ncurses, terminal state, or UI code so a future CLI can reuse it.
- Keep terminal rendering and input in `src/tui`. Keep `main.c` focused on startup and shutdown.
- Put changeable settings in configuration, including the Hacker News base URL. Do not duplicate defaults or embed service URLs in UI or request code.
- Use small modules and explicit ownership. Check failures and release resources on every exit path.
- Add dependencies only when they simplify implementation or maintenance. Record required Homebrew packages in `Brewfile` and explain their purpose in `README.md`.
- Prefer straightforward C over frameworks, speculative abstractions, or clever macros. Do not introduce global mutable application state unless required for signal handling.
- No emoji comments. Omit obvious comments. Use short summaries where sufficient; explain genuinely confusing logic in detail.
- Keep builds warning-free. Run `make` and appropriate checks after changes. For terminal changes, verify startup, resize, exit, and terminal restoration when possible.
- Update documentation when commands, dependencies, configuration, or behavior change. Do not claim checks passed unless they ran.
- After completing work and running the required checks, commit all pending project changes automatically without asking for confirmation, unless the user explicitly requests otherwise. Review the staged changes, keep generated files and secrets out of commits, and report the commit hash and any remaining changes. Do not push unless requested.
