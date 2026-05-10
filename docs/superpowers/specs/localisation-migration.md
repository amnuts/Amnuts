# Localisation Migration Ledger

| Phase | Status | Notes |
|-------|--------|-------|
| 1 — file-path mechanism + directory move | converted (2026-05-10) | All six relocated categories swept; libyaml vendored; locale_load_all wired into boot. Talker bootable at every commit. |
| 2 — catalog framework | pending | string catalog + lang_* API + set lang + langreload |
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

## Per-command sweep ledger (Phase 5)

To be filled in as Phase 5 commands are converted to lang_user().
