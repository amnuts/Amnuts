# Localisation Phase 4 — Frame-Heavy Command Sweep Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert the five "frame-heavy" wizard / status commands — `show_igusers`, `grepusers`, `listbans`, `system`, and the `help` command's per-topic listing — from inline `+----+`/`|...|` box drawing to the Phase 3 `box_*`/`table_*`/`rule` builders, and move every server-authored string in those commands into the `strings.yml` catalog. After this phase, theming the talker's most visually distinctive screens is a matter of editing one locale's `strings.yml`.

**Architecture:** Each command is converted in its own commit. The pattern established by `wizlist` (Phase 3 Task 8) drives every conversion: replace inline frame-drawing with `box_open` / `table_open` + `box_line` / `table_row`, replace literal output strings with `lang_user` calls and matching catalog keys, and update the per-command sweep ledger. On en_GB every conversion is **byte-identical to its pre-conversion output**; on themed locales the same screens pick up the locale's frame style automatically.

**Tech Stack:** C (clang gnu23, `-Wall -Wextra -Wpedantic`), Phase 2's catalog + lang_* API, Phase 3's UI builders. No new dependencies. No test framework — verification is clean build + side-by-side telnet output comparison against the pre-conversion screen for each command.

**Reference spec:** `docs/superpowers/specs/2026-05-10-localisation-design.md` §9.1 (Phase 4 scope) and §11 (test plan).

**Codebase notes for the implementing engineer:**
- Phase 3 already shipped: `BOX`, `TABLE`, `visible_strlen`, `align_into`, `rule`, `box_*`, `table_*`. Read `src/uibuilders.c` once before starting so you understand the API.
- Phase 2 already shipped: `lang(user, key)`, `lang_user(user, key, ...)`, `lang_format(user, buf, buflen, key, ...)`. Use `lang_user` for plain server-to-user output, `lang_format` when you need the rendered string in a buffer for further composition, and `lang(user, key)` only as the format argument to `vwrite_user` on the same line (do not store it).
- The catalog hard-fails at boot if **default** strings.yml is missing keys that the running binary reads. Non-default locales soft-fall-back per-key. So when a conversion adds a new `foo.bar` key reference in C code, the en_GB catalog MUST be updated in the SAME commit — or the talker will print `[??? foo.bar]` to en_GB users.
- The conversion pattern: read the existing command, identify each `vwrite_user`/`write_user` call, mint a catalog key for each, and replace the call with `lang_user(user, "<key>", args...)`. The key namespace convention is `<command>.<context>.<variant>` (e.g., `system.title`, `system.uptime`, `listbans.swears.header`).
- "Byte-identical on en_GB" means the new key's value in `en_GB/strings.yml` reproduces the literal that used to be in the C source, INCLUDING colour escape sequences. Translators may then change emphasis, but the default ships as today.
- The migration ledger's "Per-command sweep ledger (Phase 5)" table is the right place to tick off each command. Phase 4 commands all use the same table.

**Commit convention:** one command per commit. Each commit modifies (a) the command's `.c` file, (b) `en_GB/strings.yml`, and (c) the migration ledger. Use `Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>`.

---

## File Structure

**Modified files (per command, each in its own commit):**

| Command          | Source file                       | Catalog key namespace |
|------------------|-----------------------------------|------------------------|
| `show_igusers`   | `src/commands/show_igusers.c`     | `show_igusers.*`       |
| `grepusers`      | `src/commands/grepusers.c`        | `grepusers.*`          |
| `listbans`       | `src/commands/listbans.c`         | `listbans.*`           |
| `system`         | `src/commands/system.c` (or equivalent — search) | `system.*`  |
| `help`           | `src/commands/help.c`             | `help.*`               |

**Cross-cutting modified files:**

- `files/langs/en_GB/strings.yml` — grows with each command's new keys.
- `docs/superpowers/specs/localisation-migration.md` — per-command sweep ledger ticks off each commit.

---

## Task 1: Establish the per-command conversion checklist

This task is documentation-only; it locks down the exact checklist every Phase 4 conversion follows so the five commits stay consistent in style.

**Files:**
- Modify: `docs/superpowers/specs/localisation-migration.md`

- [ ] **Step 1: Append a "Per-command conversion pattern" section.**

Append to `docs/superpowers/specs/localisation-migration.md` immediately above the existing `## Per-command sweep ledger (Phase 5)` section:

````markdown
## Per-command conversion pattern (Phase 4 + 5)

Every command conversion follows the same six-step checklist. Phase 4
commands are the frame-heavy ones (`show_igusers`, `grepusers`, `listbans`,
`system`, `help`); Phase 5 is everything else.

1. **Read** the existing command end to end. List every `write_user`,
   `vwrite_user`, `write_room*`, `vwrite_level*`, `write_syslog` call
   that produces server-authored output. (Skip `write_syslog` — those
   stay in C; admin logs aren't translatable.)

2. **Identify the frames.** Look for inline `+----+`, `|...|`,
   `+----+`, header lines, separator lines. These become `box_open`
   / `box_separator` / `table_*` / `rule` / `box_close` calls.

3. **Mint catalog keys.** Each unique string becomes one key in
   `<command>.<context>.<variant>` form. Examples:
   - Section titles → `<command>.title` or `<command>.<section>.title`
   - Column headers → `<command>.col.<colname>`
   - Body row templates → `<command>.row` (with positional `%1$s`,
     `%2$d`, etc.)
   - Footer / count summaries → `<command>.footer` or
     `<command>.empty` (the "no results" case)
   - Inline error/usage messages → `<command>.error.<what>` or
     `<command>.usage`

4. **Add the keys to `en_GB/strings.yml`** with EXACTLY the literal
   that appeared in the C source (including any `~OL`, `~FC`, `~RS`
   escapes). En_GB is the source of truth; everything else
   diff-matches against it.

5. **Convert the C source.** Replace each output call with the
   appropriate builder + `lang_user`. The byte-identical contract:
   on en_GB the user must see exactly what they saw before. Use
   `vwrite_user(user, lang(user, "key"), arg, …)` only as a single-line
   pattern; never store `lang()`'s return across statements.

6. **Update the sweep ledger.** Tick the command's row from `pending`
   to `converted (YYYY-MM-DD)`.

A conversion is "done" when:
- `make build` is clean.
- On en_GB the command's output before/after the commit is byte-identical
  (compare with `script` + `diff`, or eyeball if the output is short).
- On a themed locale (`cowboy`), the frames pick up the theme; the rest
  of the strings render unchanged (because no themed translations exist
  yet — Phase 6 is when translators arrive).
````

- [ ] **Step 2: Seed the per-command sweep ledger table.**

Replace the existing "Per-command sweep ledger (Phase 5)" placeholder section with a real table that already lists the Phase 4 commands as `pending`:

```markdown
## Per-command sweep ledger (Phases 4 + 5)

Phase 4: frame-heavy commands. Phase 5: every other command in src/commands/.

| Command         | File                              | Status   | Notes |
|-----------------|-----------------------------------|----------|-------|
| wizlist         | src/commands/wizlist.c            | converted (Phase 3 pilot) | |
| show_igusers    | src/commands/show_igusers.c       | pending  | Phase 4 |
| grepusers       | src/commands/grepusers.c          | pending  | Phase 4 |
| listbans        | src/commands/listbans.c           | pending  | Phase 4 |
| system          | <locate via semble>               | pending  | Phase 4 |
| help            | src/commands/help.c               | pending  | Phase 4 |

(Phase 5 rows added as commands are swept in that phase.)
```

- [ ] **Step 3: Commit.**

```bash
git add docs/superpowers/specs/localisation-migration.md
git commit -m "$(cat <<'EOF'
Document Phase 4 / Phase 5 per-command conversion pattern

Lock down the six-step checklist every command conversion follows so
the sweep stays consistent. Seed the ledger with the Phase 4 commands.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 2: Convert `show_igusers`

**Files:**
- Modify: `src/commands/show_igusers.c`
- Modify: `files/langs/en_GB/strings.yml`
- Modify: `docs/superpowers/specs/localisation-migration.md`

`show_igusers` lists the users a player is ignoring (or in admin mode, lists everyone someone else is ignoring). It's a moderate-size frame with a single column.

- [ ] **Step 1: Read the command end to end.**

```
semble search "show_igusers ignore list display" .
```

Read `src/commands/show_igusers.c`. Note:
- Total frame width (likely 78).
- Header / title text (usually contains the user's name).
- The column(s) for the ignored-user list.
- The footer / count summary.
- The "no users ignored" empty-state message.

- [ ] **Step 2: Pick keys.**

Append to `files/langs/en_GB/strings.yml`:

```yaml
# show_igusers
show_igusers.title.self:      "Users you are ignoring"
show_igusers.title.other:     "Users that %1$s is ignoring"
show_igusers.empty.self:      "You are not ignoring any users."
show_igusers.empty.other:     "%1$s is not ignoring any users."
show_igusers.footer:          "Total: %1$d user%2$s."
```

(Adapt to the actual messages the existing command emits — capture every literal.)

- [ ] **Step 3: Convert the command body.**

Pattern:

```c
void
show_igusers(UR_OBJECT user)
{
    UR_OBJECT u = /* the target (self or argument) */;
    int self = (u == user);
    int count = 0;
    /* ... walk the flagged-user list, count ignored entries, collect names ... */

    BOX b = box_open(user, 78,
                     self ? lang(user, "show_igusers.title.self")
                          : NULL);
    if (!self) {
        /* Fall through into a separately-rendered themed title because the
         * "other" form takes a username argument. Use lang_format into a
         * local buf then call box_open with that buf. */
        char title[ARR_SIZE];
        lang_format(user, title, sizeof title, "show_igusers.title.other",
                    u->recap);
        box_close(b);
        b = box_open(user, 78, title);
    }
    /* ... iterate, box_line per name ... */
    if (count == 0) {
        box_close(b);
        if (self) lang_user(user, "show_igusers.empty.self");
        else      lang_user(user, "show_igusers.empty.other", u->recap);
        return;
    }
    box_close(b);
    lang_user(user, "show_igusers.footer", count, count == 1 ? "" : "s");
}
```

Read the actual code first — the pattern above is illustrative, not literal. Match the existing column structure and message text exactly. Adapt the empty-state and self-vs-other split to what's already there.

- [ ] **Step 4: Tick the sweep ledger.**

In `docs/superpowers/specs/localisation-migration.md`, update the `show_igusers` row from `pending` to `converted (YYYY-MM-DD)`.

- [ ] **Step 5: Commit.**

```bash
git add src/commands/show_igusers.c files/langs/en_GB/strings.yml docs/superpowers/specs/localisation-migration.md
git commit -m "$(cat <<'EOF'
Convert show_igusers to UI builders + catalog

Frame switches to box_*; all output strings move into show_igusers.*
keys in en_GB/strings.yml. Byte-identical output on en_GB; picks up
themed frames on non-default locales.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 3: Convert `grepusers`

**Files:**
- Modify: `src/commands/grepusers.c`
- Modify: `files/langs/en_GB/strings.yml`
- Modify: `docs/superpowers/specs/localisation-migration.md`

`grepusers` is similar to `show_igusers` but with multiple columns (name + matching field). Tables, not boxes.

- [ ] **Step 1: Read the command.**

```
semble search "grepusers search match user pattern columns" .
```

Note the search modes / columns the existing command supports.

- [ ] **Step 2: Pick keys.**

Append to `en_GB/strings.yml`:

```yaml
# grepusers
grepusers.title:              "Grep users — matches for ~OL%1$s~RS"
grepusers.col.name:           "Name"
grepusers.col.match:          "Matching field"
grepusers.empty:              "No users matched ~OL%1$s~RS."
grepusers.footer:             "Matched %1$d user%2$s."
grepusers.usage:              "Usage: grepusers <pattern>\n"
```

- [ ] **Step 3: Convert with `table_open` / `table_columns` / `table_header` / `table_row`.**

Pattern (illustrative — match the actual columns):

```c
void
grepusers(UR_OBJECT user)
{
    if (word_count < 2) {
        lang_user(user, "grepusers.usage");
        return;
    }
    char title[ARR_SIZE];
    lang_format(user, title, sizeof title, "grepusers.title", word[1]);

    TABLE t = table_open(user, 78);
    table_columns(t, 2, 20, 56);
    /* Title row via box_centered? Or just a title above the table. The
     * existing command probably does a separate "search results" header.
     * Mirror that. */
    table_header(t, lang(user, "grepusers.col.name"),
                    lang(user, "grepusers.col.match"));
    /* ... iterate matches, call table_row per match ... */
    table_close(t);
    if (count == 0) lang_user(user, "grepusers.empty", word[1]);
    else            lang_user(user, "grepusers.footer", count, count == 1 ? "" : "s");
}
```

- [ ] **Step 4: Tick ledger.**

- [ ] **Step 5: Commit.**

```bash
git add src/commands/grepusers.c files/langs/en_GB/strings.yml docs/superpowers/specs/localisation-migration.md
git commit -m "$(cat <<'EOF'
Convert grepusers to UI builders + catalog

Two-column table via table_*; messages move into grepusers.* keys.
Byte-identical on en_GB; themeable on other locales.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 4: Convert `listbans`

**Files:**
- Modify: `src/commands/listbans.c`
- Modify: `files/langs/en_GB/strings.yml`
- Modify: `docs/superpowers/specs/localisation-migration.md`

`listbans` has multiple subcommands (`sites`, `users`, `new`, `swears`), each with its own frame.

- [ ] **Step 1: Read the command.**

```
semble search "listbans swears sites users new banned" .
```

Inventory each subcommand's:
- Header text
- Body shape (single column, multi-column, free text)
- Empty-state message
- The "(swearing ban is currently off)" inline note
- Usage line

- [ ] **Step 2: Pick keys.**

Append to `en_GB/strings.yml`:

```yaml
# listbans
listbans.usage:               "Usage: lban sites|users|new|swears\n"

listbans.swears.title:        "Banned swear words"
listbans.swears.empty:        "There are no banned swear words."
listbans.swears.off_note:     "(Swearing ban is currently off)"

listbans.new.title:           "New users banned from sites and domains"
listbans.new.empty:           "There are no sites and domains where new users have been banned.\n"

listbans.users.title:         "Banned users"
listbans.users.empty:         "There are no banned users."

listbans.sites.title:         "Banned sites and domains"
listbans.sites.empty:         "There are no banned sites and domains."
```

Read the existing C source to capture the exact wording for each — these are guesses based on the structure visible in the spec.

- [ ] **Step 3: Convert.**

Each subcommand branch replaces its inline `write_user("...\n")` block with a `box_open(user, 78, lang(user, "listbans.<sub>.title"))` + body iteration + `box_close` + footer where applicable. The `more(user, …, filename)` call to display a banned-sites file remains as-is — Phase 4 doesn't touch the file-paging mechanism.

- [ ] **Step 4: Tick ledger.**

- [ ] **Step 5: Commit.**

```bash
git add src/commands/listbans.c files/langs/en_GB/strings.yml docs/superpowers/specs/localisation-migration.md
git commit -m "$(cat <<'EOF'
Convert listbans to UI builders + catalog

Each subcommand (sites/users/new/swears) wraps its output in a
themed box; section titles + empty-state messages live in
listbans.* keys. The file-paging path (more()) for the banned-sites
list is unchanged.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 5: Convert `system`

**Files:**
- Modify: `src/commands/<system command file>` (locate via semble)
- Modify: `files/langs/en_GB/strings.yml`
- Modify: `docs/superpowers/specs/localisation-migration.md`

`system` displays the server's configuration / runtime status — a tall multi-section box. It's the most prose-heavy of the Phase 4 commands.

- [ ] **Step 1: Locate and read.**

```
semble search "system command uptime memory users boot show" .
```

Find the exact source file. `system` may be in `src/admin.c` rather than its own `commands/` file.

- [ ] **Step 2: Pick keys.**

The number of keys for `system` is larger than the other commands (probably 20+). Group them by section:

```yaml
# system command
system.title:                 "System Information"
system.section.server:        "Server"
system.row.version:           "Version       : ~OL%1$s~RS"
system.row.uptime:            "Uptime        : %1$s"
system.row.users:             "Users online  : %1$d / %2$d"
system.section.config:        "Configuration"
# ... etc ...
```

Each prose row becomes a key with positional `%1$s`/`%2$d`/etc. arguments matching what the existing code substitutes in. Read the existing code one row at a time to capture each.

- [ ] **Step 3: Convert.**

Use `box_open(user, W, lang(user, "system.title"))` for the outer frame, `rule` or `box_separator` for between-section dividers (or just `box_separator(b)` — pick whichever the existing layout most closely resembles), and `lang_user` / `box_line` for each prose row.

- [ ] **Step 4: Tick ledger.**

- [ ] **Step 5: Commit.**

```bash
git add <files> docs/superpowers/specs/localisation-migration.md
git commit -m "$(cat <<'EOF'
Convert system to UI builders + catalog

Single themed box with internal separators; every prose row moves
into system.* keys. Byte-identical on en_GB; themeable on other
locales.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 6: Convert `help`'s topic-display path

**Files:**
- Modify: `src/commands/help.c`
- Modify: `files/langs/en_GB/strings.yml`
- Modify: `docs/superpowers/specs/localisation-migration.md`

The `help` command has two parts: (a) the topic-display path that opens a help file from `langs/<locale>/helpfiles/` and pages it through `more`, and (b) the "commands list" and "credits" screens that have their own framed output. Phase 4 converts only the framed listings; the helpfile content itself remains operator-edited prose in `helpfiles/` (unchanged from Phase 1).

- [ ] **Step 1: Read `help.c` end to end.**

Identify:
- The unknown-topic / "no help" message.
- The commands-list header / footer (different per `cmd_type` 0/1).
- The credits screens (`.help credits`, `.help nuts`).

- [ ] **Step 2: Pick keys.**

```yaml
# help command
help.unknown:                 "Sorry, there is no help on that topic.\n"
help.commands.title:          "Commands available"
help.commands.col.name:       "Command"
help.commands.col.level:      "Level"
help.commands.col.type:       "Type"
help.credits.title:           "Amnuts credits"
help.credits.body:            |
  Original NUTS 3.3.3 by Neil Robertson 1996.
  Amnuts development by Andrew Collington 1996–2026.
  …
help.nuts.title:              "NUTS credits"
help.nuts.body:               |
  …
```

Block scalars (`|`) preserve newlines for multi-line bodies. Capture the existing C-source strings verbatim.

- [ ] **Step 3: Convert.**

- The unknown-topic path (currently `write_user(user, "Sorry, there is no help on that topic.\n")`) becomes `lang_user(user, "help.unknown")`.
- `help_commands_level()` / `help_commands_function()` — their box/table drawing replaces with `box_open`/`table_open`.
- `help_amnuts_credits()` / `help_nuts_credits()` — wrap each in a `box_open` + `box_line` for each paragraph.

The actual helpfile contents path (read from disk via `more()`) stays as it is. Phase 1's `locale_path()` already directs that lookup to the user's locale.

- [ ] **Step 4: Tick ledger.**

- [ ] **Step 5: Commit.**

```bash
git add src/commands/help.c files/langs/en_GB/strings.yml docs/superpowers/specs/localisation-migration.md
git commit -m "$(cat <<'EOF'
Convert help command framing to UI builders + catalog

Commands listing and credits screens use box_*/table_*; unknown-topic
and other inline messages move into help.* keys. The per-topic
helpfile content path (more() over files/langs/<locale>/helpfiles/)
is unchanged — that's already locale-aware from Phase 1.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 7: End-to-end verification with all five commands

**Files:** (none committed)

The Linux/macOS host smoke-test that proves the entire Phase 4 sweep landed cleanly.

- [ ] **Step 1: Build.** `make build` — clean.

- [ ] **Step 2: Pre-flight check.** Boot the talker; expect the startup lines:
  ```
  Localisation: discovered N locale(s); default = en_GB.
  Localisation: catalog loaded (N locale(s)).
  ```

- [ ] **Step 3: For each command, byte-compare default-locale output before vs after.**

Pre-conversion screen capture isn't required if the implementer was careful with the en_GB strings, but a quick visual diff is wise. On the live talker (default user, no `set lang`), run:

- `.show_igusers` (alone, then with an argument)
- `.grepusers <pattern>` with a known match
- `.lban sites` / `.lban users` / `.lban new` / `.lban swears`
- `.system`
- `.help` / `.help commands` / `.help credits` / `.help <topic>`

Each screen should look identical to a pre-Phase-4 baseline. Spot-check column alignment in particular.

- [ ] **Step 4: Switch to cowboy and re-run.** `set lang cowboy`, then re-run every command. Every frame should now use the cowboy theme; every string remains in English (because no translations exist in cowboy/strings.yml for the new keys — they fall back to en_GB).

- [ ] **Step 5: Stress-test long input.** Run `.grepusers` with a pattern that produces wide matches, including coloured `recap` names. Verify table column wrapping doesn't drift the right border.

- [ ] **Step 6: No commit.**

---

## Task 8: Phase 4 migration ledger update

**Files:**
- Modify: `docs/superpowers/specs/localisation-migration.md`

- [ ] **Step 1: Mark Phase 4 converted.**

```markdown
| 4 — frame-heavy commands | converted (2026-MM-DD) | show_igusers, grepusers, listbans, system, help — all framed output via box_*/table_*; strings in en_GB/strings.yml |
```

- [ ] **Step 2: Append a Phase 4 verification block** mirroring earlier phases.

- [ ] **Step 3: Commit.**

```bash
git add docs/superpowers/specs/localisation-migration.md
git commit -m "$(cat <<'EOF'
Phase 4 complete: frame-heavy command sweep

Five commands converted: show_igusers, grepusers, listbans, system,
help. Each landed in its own commit with matching en_GB catalog
entries and a ledger update.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Self-Review Checklist

- [ ] Spec §9.1 Phase 4 scope: `wizlist` (Phase 3 pilot), `show_igusers`, `grepusers`, `listbans`, `system`, parts of `help`. All listed. ✓
- [ ] Conversion pattern: Task 1 locks the six-step checklist. Subsequent tasks each call it out. ✓
- [ ] Byte-identical-on-en_GB contract: each task's commit message states it, and the verification step (Task 7) confirms it. ✓
- [ ] Per-command commits: one command per task → one commit each. Migration is reversible per command, and bisecting is easy if a regression slips through. ✓
- [ ] Ledger updates: every conversion task ends with a ledger update step. ✓
- [ ] Phase 5 hand-off: Task 1's "Per-command conversion pattern" section is reusable verbatim in Phase 5. ✓

**Placeholder scan:** The illustrative C in Tasks 2–6 ("Pattern (illustrative — match the actual columns):") is deliberately not a copy-paste recipe — the implementer must read each command and adapt. That's intentional because the existing commands vary in shape; a literal recipe would mislead. The "Read the existing command first" step is the substantive instruction, and the pattern shows what the result should look like.

**Type consistency:** Every `lang(user, "key")` returns `const char *` (Phase 2 contract); every `lang_user(user, "key", args)` returns void; every `box_*`/`table_*` API call uses the types established in Phase 3. No new types introduced in Phase 4.
