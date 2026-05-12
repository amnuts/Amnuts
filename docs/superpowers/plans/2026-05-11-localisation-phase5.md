# Localisation Phase 5 — Bulk Inline-String Conversion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Sweep the remaining ~1,500 `write_user` / `vwrite_user` / `write_room*` / `vwrite_level*` call sites in `src/commands/` (and a handful in `src/admin.c`, `src/messages.c`, `src/games.c`, `src/amnuts.c`) and convert each one to its `lang_*` equivalent with a catalog key in `en_GB/strings.yml`. Build the Python tooling (`tools/locale/{extract,refs,check}.py`) the design spec calls for so translators have a workflow. Each command file is independently mergeable; unswept sites continue to use `write_user` and remain non-translatable — that's a feature, not a bug.

**Architecture:** This phase has no single end state — it's a steady drumbeat of small per-file conversions. We establish the tooling first (Task 1–3), then sweep `src/commands/*.c` file-by-file. Each sweep mirrors the Phase 4 pattern (read, mint keys, convert, ship en_GB strings, update ledger), but operates on commands whose output isn't frame-heavy: short status / error / acknowledgement messages mostly. Phase 5 is "done" not when every site is converted but when the ledger reaches an agreed coverage threshold (target: 90%+ of user-facing strings) or when a deliberate decision is made to ship.

**Tech Stack:** C (clang gnu23, `-Wall -Wextra -Wpedantic`), Phase 2 catalog + lang_* API, Phase 3 UI builders (used selectively), Python 3 + PyYAML for the new tooling. No new C-side dependencies. No test framework — verification is clean build + spot-check telnet play-through per converted command.

**Reference spec:** `docs/superpowers/specs/2026-05-10-localisation-design.md` §9.1 (Phase 5), §9.2 (tooling), §11 (test plan).

**Codebase notes for the implementing engineer:**
- Phases 2–4 already shipped: full `lang_*` API, `set lang` / `langreload`, UI builders, five frame-heavy commands converted. Phase 5 reuses the per-command conversion pattern documented at the top of `docs/superpowers/specs/localisation-migration.md` (added in Phase 4 Task 1).
- `src/commands/` currently contains roughly 165 `.c` files. Most have between 1 and 30 user-facing strings. Estimate: ~10 minutes of mechanical work per simple command, more for complex ones. Pace expectation: 5–15 commands per implementation session.
- Some command files have no user-facing output (pure dispatcher, pure persistence). Mark those `not-applicable` in the ledger rather than `converted` so the difference is visible.
- The `lang_level` `notify_invis = 0` semantic gap (flagged at the end of Phase 2) MUST be closed before any Phase 5 conversion migrates a `vwrite_level(<lvl>, 0, …)` call. Task 0 below closes it.
- New keys go into `files/langs/en_GB/strings.yml`. The file is going to grow substantially — keep it sorted by key prefix groups (one block of `<command>.*` keys per command). Add comments above each group so a translator can navigate.
- `lang()`'s raw-pointer return is unsafe to store. Use `lang_user` / `lang_format` whenever the rendered string needs to live beyond a single statement. The header comment on `lang()` documents this; treat it as a hard rule.

**Commit convention:** one command per commit. Each commit modifies (a) one or more `.c` files for that command, (b) `en_GB/strings.yml`, (c) the migration ledger. Use `Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>`. Keep commit messages tight: `Convert <command> to lang_*` body explaining notable surprises.

---

## File Structure

**New files:**
- `tools/locale/extract.py` — given a `.c` file, lists output literals and suggests catalog keys.
- `tools/locale/refs.py` — walks source for `lang_*("key", ...)` calls and walks loaded catalogs; reports missing keys (in source but not in en_GB) and orphans (in en_GB but no source caller).
- `tools/locale/check.py` — runs format-signature validation as a standalone CLI for translators (loads en_GB plus one other locale, reports drops).
- `tools/locale/README.md` — short usage docs for the three tools.

**Modified files (per command, each in its own commit):**
- Each command's `.c` file under `src/commands/`.
- `files/langs/en_GB/strings.yml` — grows per conversion.
- `docs/superpowers/specs/localisation-migration.md` — per-command sweep ledger ticks.

**Cross-cutting Task 0:**
- `src/catalog.c` — close the `notify_invis` semantic gap in `lang_level`.

---

## Task 0: Close the `lang_level` notify_invis gap

**Files:**
- Modify: `src/catalog.c`

Phase 2 left `notify_invis` as a no-op with a one-shot warning syslog. Phase 5 conversions will migrate several `vwrite_level(<lvl>, 0, …)` call sites; running with the no-op would silently regress the invisibility-suppression contract. Close the gap before any Phase 5 conversion touches `vwrite_level`.

- [ ] **Step 1: Read `vwrite_level`'s semantics.**

```
semble search "vwrite_level notify_invis hide invisible user broadcast" .
```

Read the function in `src/messages.c` end to end. Capture:
- Which condition makes a recipient "invisible" for the purposes of suppression (`u->vis == 0`? `u->level >= ARCH`? both?).
- What `notify_invis = 0` and `notify_invis = 1` do differently.
- Whether `record_flag` affects the message routing or only audit logging.

- [ ] **Step 2: Port the semantics into `lang_level`.**

In `src/catalog.c`'s `lang_level`, replace the `static int warned_notify_invis;` block and the `(void) notify_invis;` cast with the actual gating logic. Outline:

```c
void
lang_level(int min_level, int notify_invis, int record_flag,
           UR_OBJECT exclude, const char *key, ...)
{
    /* record_flag exists for parity with vwrite_level; in Phase 5 it
     * controls whether the broadcast is added to review buffers. Mirror
     * vwrite_level's behaviour exactly — see src/messages.c. */
    va_list ap0;
    va_start(ap0, key);

    for (UR_OBJECT u = user_first; u; u = u->next) {
        if (u->type == CLONE_TYPE) continue;
        if (u == exclude) continue;
        if (u->level < (enum lvl_value) min_level) continue;

        /* notify_invis = 0: suppress for invisible recipients.
         * (Replicate the exact check vwrite_level uses.) */
        if (!notify_invis && u->vis == 0) continue;

#ifdef NETLINKS
        if (!u->socket) continue;
#endif
        const char *fmt = catalog_resolve(u->catalog, key);
        char buf[ARR_SIZE * 2];
        if (!fmt) {
            snprintf(buf, sizeof buf, "[??? %s]\n", key);
        } else {
            va_list apc;
            va_copy(apc, ap0);
            vsnprintf(buf, sizeof buf, fmt, apc);
            va_end(apc);
        }
        write_user(u, buf);
        /* record_flag: forward into the room's review buffer or the
         * level's audit log, exactly as vwrite_level does. Verify
         * the call into write_room_buffer / write_level_log / etc.
         * by reading src/messages.c. */
    }
    va_end(ap0);
}
```

The exact `notify_invis` check and `record_flag` handling MUST match `vwrite_level`. Read it carefully. If `record_flag` participates in writing to per-user review buffers, replicate that. If it gates audit syslogging, replicate that too.

- [ ] **Step 3: Verify by side-by-side comparison.**

Pick one existing call site in `src/messages.c` or elsewhere that uses `vwrite_level` with both `notify_invis` and `record_flag` non-default. Add a temporary debug syslog line at the head of `lang_level` and a similar one in `vwrite_level` recording each delivered recipient. Run the talker, exercise the call site, and confirm both functions deliver to the same set of recipients. Remove the debug code before committing.

(If the implementer is uncomfortable with this verification step on the Windows host where the talker can't actually run, ALSO insert the same temporary debug into both functions for the operator to verify when they next build on Linux. Note that explicitly in the commit message.)

- [ ] **Step 4: Commit.**

```bash
git add src/catalog.c
git commit -m "$(cat <<'EOF'
lang_level: port full notify_invis and record_flag semantics

Phase 5 conversions begin to migrate vwrite_level call sites that
pass notify_invis = 0 or record_flag = 1; before that landed, the
flags were silently ignored and the no-op-warning syslog was the
only safety net. Now the function fully matches vwrite_level so
conversions are byte-equivalent.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 1: Build the `extract.py` tool

**Files:**
- Create: `tools/locale/extract.py`
- Create: `tools/locale/README.md`

Given a `.c` file, scan for `write_user`, `vwrite_user`, `write_room*`, `vwrite_level*` calls; extract the literal format string; suggest a catalog key; print a YAML stub the implementer can paste into `strings.yml`.

- [ ] **Step 1: Write `tools/locale/extract.py`.**

```python
#!/usr/bin/env python3
"""
Extract user-facing string literals from a C source file.

Walks every `write_user(...)`, `vwrite_user(...)`, `write_room*(...)`,
and `vwrite_level*(...)` invocation and prints a YAML stub of catalog
keys with the literal value. Run before converting a command file:

    python tools/locale/extract.py src/commands/foo.c > foo.yml

Then paste the stub into files/langs/en_GB/strings.yml, edit keys,
and convert the C source.
"""
import argparse
import os
import re
import sys

OUTPUT_FNS = re.compile(r"\b(v?write_user|v?write_room(?:_except)?|v?write_level|v?write_monitor)\s*\(")

# Crude: find string literals appearing as arguments. Doesn't handle
# concatenation across multiple lines well, but the codebase mostly
# uses single-line literals. The output is a suggestion, not a
# mechanical rewrite — the human reviews before committing.
LITERAL = re.compile(r'"((?:\\.|[^"\\])*)"')


def suggest_key(filename, literal, index):
    base = os.path.splitext(os.path.basename(filename))[0]
    # Slugify the first few visible words of the literal.
    visible = re.sub(r"~[A-Za-z0-9]{2}", "", literal)
    visible = re.sub(r"[^A-Za-z0-9]+", "_", visible).strip("_").lower()
    if not visible:
        visible = "msg"
    visible = "_".join(visible.split("_")[:4])
    return f"{base}.{visible or f'msg_{index}'}"


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("source", help="C source file")
    args = p.parse_args()

    with open(args.source, encoding="utf-8", errors="replace") as f:
        text = f.read()

    findings = []
    for m in OUTPUT_FNS.finditer(text):
        # Find the next string literal after the call, on the same logical
        # statement (until we hit a semicolon at depth 0).
        i = m.end()
        depth = 1
        snippet = []
        while i < len(text) and depth > 0:
            c = text[i]
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
            elif c == ";" and depth == 0:
                break
            snippet.append(c)
            i += 1
        snippet_text = "".join(snippet)
        lits = LITERAL.findall(snippet_text)
        if not lits:
            continue
        # The first literal is typically the format string.
        findings.append((m.start(), lits[0]))

    print(f"# Stub for {args.source} — review keys before pasting.")
    print(f"# Generated by tools/locale/extract.py.")
    for idx, (offset, lit) in enumerate(findings, start=1):
        line_no = text.count("\n", 0, offset) + 1
        key = suggest_key(args.source, lit, idx)
        # YAML-escape: double-quoted scalar.
        yaml_val = lit.replace("\\", "\\\\").replace('"', '\\"')
        print(f"\n# {args.source}:{line_no}")
        print(f'{key}: "{yaml_val}"')


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 2: Write `tools/locale/README.md`.**

```markdown
# Localisation tools

Three small scripts that support the bulk Phase 5 sweep and the
translator workflow that follows. All three are dev-only Python 3,
require PyYAML, and are not bundled with the talker binary.

## extract.py

Suggests catalog keys for a single C source file:

    python tools/locale/extract.py src/commands/quit.c

Output is YAML you can paste into `files/langs/en_GB/strings.yml`,
review, and edit. The keys are suggestions — change them to match
the project's `<command>.<context>.<variant>` convention.

## refs.py

Cross-checks `lang_*("key", ...)` invocations in source against the
catalog (`files/langs/en_GB/strings.yml`). Reports:

- Keys used in source but missing from the default catalog.
- Keys in the default catalog with no source caller (orphans).

    python tools/locale/refs.py

Run before committing each conversion.

## check.py

Validates a non-default locale's `strings.yml` against the default's
signatures. Loads both files, runs the same format-signature
extraction the C-side loader uses, and reports the keys a real
talker boot would drop.

    python tools/locale/check.py files/langs/fr/strings.yml

Useful for translators before they push a new locale.
```

- [ ] **Step 3: Test the extractor.**

```
python tools/locale/extract.py src/commands/quit.c
```

Spot-check the output — it should list each user-facing literal in `quit.c` with a suggested key.

- [ ] **Step 4: Commit.**

```bash
git add tools/locale/extract.py tools/locale/README.md
git commit -m "$(cat <<'EOF'
tools/locale/extract.py: per-command key extractor

Walks one C source file, lists every literal passed to
write_user / vwrite_user / write_room* / vwrite_level*, suggests
catalog keys, prints a YAML stub the implementer can paste into
en_GB/strings.yml.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 2: Build the `refs.py` tool

**Files:**
- Create: `tools/locale/refs.py`

Cross-check `lang_*("key", ...)` usage in source against the catalog.

- [ ] **Step 1: Write `tools/locale/refs.py`.**

```python
#!/usr/bin/env python3
"""
Cross-check lang_*("key", ...) calls in source against the default
catalog. Reports missing keys (referenced in C but absent from
en_GB/strings.yml) and orphans (in en_GB/strings.yml but no caller).
"""
import argparse
import os
import re
import sys

try:
    import yaml
except ImportError:
    sys.exit("PyYAML required: pip3 install pyyaml")

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CATALOG   = os.path.join(REPO_ROOT, "files", "langs", "en_GB", "strings.yml")
SRC_GLOB  = os.path.join(REPO_ROOT, "src")

LANG_CALL = re.compile(r'\blang(?:_user|_room|_level|_format|)\s*\([^,)]*,\s*"([^"]+)"')


def find_source_keys():
    keys = {}  # key -> [(file, line), ...]
    for dirpath, _, files in os.walk(SRC_GLOB):
        if "vendors" in dirpath:
            continue
        for f in files:
            if not f.endswith(".c"):
                continue
            path = os.path.join(dirpath, f)
            with open(path, encoding="utf-8", errors="replace") as fh:
                for line_no, line in enumerate(fh, start=1):
                    for k in LANG_CALL.findall(line):
                        keys.setdefault(k, []).append((path, line_no))
    return keys


def find_catalog_keys():
    with open(CATALOG, encoding="utf-8") as f:
        data = yaml.safe_load(f) or {}
    return set(data.keys())


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--missing-only", action="store_true")
    p.add_argument("--orphans-only", action="store_true")
    args = p.parse_args()

    source_keys = find_source_keys()
    cat_keys    = find_catalog_keys()

    missing = sorted(set(source_keys) - cat_keys)
    orphans = sorted(cat_keys - set(source_keys))

    if not args.orphans_only and missing:
        print(f"# Missing keys ({len(missing)}): referenced in source but not in {os.path.relpath(CATALOG, REPO_ROOT)}.")
        for k in missing:
            sites = source_keys[k]
            print(f"{k}")
            for path, line in sites[:5]:
                print(f"  {os.path.relpath(path, REPO_ROOT)}:{line}")
            if len(sites) > 5:
                print(f"  ... and {len(sites) - 5} more.")
        print()

    if not args.missing_only and orphans:
        # Filter out ui.* and meta.* keys — they're consumed by the C
        # builders / set-lang listing rather than direct lang_* calls,
        # so they look orphaned but aren't.
        real_orphans = [k for k in orphans
                        if not k.startswith("ui.")
                        and not k.startswith("meta.")]
        if real_orphans:
            print(f"# Orphan keys ({len(real_orphans)}): present in catalog but no lang_* caller.")
            for k in real_orphans:
                print(f"  {k}")

    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
```

The `ui.*` / `meta.*` filter prevents false-positive orphans for keys consumed by `lang()` directly inside `src/uibuilders.c` (which the regex matches less reliably) and by `locale_list`.

- [ ] **Step 2: Test.**

```
python tools/locale/refs.py
```

After Phase 4, this should report no missing keys and a small number of false-positive orphans (any `ui.*` / `meta.*` should be filtered out). Confirm by reading the output.

- [ ] **Step 3: Commit.**

```bash
git add tools/locale/refs.py
git commit -m "$(cat <<'EOF'
tools/locale/refs.py: lang_* caller / catalog cross-check

Walks all C sources for lang_*("key", ...) calls and reports keys
referenced in source but missing from en_GB, plus keys in en_GB
with no caller. Filters out ui.* and meta.* (consumed by builders
and the locale listing). Run before each Phase 5 commit.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 3: Build the `check.py` tool

**Files:**
- Create: `tools/locale/check.py`

A standalone format-signature validator translators can run before submitting their `strings.yml`.

- [ ] **Step 1: Write `tools/locale/check.py`.**

```python
#!/usr/bin/env python3
"""
Validate a non-default locale's strings.yml against the default's
format signatures. Mirrors the C-side load-time validation. Run
before committing a translation:

    python tools/locale/check.py files/langs/fr/strings.yml
"""
import argparse
import os
import re
import sys

try:
    import yaml
except ImportError:
    sys.exit("PyYAML required: pip3 install pyyaml")

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_YML = os.path.join(REPO_ROOT, "files", "langs", "en_GB", "strings.yml")


def extract_signature(fmt):
    """Return (count, types_per_position) for a printf format. Raise on disallowed input."""
    types = [None] * 8
    implicit = 0
    pos_mode = imp_mode = False
    i = 0
    while i < len(fmt):
        if fmt[i] != "%":
            i += 1; continue
        i += 1
        if i >= len(fmt):
            raise ValueError("trailing %")
        if fmt[i] == "%":
            i += 1; continue
        # Optional position
        pos = 0
        j = i
        while j < len(fmt) and fmt[j].isdigit():
            j += 1
        if j > i and j < len(fmt) and fmt[j] == "$":
            pos = int(fmt[i:j])
            i = j + 1
            pos_mode = True
        if pos == 0:
            implicit += 1
            pos = implicit
            imp_mode = True
        if pos_mode and imp_mode:
            raise ValueError("mixed positional and implicit specifiers")
        if pos < 1 or pos > 8:
            raise ValueError(f"argument position out of range: {pos}")
        # Skip flags / width / precision / length
        while i < len(fmt) and fmt[i] in "-+ #0": i += 1
        while i < len(fmt) and fmt[i].isdigit(): i += 1
        if i < len(fmt) and fmt[i] == ".":
            i += 1
            while i < len(fmt) and fmt[i].isdigit(): i += 1
        while i < len(fmt) and fmt[i] in "hlLzjt": i += 1
        if i >= len(fmt):
            raise ValueError("trailing %")
        conv = fmt[i]; i += 1
        if conv in "diuxXo":
            canon = "d"
        elif conv == "s":
            canon = "s"
        elif conv == "c":
            canon = "c"
        elif conv == "n":
            raise ValueError("%n disallowed")
        else:
            raise ValueError(f"unsupported conversion: {conv!r}")
        if types[pos - 1] and types[pos - 1] != canon:
            raise ValueError("argument type conflict between positions")
        types[pos - 1] = canon
    return types


def load_yaml(path):
    with open(path, encoding="utf-8") as f:
        return yaml.safe_load(f) or {}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("locale_yml", help="Path to the locale's strings.yml")
    args = p.parse_args()

    default = load_yaml(DEFAULT_YML)
    target  = load_yaml(args.locale_yml)

    errors = 0
    for key, val in target.items():
        if not isinstance(val, str):
            print(f"DROP  {key!r}: value is not a scalar string")
            errors += 1
            continue
        try:
            tgt_sig = extract_signature(val)
        except ValueError as e:
            print(f"DROP  {key!r}: {e}")
            errors += 1
            continue
        if key in default:
            try:
                def_sig = extract_signature(default[key])
            except ValueError:
                continue  # default itself broken; reported when checking en_GB
            for i in range(8):
                if tgt_sig[i] and not def_sig[i]:
                    print(f"DROP  {key!r}: uses position {i+1} that default omits")
                    errors += 1
                    break
                if tgt_sig[i] and def_sig[i] and tgt_sig[i] != def_sig[i]:
                    print(f"DROP  {key!r}: position {i+1} type mismatch ({tgt_sig[i]} vs default {def_sig[i]})")
                    errors += 1
                    break

    if errors == 0:
        print(f"OK: {args.locale_yml} validates clean.")
    else:
        print(f"\n{errors} key(s) would be dropped at load time.")
    return 0 if errors == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 2: Test against `fallback_test`** (which currently has no strings.yml — write a quick one with a deliberate `%n` to confirm the tool catches it).

- [ ] **Step 3: Commit.**

```bash
git add tools/locale/check.py
git commit -m "$(cat <<'EOF'
tools/locale/check.py: translator-side strings.yml validator

Replicates the C-side format-signature validation as a standalone
CLI. Translators run it before pushing a new locale; the same
keys the talker would drop at load time are reported here, with
human-readable reasons.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 4: Sweep batch — small simple commands (~10 commands per commit)

The bulk of Phase 5. Tasks 4a, 4b, … are batches of small command conversions. Each batch is a single commit covering up to 10 commands that are individually trivial (1–5 strings each, no frames). Batching keeps the commit log scrollable while preserving the "one logical change per commit" principle.

**Files (per batch commit):**
- Multiple `.c` files in `src/commands/` (the batch).
- `files/langs/en_GB/strings.yml`.
- `docs/superpowers/specs/localisation-migration.md`.

### Task 4a: First simple-command batch

- [ ] **Step 1: Pick 10 small commands not yet swept.**

`semble search`-or-`ls` `src/commands/` for `.c` files with 1–5 user-facing strings each. Good starting candidates (read each first to verify):

- `quit.c`
- `passwd.c`
- `version.c` (the `ver` command)
- `set_age.c` / `set_email.c` / `set_homep.c` / `set_icq.c` (small set_*)
- `bcast.c` (likely already in scope: a one-liner broadcast)
- `wake.c` / `muzzle.c` / `unmuzzle.c` (admin one-liners)

Skip files that have already been swept or that have no user-facing output.

- [ ] **Step 2: For each command in the batch, run the extractor.**

```
python tools/locale/extract.py src/commands/<file>.c
```

Review the suggested keys. Adapt to the `<command>.<context>.<variant>` convention.

- [ ] **Step 3: Update `en_GB/strings.yml`.**

Append a new comment-headed block per command. Sort blocks roughly alphabetically by command name so the file stays navigable.

```yaml
# quit
quit.confirm:                 "Are you sure? (y/n)\n"
quit.goodbye:                 "Goodbye!\n"

# passwd
passwd.usage:                 "Usage: passwd <old> <new>\n"
passwd.changed:               "Password changed.\n"
passwd.mismatch:              "Old password does not match.\n"

# ...
```

- [ ] **Step 4: Convert each command's source.**

For each `write_user(user, "literal")` → `lang_user(user, "key")`.
For each `vwrite_user(user, "fmt", arg1, arg2)` → `lang_user(user, "key", arg1, arg2)`.
For each `write_room(rm, "literal")` → `lang_room(rm, NULL, "key")` (or with `exclude` if the original used `write_room_except`).
For each `vwrite_level(lvl, ...)` → `lang_level(lvl, notify_invis, record_flag, exclude, "key", args...)`.

- [ ] **Step 5: Run the cross-checker.**

```
python tools/locale/refs.py
```

Expect zero missing keys after the en_GB update. Any unexpected orphans should be investigated.

- [ ] **Step 6: Update the sweep ledger.**

Tick each converted command's row to `converted (YYYY-MM-DD)`.

- [ ] **Step 7: Commit the batch.**

```bash
git add src/commands/quit.c src/commands/passwd.c <other files...> files/langs/en_GB/strings.yml docs/superpowers/specs/localisation-migration.md
git commit -m "$(cat <<'EOF'
Phase 5 sweep batch: quit, passwd, ver, set_{age,email,homep,icq}, bcast, wake, muzzle, unmuzzle

10 small commands converted to lang_* + catalog keys. Each is a
2-5 string trivial conversion; batched into one commit because
individually they'd swamp the log.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

### Task 4b–4n: Repeat for further batches

The implementer continues Task 4 in batches of ~10 commands until the simple-commands queue is exhausted. Each batch:

- Picks 10 not-yet-swept simple commands.
- Runs the same six-step sweep.
- Commits as `Phase 5 sweep batch: <commands>`.

There's no fixed number of batches — the size of `src/commands/` is finite; estimate 10–15 batches total to cover the simple-command queue.

---

## Task 5: Sweep individually-large commands (1 command per commit)

Some commands have 20+ strings or non-trivial control flow. Convert each one in its own commit so it can be reviewed and reverted independently.

**Likely candidates (verify before starting):**

- `examine.c` (user profile display)
- `who.c` (the connected-user listing)
- `tell_user.c` (tells, including AFK/edit detours)
- `mail.c` / `smail.c` / `rmail.c` (mail commands)
- `room.c` (room info display)
- `display.c` (file viewer)
- `topic.c` (room topic management)
- `editor` / `lmail` (the line-editor flows)
- `setbans.c` (the ban configuration)
- `setlevel.c` (level configuration)

Use semble to discover them:

```
semble search "examine command user profile email idle login" .
```

- [ ] **For each large command:**

  - **Step 1: Read end to end.**
  - **Step 2: Run extractor; review keys; adapt to convention.**
  - **Step 3: Append catalog block in en_GB/strings.yml.**
  - **Step 4: Convert source.**
  - **Step 5: Cross-check with `refs.py`.**
  - **Step 6: Tick ledger.**
  - **Step 7: Commit.**

Allocate one task per command in the actual execution; this plan lists them generically because the pace will be set by the implementer.

---

## Task 6: Sweep non-commands (admin.c, messages.c, games.c, parts of amnuts.c)

A handful of user-facing strings live outside `src/commands/`. Catch them here.

- [ ] **Step 1: Find them.**

```
semble search "write_user vwrite_user admin.c messages.c games.c amnuts.c" .
```

Expected hot files: `src/admin.c`, `src/messages.c`, `src/games.c`, `src/amnuts.c` (the connect/disconnect flow).

- [ ] **Step 2: Sweep each, one commit per file.**

The per-command pattern applies. `src/amnuts.c` is large (~7000 lines) — consider splitting its sweep across multiple commits by logical section (connect, login, disconnect, room-movement helpers, etc.).

- [ ] **Step 3: Ledger.**

Add new rows for these non-command files. Use a `(non-command)` annotation to distinguish them from the command-file sweep.

---

## Task 7: Phase 5 verification pass with `refs.py`

**Files:** (none committed)

After the sweeps subside, run a comprehensive cross-check.

- [ ] **Step 1: Missing keys.**

```
python tools/locale/refs.py --missing-only
```

Should be empty. Anything reported here is a Phase 5 bug — the implementer added a `lang_*` call but forgot to ship the en_GB key.

- [ ] **Step 2: Orphans.**

```
python tools/locale/refs.py --orphans-only
```

Should be very small (filter is already in place for `ui.*` / `meta.*`). Real orphans suggest stale catalog entries — either delete them or restore the caller that should be using them.

- [ ] **Step 3: Boot the talker and `langreload`.**

Run a `.langreload` as WIZ. The reload should report no per-key drops on en_GB (drops imply a bad format string slipped in during the sweep).

- [ ] **Step 4: No commit.**

---

## Task 8: Phase 5 migration ledger update

**Files:**
- Modify: `docs/superpowers/specs/localisation-migration.md`

- [ ] **Step 1: Set Phase 5 to "in progress" with coverage stat.**

Phase 5 is more of a milestone than a discrete completion. Record coverage:

```markdown
| 5 — bulk inline-string conversion | in progress (NNN of MMM commands swept) | per-command sub-ledger below |
```

When the implementer decides to "ship" Phase 5 (typically when coverage reaches an agreed threshold — 90% is reasonable), update to:

```markdown
| 5 — bulk inline-string conversion | converted (YYYY-MM-DD; NNN of MMM commands swept) | remaining MMM are intentionally unswept (admin-only, deprecated, or out-of-scope) |
```

- [ ] **Step 2: Per-command sub-ledger.**

The table seeded in Phase 4 Task 1 grows here. By Phase 5's end the table should list every `src/commands/*.c` file with one of: `converted`, `not-applicable`, or `deferred`.

- [ ] **Step 3: Append a Phase 5 verification block** mirroring the earlier phases.

- [ ] **Step 4: Commit.**

```bash
git add docs/superpowers/specs/localisation-migration.md
git commit -m "$(cat <<'EOF'
Phase 5: declare in-progress / converted with coverage stats

(Adjust message based on actual milestone.)

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Operator-Facing Notes

Phase 5 ships a partial-conversion state intentionally. Unswept commands continue to use `write_user` and remain non-translatable. The talker still works for all users; non-default-locale users see English for the unswept commands. There's no operational pressure to finish Phase 5 — it's a steady migration that benefits from being mergeable in small pieces.

When operators ask "is Phase 5 done?" the right answer is: "The framework is fully in place since Phase 2; conversions are landing per command. See the migration ledger for current coverage." There's no hard finish line for this phase — the project decides when coverage is sufficient.

---

## Self-Review Checklist

- [ ] Spec §9.1 (Phase 5 scope: ~1500 sites in commands + a handful in admin/messages/games/amnuts) → Tasks 4–6 cover commands; Task 6 covers the non-command files.
- [ ] Spec §9.2 (tooling: extract.py / refs.py / check.py) → Tasks 1–3 ship all three plus a README.
- [ ] Spec §9.3 (migration tracking via the ledger) → Per-command commits each tick the ledger; Task 8 finalises.
- [ ] Spec §11 Phase 5 test plan (per-command byte-identical-on-default and graceful-fallback-on-partial-translation) → Each task's Step 5 (refs.py) plus the byte-identical contract from the Phase 4 conversion pattern enforce this.
- [ ] Phase 2 `notify_invis` gap → Task 0 closes it before any Phase 5 conversion touches `vwrite_level`.

**Placeholder scan:**

- Task 4 doesn't list every command — it explicitly defers to the implementer to pick batches. That's deliberate: the queue is large enough that listing every command in this plan would bloat it without adding clarity. The per-batch step list IS fully specified.
- Task 5 has a "likely candidates" list, not a definitive one. Same rationale.
- Task 6's section about splitting `src/amnuts.c` "across multiple commits by logical section" leaves the partitioning to the implementer because the right split depends on what's in `amnuts.c` at the time of the sweep.

These are deliberate hand-offs, not unfilled placeholders — each one is bounded by a specific question the implementer answers ("pick 10", "pick the big ones", "split logically") with the criteria explicit.

**Type consistency:** All conversions go from `vwrite_user(user, fmt, args)` → `lang_user(user, key, args)` with positional `%1$s` / `%2$d` specifiers in the catalog. No new types introduced. Per-command commits each ship the matching en_GB catalog entries so the refs.py cross-check stays clean.
