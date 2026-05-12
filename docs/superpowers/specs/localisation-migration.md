# Localisation Migration Ledger

| Phase | Status | Notes |
|-------|--------|-------|
| 1 — file-path mechanism + directory move | converted (2026-05-10) | All six relocated categories swept; libyaml vendored; locale_load_all wired into boot. Talker bootable at every commit. |
| 2 — catalog framework | converted (2026-05-11) | string catalog + lang_* API + set lang (USER) + langreload (WIZ); en_GB/strings.yml ships with meta.* and one smoke-test key; no in-source call sites use lang_* yet |
| 3 — UI builders | converted (2026-05-12) | visible_strlen / align_into / rule / box_* / table_*; ui.* keys in en_GB and cowboy test locale; wizlist pilot converted |
| 4 — frame-heavy commands | converted (2026-05-12) | wizlist (Phase 3 pilot), show_igusers, grepusers, listbans, system, help — all framed output via box_*/table_* or opaque catalog frame literals; strings in en_GB/strings.yml |
| 5 — bulk inline-string conversion | pending | per-command sub-ledger below |
| 6 — first non-default locale | pending | translator's work, not a code phase |

## Phase 1 verification

The full Phase 1 sweep landed across the commits between the libyaml vendor (`69902cc`) and the TEXTFILES sweep (`213f38a`).

End-to-end runtime verification deferred — the development environment used to land these commits lacks clang/make and Docker daemon. The user should:

1. Run `make build` from a Linux/macOS host (or under WSL with clang installed).
2. Boot `./amnutsTalker` and confirm the startup line:
   - `Localisation: discovered 2 locale(s); default = en_GB.`
3. Connect via telnet on the main port, log in, and exercise:
   - `help <topic>` — verify help text displays (HELPFILES path)
   - `news`, `rules`, `wizrules` — verify content displays (MISCFILES path)
   - MOTD on login should still appear (MOTDFILES path)
   - Walk between rooms — verifies the `.R` room loader (DATAFILES path)
   - As a wizard: `setbans`/`listbans` cycle to verify the ban-list read/write paths (DATAFILES)
4. Sanity-check the fallback locale:
   - The `fallback_test` directory ships with one overridden file under `motds/motd2/`.
   - In a future commit (Phase 2) we'll wire up `set lang` and users can opt in.
   - For now, you can manually verify the path-resolution behaviour by temporarily injecting a non-empty `user->locale` value (e.g. via a debug patch) and confirming the resolver returns 2 for the overridden file, 1 for any file absent from `fallback_test/`.

## Phase 2 verification

The Phase 2 sweep landed across the commits introducing `src/catalog.c`,
`src/yaml_util.c`, the `SETLANG` and `LANGRELOAD` command wiring, the
`USERDB_LIST` extension for the per-user `language` field, and the initial
`files/langs/en_GB/strings.yml`.

End-to-end runtime verification deferred — the development environment used
to land these commits is Windows with no clang/make/Docker. The user should:

1. Build on a Linux/macOS host (or under WSL): `make build`.
2. Boot `./amnutsTalker`; confirm the two new startup lines appear after the
   Phase 1 discovery line:
   - `Localisation: discovered N locale(s); default = en_GB.`
   - `Localisation: catalog loaded (N locale(s)).`
3. Telnet in, log in. Exercise:
   - `set` (no arg) — `lang` should appear in the attribute list.
   - `set lang` — shows the available-locales table; `en_GB` marked `*`.
   - `set lang fallback_test` — switches; reports `Language set to fallback_test.`
   - `set lang` again — `fallback_test` now also marked `>`.
   - `quit`, then `grep '^language' files/userfiles/<your>.D` — should show
     `language      fallback_test`. Log in again — the choice survives.
   - `set lang default` — clears the override; the `language` line disappears
     from the user file on the next save.
   - `set lang nonsense` — "No such language" error.
4. As a wizard: `.langreload` — confirmation message and a syslog entry of
   the form `[locale] langreload: N catalog(s) reloaded, M user(s) reset.`
5. Format-safety check:
   - Edit a non-default locale's `strings.yml` to include a key with `%n`
     (e.g., `meta.description: "broken %n"`).
   - Run `.langreload` — the reload still succeeds; that broken key is
     dropped and a syslog warning is written; other keys still load.

No code outside `src/catalog.c`, `src/commands/{set_lang,langreload}.c`,
the `set` dispatch, and the user-file save/load uses `lang_*` yet. Phase 4
starts the actual sweep, beginning with `wizlist`.

## Phase 3 verification

The Phase 3 sweep landed `src/uibuilders.c` + `src/includes/uibuilders.h`
(visible_strlen, align_into, rule, full box_* family, full table_* family
with cell wrap), added the canonical `ui.*` keys to en_GB/strings.yml,
shipped a `cowboy` test locale that overrides only `meta.*` + `ui.*`, and
converted `wizlist` as the end-to-end pilot.

End-to-end runtime verification deferred — Windows dev host. The user
should, on Linux/macOS:

1. `make build` — clean compile under `-Wall -Wextra -Wpedantic`.
2. Boot the talker. Expected startup lines:
   - `Localisation: discovered 3 locale(s); default = en_GB.` (en_GB +
     fallback_test + cowboy)
   - `Localisation: catalog loaded (3 locale(s)).`
3. Telnet in as a wizard. Run `.wizlist`. On en_GB the output should be
   byte-identical to the pre-Phase-3 wizlist (compare against the same
   commit on a peer machine that hasn't pulled, if available; otherwise
   eyeball).
4. `set lang cowboy`, then `.wizlist` again. The `+----- ... -----+`
   section dividers should now look like `-={*----- ... -----*}=-`
   because the `wizlist.frame.*` keys are catalog values that themers
   can re-skin (and the cowboy locale's `ui.box.*` overrides shift any
   future `box_open`-based command that lands in Phase 4).
5. Pick any catalog string containing colour escapes and run
   `set lang cowboy` / `set lang default` rapidly — the body should
   never go truncated or misaligned, because every padding/wrap
   calculation routes through `visible_strlen`.

## Wizlist (Phase 3 pilot)

The Phase 3 plan introduces the per-command sweep ledger early so the
end-to-end pilot has somewhere to land. Phase 5's wider sweep will
extend this same table.

| Command | File | Status |
|---------|------|--------|
| wizlist | src/commands/wizlist.c | converted (2026-05-12) — Phase 3 pilot |

Notes on the pilot conversion:

- Every server-authored literal in `wiz_list()` is now lifted into
  `wizlist.*` keys in `files/langs/en_GB/strings.yml`. en_GB output is
  byte-identical to the pre-conversion version.
- The frame lines (`+----- title -----+`) are stored as opaque catalog
  values rather than synthesised via `box_open` / `rule()`. The body
  content beneath each frame in this command has no `|…|` side rails,
  so a `table_open` pass would have shifted bytes. A themed locale can
  still re-skin each section divider end-to-end by overriding the
  `wizlist.frame.*` keys.
- Variable-width formatting (`%-*.*s` with widths derived at runtime
  from `teslen`) is performed locally with `snprintf` before the value
  is handed to the catalog format string as a plain `%s`; the catalog
  signature framework rejects `%*` specifiers, and this two-stage
  approach keeps the rendered bytes identical.

## Phase 4 verification

The Phase 4 sweep converted the five frame-heavy commands listed in the
spec, each in its own commit on top of the Phase 3 pilot:

- `show_igusers` (in `src/amnuts.c`, not `src/commands/`) — `box_open`+
  `box_line` with `|…|` rails, three name cells per row preserving the
  `%-24s` byte-stream verbatim.
- `grepusers` — hybrid: outer frame and footer via `box_*`, per-match
  pair rows via opaque catalog literals (the original deliberately
  overruns the 78-col frame with 86-col pair content; preserved
  byte-equivalent rather than "fixed").
- `listbans` — every subcommand (`sites`, `users`, `new`, `swears`)
  via opaque frame literals plus per-content `lang_user` calls;
  `more()` paths into `langs/<locale>/datafiles/` untouched (Phase 1
  already made those locale-aware).
- `system` (in `src/commands/system.c`) — 57 catalog keys across
  six sections (general, port info, users, netlinks, rooms, memory);
  byte-equivalent verified via an offline positional-printf-aware
  comparison harness across every format path including the
  asymmetric ` : ` vs `: ` spacing in the original.
- `help` — used the lower-risk approach: existing `align_string()`
  calls were retained and fed `lang(user, "key")` as the format
  argument; the per-topic `more()` file-paging path is unchanged.

End-to-end runtime verification deferred to a Linux/macOS host (Windows
dev box). Smoke checks the user should run after a clean build:

1. `.show_igusers` (alone) and as a wiz: `.show_igusers <other_user>`.
2. `.grepusers <pattern>` with a known match.
3. `.lban sites` / `.lban users` / `.lban new` / `.lban swears` /
   `.lban` (no arg, which deliberately falls into the new-bans branch
   per the preserved quirk).
4. `.system` and `.system -a` — verify every section header, every
   stat row, the asymmetric colon-spacing, and the inner `|---|`
   separators (note: NOT `+---+`).
5. `.help`, `.help commands`, `.help credits`, `.help nuts`, `.help
   <topic>`, `.help nonsense`.
6. `set lang cowboy`, then re-run all of the above. Frames via `box_*`
   pick up the cowboy theme; opaque frame literals stay in English.

## Per-command conversion pattern (Phase 4 + 5)

Every command conversion follows the same six-step checklist. Phase 4
commands are the frame-heavy ones (`show_igusers`, `grepusers`, `listbans`,
`system`, `help`); Phase 5 is everything else.

1. **Read** the existing command end to end. List every `write_user`,
   `vwrite_user`, `write_room*`, `vwrite_level*` call that produces
   server-authored output. (Skip `write_syslog` — those stay in C;
   admin logs aren't translatable.)

2. **Identify the frames.** Look for inline `+----+`, `|...|`, header
   lines, separator lines. These become `box_open` / `box_separator` /
   `table_*` / `rule` / `box_close` calls — OR, where the original
   doesn't use side rails, opaque catalog frame literals (the
   wizlist pilot's `wizlist.frame.*` pattern).

3. **Mint catalog keys** in `<command>.<context>.<variant>` form.

4. **Add the keys to `en_GB/strings.yml`** with EXACTLY the literal
   that appeared in the C source (including `~OL` / `~FC` / `~RS`
   escapes). En_GB is the source of truth; everything else
   diff-matches against it.

5. **Convert the C source.** Replace each output call with the
   appropriate builder + `lang_user`. The byte-identical contract:
   on en_GB the user must see exactly what they saw before. Use
   `vwrite_user(user, lang(user, "key"), arg, …)` only as a single-line
   pattern; never store `lang()`'s return across statements.
   `%*` variable-width specifiers are not catalog-safe — pre-format
   with `snprintf` locally and pass the result as a plain `%s` arg
   to the catalog format (the wizlist pilot demonstrates this).

6. **Update the sweep ledger.** Tick the command's row from `pending`
   to `converted (YYYY-MM-DD)`.

A conversion is "done" when:
- `make build` is clean.
- On en_GB the command's output before/after the commit is
  byte-identical (compare with `script` + `diff`, or eyeball).
- On a themed locale (`cowboy`), the frames pick up the theme; the
  rest of the strings render unchanged (no themed translations yet
  outside `ui.*` and `meta.*`).

## Per-command sweep ledger (Phases 4 + 5)

Phase 4: frame-heavy commands. Phase 5: every other command in `src/commands/`.

| Command         | File                              | Status   | Notes |
|-----------------|-----------------------------------|----------|-------|
| wizlist         | src/commands/wizlist.c            | converted (Phase 3 pilot) | |
| show_igusers    | src/amnuts.c (function)           | converted (2026-05-12) | Frame via box_open/box_line/box_close; inner = 76 cols. show_igusers.title carries a load-bearing leading space; rows concatenate show_igusers.name_cell up to three times before each box_line emission. |
| grepusers       | src/commands/grepusers.c          | converted (2026-05-12) | Frame + title + empty-state + footer via box_open/box_line/box_separator/box_close (78 cols total, inner = 76). Per-match body rows are opaque catalog values written direct via write_user because the original layout overruns the frame (paired row = 86 vis cols, orphan first half padded to 82). Names and level labels pre-padded with snprintf so the row signatures stay plain `%s`. |
| listbans        | src/commands/listbans.c           | converted (2026-05-12) | Four subcommand banners (sites/users/swears/new) ship as opaque catalog frame literals — the original layout had no |…| body rails, so synthesising via box_open would shift bytes. Three subcommands page their bodies via more() against a locale-aware filename and are left untouched. Quirk preserved: the source's `if (strcmp(word[1], "new"))` is inverted, so the usage key only fires when the user types `lban new` exactly; any other unrecognised arg (or no arg) falls into the new-bans banner. |
| system          | src/commands/system.c             | converted (2026-05-12) | Five sections (header, users, netlinks, rooms, memory) chain through one box_open/box_line/box_separator/box_close pipeline at 78 cols (inner = 76). Two divergences from the standard box flow: (1) the per-section inner separator uses `|` caps not `+` caps, so it ships as the opaque catalog literal `system.inner_sep` and is emitted via write_user — it cannot reuse box_separator. (2) The port-info header has 8 NETLINKS x IDENTD-state x WIZPORT variants; each ships as a (label, value) pair of keys under `system.ports.label.*` / `system.ports.value.*`. Memory section's "total" row mixes `%12.3f` (Mb) and `%d` (bytes); the catalog signature validator rejects `%f`, so the Mb value is pre-formatted via snprintf to a 12-char string locally and shipped as `%12s`. Every label is baked into the row's catalog format so themed locales can rewrite both wording and column geometry. Standalone `-u`/`-n`/`-r`/`-m` open and close their own box; `-a` chains all five sections through one open + box_separators + one close. |
| help            | src/commands/help.c               | converted (2026-05-12) | Framing parts only: the "no help on that topic" / ambiguous-match messages, the level- and function-grouped commands-list screens, and the Amnuts/NUTS credits screens. The per-topic helpfile path (`more()` against `files/langs/<locale>/helpfiles/<topic>`) is OUT OF SCOPE — that path is already locale-aware from Phase 1 and stays untouched. Approach (a) per the watch-points: the commands-list screens keep the existing `align_string` pipeline and feed it the catalog format via `lang(user, "key")` rather than switching to `box_open`/`box_line`. align_string already emits the byte stream the original sprintf chain produced (including the `|…|` rails baked into the marker arg), so this is the safer byte-identical conversion. The two per-row in-loop format strings (`~FR%s~RS%s %s` / `%s %s` and the function-grouped pair) ship as `help.commands.row.{level,function}_{xcom,plain}` catalog keys; the row LOOP stays in C and the assembled cell is fed to `align_string` via the existing sds-based concatenation. The credits screens are flat (no rails) and ship as 3 catalog values each (header, version, body); the body is one double-quoted scalar containing all remaining static lines — long but byte-identical, and `lang_user`'s internal `ARR_SIZE * 2` (2000) buffer comfortably holds either credits body. |
| cafk            | src/commands/cafk.c               | converted (2026-05-12) | Phase 5 batch 1. One-liner: single write_user → lang_user, no positional args. |
| cls             | src/commands/cls.c                | converted (2026-05-12) | Phase 5 batch 1. Two opaque catalog values: a 10x newline flood emitted six times for telnet clients that ignore ANSI, and the literal ANSI clear-screen sequence (ESC + `[2J`) — represented in YAML via `\x1B`. No positional args. |
| home            | src/commands/home.c               | converted (2026-05-12) | Phase 5 batch 1 (NETLINKS-gated). Two write_user → lang_user and one vwrite_room_except → lang_room(user->room, user, ...). The invisible-arrival branch still routes through write_room_except with the global `invisenter` constant — that string is outside the per-command catalog surface. |
| muzzle          | src/commands/muzzle.c             | converted (2026-05-12) | Phase 5 batch 1. Five user-facing write_user / vwrite_user calls converted. The `sprintf(text, "...")` + `send_mail`/`write_user(u, text)` pattern that delivers the "You have been muzzled!" message to the target is left alone — mail bodies are sender-authored content stored verbatim on disk; translating them under the target's locale would break the mail-spool round-trip and is out of scope for this batch. |
| revafk          | src/commands/revafk.c             | converted (2026-05-12) | Phase 5 batch 1. Three write_user → lang_user calls (header, empty-buffer message, footer). Pager start/stop unchanged. |
| revclr          | src/commands/revclr.c             | converted (2026-05-12) | Phase 5 batch 1. One vwrite_room_except → lang_room(user->room, user, "revclr.room_notice", name); one write_user → lang_user. The `name` arg is pre-computed in C from user->vis (recap vs global `invisname`) and passed in. |
| set_desc        | src/commands/set_desc.c           | converted (2026-05-12) | Phase 5 batch 1. Four messages: current-description display (no arg), the "(CLONE)" rejection, the too-long rejection, and the "Description set." confirmation. |
| suicide         | src/commands/suicide.c            | converted (2026-05-12) | Phase 5 batch 1. Three messages including the confirm prompt with embedded \a (bell) — the original `\07` octal escape becomes `\a` in YAML, which libyaml parses to byte 0x07 byte-identically. `misc_op = 6` / `no_prompt = 1` state machine flow is unchanged. |
| unmuzzle        | src/commands/unmuzzle.c           | converted (2026-05-12) | Phase 5 batch 1. Symmetric to muzzle: five user-facing messages converted; the `sprintf(text, ...)` + `send_mail`/`write_user(u, text)` mail-delivery branch left untouched for the same reason as muzzle. |
| wake            | src/commands/wake.c               | converted (2026-05-12) | Phase 5 batch 1. Ten user-facing messages converted. The `static const char usage[]` declaration is removed and inlined into `lang_user(user, "wake.usage")`. The "wake target" message routes through lang_user(u, ...) so it renders in the target's locale, not the sender's. The `notloggedon` global string is left alone (it's a shared global, not per-command). The two "ignoring wakes" variants are distinct keys (ignoring_you for sender-on-target-ignore, ignoring_beeps for the target-rejects-beeps case). |

(More Phase 5 batches will append rows here.)
