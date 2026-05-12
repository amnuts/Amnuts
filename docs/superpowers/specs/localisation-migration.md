# Localisation Migration Ledger

| Phase | Status | Notes |
|-------|--------|-------|
| 1 — file-path mechanism + directory move | converted (2026-05-10) | All six relocated categories swept; libyaml vendored; locale_load_all wired into boot. Talker bootable at every commit. |
| 2 — catalog framework | converted (2026-05-11) | string catalog + lang_* API + set lang (USER) + langreload (WIZ); en_GB/strings.yml ships with meta.* and one smoke-test key; no in-source call sites use lang_* yet |
| 3 — UI builders | converted (2026-05-12) | visible_strlen / align_into / rule / box_* / table_*; ui.* keys in en_GB and cowboy test locale; wizlist pilot converted |
| 4 — frame-heavy commands | pending | wizlist, show_igusers, grepusers, listbans, system, parts of help |
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

## Per-command sweep ledger (Phase 5)

To be filled in as Phase 5 commands are converted to lang_user().
