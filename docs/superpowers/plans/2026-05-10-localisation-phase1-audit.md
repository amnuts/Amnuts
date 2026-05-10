# Phase 1 Audit Ledger

For each call site, capture: file path:line, the function it's in, the operation
(`fopen` / `more` / `opendir` / `sprintf` building a path), and the conversion target
(`locale_path(user, ...)` for per-user lookup, `locale_default_path(...)` for boot/server-internal).

Definition lines in `src/includes/defines.h` (lines 51, 52, 54, 57, 58, 60) are the
macro definitions themselves and are deliberately excluded from this audit.

Two string-literal occurrences of `MOTDFILES` inside `perror`/`write_syslog` messages at
`src/amnuts.c:2402` and `src/amnuts.c:2404` are also excluded; they are diagnostic text,
not macro expansions, so they are not call sites.

Total: 51 call sites across 16 source files.

## ADMINFILES

- [ ] `src/commands/display.c:32` — `display_files` — `sprintf(filename, "%s/%s/%s", TEXTFILES, ADMINFILES, SHOWFILES)` then `more(user, ...)` → `locale_path(user, ...)` (per-user; file is displayed to `user`)
- [ ] `src/commands/display.c:61` — `display_files` — `sprintf(filename, "%s/%s/%s", TEXTFILES, ADMINFILES, word[1])` then `more(user, ...)` → `locale_path(user, ...)` (per-user; admin-prefixed text file shown to `user`)

## DATAFILES

- [ ] `src/amnuts.c:1035` — `load_and_parse_config` — `sprintf(filename, "%s/%s", DATAFILES, confile)` then `fopen(filename, "r")` → `locale_default_path(...)` (boot-time config load, no user context)
- [ ] `src/amnuts.c:1226` — `load_and_parse_config` — `sprintf(filename, "%s/%s.R", DATAFILES, rm1->name)` then `fopen(filename, "r")` → `locale_default_path(...)` (boot-time room description load)
- [ ] `src/amnuts.c:5706` — `exec_com` (case `MAP`) — `sprintf(filename, "%s/%s.map", DATAFILES, user->room->map)` then `more(user, user->socket, filename)` → `locale_path(user, ...)` (per-user; map shown to `user`)
- [ ] `src/admin.c:133` — `site_banned` — `sprintf(filename, "%s/%s", DATAFILES, NEWBAN)` then `fopen(filename, "r")` → `locale_default_path(...)` (shared ban state, server-internal lookup)
- [ ] `src/admin.c:135` — `site_banned` — `sprintf(filename, "%s/%s", DATAFILES, SITEBAN)` then `fopen(filename, "r")` → `locale_default_path(...)` (shared ban state)
- [ ] `src/admin.c:183` — `user_banned` — `sprintf(filename, "%s/%s", DATAFILES, USERBAN)` then `fopen(filename, "r")` → `locale_default_path(...)` (shared ban state)
- [ ] `src/admin.c:208` — `auto_ban_site` — `sprintf(filename, "%s/%s", DATAFILES, SITEBAN)` then `fopen(filename, "a")` → `locale_default_path(...)` (shared ban state, append)
- [ ] `src/admin.c:276` — `ban_site` — `sprintf(filename, "%s/%s", DATAFILES, SITEBAN)` then `fopen(filename, "r")` / `"a"` → `locale_default_path(...)` (shared ban state)
- [ ] `src/admin.c:327` — `ban_user` — `sprintf(filename, "%s/%s", DATAFILES, USERBAN)` then `fopen(filename, "r")` / `"a"` → `locale_default_path(...)` (shared ban state)
- [ ] `src/admin.c:420` — `ban_new` — `sprintf(filename, "%s/%s", DATAFILES, NEWBAN)` then `fopen(filename, "r")` / `"a"` → `locale_default_path(...)` (shared ban state)
- [ ] `src/admin.c:465` — `unban_site` — `sprintf(filename, "%s/%s", DATAFILES, SITEBAN)` then `fopen(filename, "r")` → `locale_default_path(...)` (shared ban state, rewrite)
- [ ] `src/admin.c:513` — `unban_user` — `sprintf(filename, "%s/%s", DATAFILES, USERBAN)` then `fopen(filename, "r")` → `locale_default_path(...)` (shared ban state, rewrite)
- [ ] `src/admin.c:565` — `unban_new` — `sprintf(filename, "%s/%s", DATAFILES, NEWBAN)` then `fopen(filename, "r")` → `locale_default_path(...)` (shared ban state, rewrite)
- [ ] `src/messages.c:757` — `read_board_specific` — `sprintf(filename, "%s/%s.B", DATAFILES, rm->name)` then `fopen(filename, "r")` → `locale_default_path(...)` (shared room board state)
- [ ] `src/messages.c:836` — `check_board_wipe` — `sprintf(filename, "%s/%s.B", DATAFILES, rm->name)` then `fopen(filename, "r")` → `locale_default_path(...)` (shared room board state)
- [ ] `src/messages.c:910` — `board_from` — `sprintf(filename, "%s/%s.B", DATAFILES, rm->name)` then `fopen(filename, "r")` → `locale_default_path(...)` (shared room board state)
- [ ] `src/commands/listbans.c:28` — `listbans` — `sprintf(filename, "%s/%s", DATAFILES, SITEBAN)` then `more(user, user->socket, filename)` → `locale_default_path(...)` (shared ban state displayed verbatim; not localisable content)
- [ ] `src/commands/listbans.c:41` — `listbans` — `sprintf(filename, "%s/%s", DATAFILES, USERBAN)` then `more(user, user->socket, filename)` → `locale_default_path(...)` (shared ban state)
- [ ] `src/commands/listbans.c:71` — `listbans` — `sprintf(filename, "%s/%s", DATAFILES, NEWBAN)` then `more(user, user->socket, filename)` → `locale_default_path(...)` (shared ban state)
- [ ] `src/commands/read_board.c:61` — `read_board` — `sprintf(filename, "%s/%s.B", DATAFILES, rm->name)` (filepos read) → `locale_default_path(...)` (shared room board state)
- [ ] `src/commands/recount.c:67` — `check_messages` — `sprintf(filename, "%s/%s.B", DATAFILES, rm->name)` then `fopen(filename, "r")` → `locale_default_path(...)` (shared room board state)
- [ ] `src/commands/reload_room.c:39` — `reload_room_description` — `sprintf(filename, "%s/%s.R", DATAFILES, rm->name)` then `fopen(filename, "r")` → `locale_default_path(...)` (reload of boot-time room descriptions; admin-initiated, not content shown to a specific locale)
- [ ] `src/commands/reload_room.c:91` — `reload_room_description` — `sprintf(filename, "%s/%s.R", DATAFILES, rm->name)` then `fopen(filename, "r")` → `locale_default_path(...)` (same, single-room path)
- [ ] `src/commands/search_boards.c:41` — `search_boards` — `sprintf(filename, "%s/%s.B", DATAFILES, rm->name)` then `fopen(filename, "r")` → `locale_default_path(...)` (shared room board state)
- [ ] `src/commands/wipe_board.c:61` — `wipe_board` — `sprintf(filename, "%s/%s.B", DATAFILES, rm->name)` → `locale_default_path(...)` (shared room board state, wipe)
- [ ] `src/commands/write_board.c:72` — `write_board` — `sprintf(filename, "%s/%s.B", DATAFILES, user->room->name)` then `fopen(filename, "a")` → `locale_default_path(...)` (shared room board state, append)

## HELPFILES

- [ ] `src/commands/help.c:83` — `help` — `sprintf(filename, "%s/%s", HELPFILES, com->name)` (subsequently used by `more(user, ...)`) → `locale_path(user, ...)` (per-user; help text shown to `user`)
- [ ] `src/commands/help.c:121` — `help` — `sprintf(filename, "%s/%s_%s", HELPFILES, com->name, attr->type)` (set attribute help) → `locale_path(user, ...)` (per-user)
- [ ] `src/commands/help.c:123` — `help` — `sprintf(filename, "%s/%s", HELPFILES, com->name)` (set fallback help) → `locale_path(user, ...)` (per-user)
- [ ] `src/commands/help.c:127` — `help` — `sprintf(filename, "%s/%s", HELPFILES, com->name)` (generic HELP fallback) → `locale_path(user, ...)` (per-user)

## MISCFILES

- [ ] `src/messages.c:28` — `count_suggestions` — `sprintf(filename, "%s/%s", MISCFILES, SUGBOARD)` then `fopen(filename, "r")` → `locale_default_path(...)` (boot-time count of shared suggestions board)
- [ ] `src/amnuts.c:4558` — `login` — `sprintf(filename, "%s/%s", MISCFILES, RULESFILE)` then `more(NULL, user->socket, filename)` → `locale_path(user, ...)` (per-user; rules shown to newly-created `user`)
- [ ] `src/amnuts.c:5584` — `exec_com` (case `NEWS`) — `sprintf(filename, "%s/%s", MISCFILES, NEWSFILE)` then `more(user, user->socket, filename)` → `locale_path(user, ...)` (per-user; news shown to `user`)
- [ ] `src/amnuts.c:5887` — `exec_com` (case `RULES`) — `sprintf(filename, "%s/%s", MISCFILES, RULESFILE)` then `more(user, user->socket, filename)` → `locale_path(user, ...)` (per-user)
- [ ] `src/amnuts.c:6118` — `exec_com` (case `WIZRULES`) — `sprintf(filename, "%s/%s", MISCFILES, WIZRULESFILE)` then `more(user, user->socket, filename)` → `locale_path(user, ...)` (per-user)
- [ ] `src/games.c:111` — `get_hang_word` — `sprintf(filename, "%s/%s", MISCFILES, HANGDICT)` then `count_lines(filename)` and word picking → `locale_default_path(...)` (shared dictionary used for game RNG; not user-displayed content)
- [ ] `src/commands/delete_suggestions.c:39` — `delete_suggestions` — `sprintf(filename, "%s/%s", MISCFILES, SUGBOARD)` then `remove(filename)` / rewrite → `locale_default_path(...)` (shared suggestions board, admin mutation)
- [ ] `src/commands/sfrom.c:31` — `suggestions_from` — `sprintf(filename, "%s/%s", MISCFILES, SUGBOARD)` then `fopen(filename, "r")` → `locale_default_path(...)` (shared suggestions board read; admin/info view of shared state)
- [ ] `src/commands/suggestions.c:27` — `suggestions` (RSUG branch) — `sprintf(filename, "%s/%s", MISCFILES, SUGBOARD)` then `more(user, user->socket, filename)` → `locale_default_path(...)` (shared suggestions board content; the data is user-written, not localised)
- [ ] `src/commands/suggestions.c:63` — `suggestions` (write branch) — `sprintf(filename, "%s/%s", MISCFILES, SUGBOARD)` then `fopen(filename, "a")` → `locale_default_path(...)` (shared suggestions board append)

## MOTDFILES

- [ ] `src/messages.c:66` — `count_motds` — `sprintf(filename, "%s/motd%d", MOTDFILES, i)` then `opendir(filename)` → `locale_default_path(...)` (boot/runtime enumeration of motd directory; no user in scope)
- [ ] `src/amnuts.c:822` — `accept_connection` — `sprintf(motdname, "%s/motd1/motd%d", MOTDFILES, get_motd_num(1))` then `more(NULL, accept_sock, motdname)` → `locale_default_path(...)` (pre-auth; no user, locale unknown — must use default)
- [ ] `src/amnuts.c:4590` — `login` — `sprintf(motdname, "%s/motd2/motd%d", MOTDFILES, get_motd_num(2))` then `more(user, user->socket, motdname)` → `locale_path(user, ...)` (per-user; post-login motd2 for new user)
- [ ] `src/amnuts.c:4629` — `login` — `sprintf(motdname, "%s/motd2/motd%d", MOTDFILES, get_motd_num(2))` then `more(user, user->socket, motdname)` → `locale_path(user, ...)` (per-user; post-login motd2 for returning user)

## TEXTFILES

- [ ] `src/commands/display.c:30` — `display_files` — `sprintf(filename, "%s/%s", TEXTFILES, SHOWFILES)` then `more(user, ...)` → `locale_path(user, ...)` (per-user; index of displayable text files)
- [ ] `src/commands/display.c:32` — `display_files` — `sprintf(filename, "%s/%s/%s", TEXTFILES, ADMINFILES, SHOWFILES)` then `more(user, ...)` → `locale_path(user, ...)` (per-user; admin index — same entry as ADMINFILES section above, listed here for the `TEXTFILES` token)
- [ ] `src/commands/display.c:59` — `display_files` — `sprintf(filename, "%s/%s", TEXTFILES, word[1])` then `more(user, ...)` → `locale_path(user, ...)` (per-user; arbitrary user-requested text file)
- [ ] `src/commands/display.c:61` — `display_files` — `sprintf(filename, "%s/%s/%s", TEXTFILES, ADMINFILES, word[1])` then `more(user, ...)` → `locale_path(user, ...)` (per-user; admin-prefixed text file — same entry as ADMINFILES section above, listed here for the `TEXTFILES` token)

## Notes / anomalies

- `src/messages.c:66` (`count_motds`) is the only `opendir(DATAFILES/MISCFILES/MOTDFILES/...)`-style call in the audited set. All other uses are `sprintf` + `fopen`/`more`/`remove`. The localisation helpers must therefore continue to yield a directory path (not just a file path) suitable for `opendir`.
- `src/commands/display.c:32` and `src/commands/display.c:61` reference *two* of the six constants on a single line (`TEXTFILES` and `ADMINFILES`). They are listed in both the `ADMINFILES` and `TEXTFILES` sections; the sweep should produce a single edit per line that resolves both constants together.
- `src/amnuts.c:2402` and `src/amnuts.c:2404` contain the literal string "MOTDFILES" inside a `perror`/`write_syslog` message. The preprocessor does not expand macros inside string literals, so these are diagnostic text, not call sites. They should be left alone (or, optionally, retargeted as part of the eventual logging/i18n pass — out of scope for Phase 1).
- Board files (`.B`) and room description files (`.R`) under `DATAFILES` are shared mutable state, not display content; they are routed to `locale_default_path(...)`. If a future phase decides to localise per-room descriptions, the `.R` reads at `src/amnuts.c:1226` and `src/commands/reload_room.c:39,91` are the lines to revisit.
- The `RULESFILE` read at `src/amnuts.c:4558` happens while `user` exists but its locale may still be the default (the user is in the middle of registration). `locale_path(user, ...)` is still correct because the helper is expected to fall back to the default when the user's locale is unset.
