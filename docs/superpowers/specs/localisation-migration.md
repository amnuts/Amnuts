# Localisation Migration Ledger

| Phase | Status | Notes |
|-------|--------|-------|
| 1 — file-path mechanism + directory move | converted (2026-05-10) | All six relocated categories swept; libyaml vendored; locale_load_all wired into boot. Talker bootable at every commit. |
| 2 — catalog framework | converted (2026-05-11) | string catalog + lang_* API + set lang (USER) + langreload (WIZ); en_GB/strings.yml ships with meta.* and one smoke-test key; no in-source call sites use lang_* yet |
| 3 — UI builders | pending | rule/box/table + ui.* keys in strings.yml |
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

## Per-command sweep ledger (Phase 5)

To be filled in as Phase 5 commands are converted to lang_user().
