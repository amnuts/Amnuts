# Localisation Phase 6 — First Non-Default Locale (Translator Runbook)

> **Note:** This is NOT a code plan. The design spec §9.1 explicitly says Phase 6 is "translator's job, not a code phase." This document is a runbook a translator (or an operator coordinating with one) follows to ship a real non-default locale once the code phases (1–5) are done.

**Goal:** Ship one usable non-default locale (`fr` is the conventional example; pick whichever language a real translator volunteers for) so the end-to-end system is demonstrated in production. Phase 6 is "done" when at least one user can run `set lang fr` (or whichever) and use the talker comfortably in that language for the swept-and-translated subset of commands.

**Architecture:** Zero C-side changes. Add a directory `files/langs/<locale>/` with at minimum a `strings.yml`. Optionally translate the `helpfiles/`, `motds/`, `textfiles/`, `miscfiles/`, `datafiles/`, `adminfiles/` content directories — anything left unset falls back to en_GB per Phase 1's `locale_path` resolver.

**Reference spec:** `docs/superpowers/specs/2026-05-10-localisation-design.md` §3 (fallback semantics), §9.1 (Phase 6), §11 (test plan).

**Prerequisites:** Phases 1–5 landed; the talker boots cleanly with `en_GB` and at least the smoke-test `cowboy` locale; `tools/locale/check.py` works.

---

## Step 1: Pick a locale identifier

The directory name IS the locale identifier. Conventions:

- ISO 639-1 language code only: `fr`, `de`, `es`, `pt`.
- ISO 639-1 + ISO 3166-1 region: `fr_FR`, `pt_BR`, `de_AT`.
- Hyphen variants: `de-DE`, `pt-BR`.

The talker treats all forms as opaque directory names. Pick a convention and stick with it. The existing `en_GB` uses underscore-then-region; copy that style for consistency unless you have a reason not to.

`LOCALE_NAME_LEN` (defined in `src/includes/defines.h`) caps the directory name at 15 chars (the 16th byte is the NUL terminator). Anything longer is rejected at discovery.

---

## Step 2: Create the locale directory

```bash
mkdir -p files/langs/<locale>
cp files/langs/en_GB/strings.yml files/langs/<locale>/strings.yml
```

Copying the en_GB file is the recommended starting point — every key is present, every value is the English literal that needs translating. Delete keys you can't translate yet; they'll fall back automatically.

Edit `files/langs/<locale>/strings.yml`:

1. Update `meta.name` and `meta.description` to describe the locale.
2. Translate each value, leaving keys untouched.
3. Preserve `%1$s` / `%2$d` positional markers exactly. You may reorder them in the translation: `"Hello %1$s, you have %2$d messages"` → `"Vous avez %2$d messages, %1$s"`.
4. Preserve `~XX` colour escapes verbatim. Translators can drop them (`"~OL%1$s~RS"` → `"%1$s"`) or change them, but the talker treats them as opaque markers — what's between two escapes is always uncoloured.
5. Block scalars (`|`) preserve newlines for multi-line bodies; leave the indentation as-is.

---

## Step 3: Validate the strings.yml

Run the translator-side validator before any commit:

```bash
python tools/locale/check.py files/langs/<locale>/strings.yml
```

Expected output: `OK: files/langs/<locale>/strings.yml validates clean.`

If any keys are reported as drops:

- **`%n disallowed`** — remove the `%n` from your translation.
- **`argument position out of range`** — you used `%9$s` or higher; the catalog limit is 8.
- **`mixed positional and implicit specifiers`** — use either `%s %s` or `%1$s %2$s`, not both.
- **`uses position N that default omits`** — your translation refers to an argument the English version doesn't have. Drop that argument.
- **`position N type mismatch`** — you changed a `%1$d` to `%1$s` (or vice versa). The default's type is authoritative.

Fix and re-run until clean.

---

## Step 4: Optional — translate content files

`strings.yml` covers C-source strings. The six content categories under `files/langs/<locale>/` (`helpfiles`, `motds`, `textfiles`, `miscfiles`, `datafiles`, `adminfiles`) hold operator-edited prose. Translate any of these by copying the file from `en_GB/<cat>/<name>` and editing in place:

```bash
mkdir -p files/langs/<locale>/motds
cp files/langs/en_GB/motds/motd1 files/langs/<locale>/motds/motd1
$EDITOR files/langs/<locale>/motds/motd1
```

Phase 1's `locale_path()` resolver picks up the translated file for users on `<locale>` and falls back to `en_GB` for files you haven't translated.

There's no validation tool for content files — they're free-form prose. The only constraint is that any `~XX` colour escapes used in the original should remain reasonably balanced (no leftover `~OL` without a matching `~RS`).

---

## Step 5: Live-load the new locale

The operator does this on the running talker:

1. With the talker running, drop the new `files/langs/<locale>/` into place.
2. As a WIZ-level user, run `.langreload`.
3. Expected console message: `Language reload complete: N+1 locale(s) loaded; default = en_GB.` where N is the previous count.

If the syslog reports any keys dropped during load, fix them in `strings.yml` and `.langreload` again. The reload is atomic — until it succeeds the live talker keeps using the previous catalog, so a failed reload never leaves users mid-broken.

---

## Step 6: User opt-in

Any user can switch:

```
set lang
set lang <locale>
```

The `set lang` (no arg) listing now shows the new locale with its `meta.name` / `meta.description`. The user's choice persists across logout/login via the `language` line in their user file (Phase 2 Task 10).

For a one-user smoke test, point your own account at the new locale and run a few representative commands. For a broader test, ask a few volunteers to opt in for a day.

---

## Step 7: Handle translator feedback

Translators almost always come back with cosmetic issues — text that overflows boxes, accent characters that render as `?`, line wrapping that breaks awkwardly. Categorise:

- **Text too long for a box / table cell:** translator can shorten, or operator widens the frame. Frame widths live in C source, not in the catalog — file an issue if the layout doesn't accommodate the translation.
- **Accents render as `?`:** this is the **parked Unicode track** the design spec §10 calls out. The talker's telnet layer doesn't currently handle UTF-8 correctly. Latin-alphabet locales work; CJK / Arabic / Cyrillic / etc. depend on the Unicode work landing first. Document the limitation; close the feedback as "blocked on Unicode track."
- **Line wrapping breaks awkwardly:** for now this is mostly inherent to the prose — translators write tighter. Eventually a smarter wrap (visible-column word-wrap that knows about catalog locale) could land, but it isn't part of Phase 6.

---

## Step 8: Commit the locale and update the ledger

Once a translator is happy, commit:

```bash
git add files/langs/<locale>/
git commit -m "$(cat <<'EOF'
Ship <locale> translation

Initial translation of strings.yml by <translator>. Coverage:
NN of MMM keys translated, the rest fall back to en_GB. Content
directories <list> also translated; others fall back.

Co-Authored-By: <translator name and contact>
EOF
)"
```

Update `docs/superpowers/specs/localisation-migration.md`:

```markdown
| 6 — first non-default locale | converted (YYYY-MM-DD) | <locale> by <translator> — coverage NN of MMM keys |
```

Append a new section listing each shipped locale:

```markdown
## Shipped non-default locales

| Locale | Coverage | Translator | First shipped |
|--------|----------|------------|---------------|
| <locale> | NN / MMM keys + <content dirs> | <translator> | YYYY-MM-DD |
```

Subsequent translators add new rows.

---

## Notes for operators running multi-locale production

- **Backups.** `files/langs/<locale>/` is operator-curated content. Back it up with the rest of `files/`.
- **`.langreload` access.** Only WIZ-level users can `.langreload`. Decide who. If you trust your translator with WIZ, they can iterate live; otherwise they hand you the YAML and you reload.
- **Locale removal.** If you delete a locale directory, any user on that locale gets reset to the server default on the NEXT `.langreload` (or seamless reboot). They see a one-line in-band notice `[ Language reset to server default — your previous setting is gone. ]`. The catalog framework handles this cleanly — no orphaned `user->locale` references survive.
- **Default-locale changes.** `default_language` in `files/langs/<default>/datafiles/config` is read at boot only. Changing it requires a full restart (not a seamless reboot). Plan accordingly.

---

## Done

Phase 6 has no fixed completion criterion. "One non-default locale shipped" is the minimal bar; "every active operator has a translation for their primary user community" is the aspirational one. The framework supports incremental, additive growth — there's nothing more for the code track to do.
