# Localisation Phase 2 — Catalog Framework Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the in-memory string catalog (`strings.yml` per locale), the `lang_*` API that renders catalog entries with per-recipient locale resolution and load-time format-signature safety, the persisted per-user `language` field, and the user-facing `set lang` and wizard `langreload` commands. After this phase the talker still produces byte-identical output today (no call sites use `lang_*` yet), but the machinery a translator needs is fully in place.

**Architecture:** A new module `src/catalog.c` (with a small `src/yaml_util.c` shared helper) parses each discovered locale's `strings.yml` into a `struct locale_catalog` (chained-hash table of `struct lang_entry`). The default locale loads first and locks in the canonical format signatures; non-default locales validate each key against those signatures and drop keys that don't match (instead of poisoning the server). `lang_room` and `lang_level` walk recipients and render once per user against that user's catalog, falling back to the default catalog per-key. User catalog pointers are non-owning, refreshed at login, on `set lang`, and on `langreload`. `langreload` swaps the table atomically and keeps one generation behind so in-flight callers stay safe.

**Tech Stack:** C (clang, `-std=gnu23 -g -Wall -Wextra -MMD -Wpedantic`), GNU Make, libyaml 0.2.5 (already vendored in Phase 1 at `src/vendors/libyaml/`). No test framework — verification is `make build` (clean) plus manual play-through.

**Reference spec:** `docs/superpowers/specs/2026-05-10-localisation-design.md` §3, §4, §7, §8.

**Codebase notes for the implementing engineer:**
- Phase 1 already landed: `LANGS_ROOT`, `LOCALE_NAME_LEN`, `MAX_LOCALES`, `struct locale_state` on `amsys`, `amsys->default_locale`, `user->locale[LOCALE_NAME_LEN]`, `locale_load_all()`, `locale_path()`, `locale_default_path()`, `locale_default()` are all live and merged. Do not re-implement them.
- libyaml is vendored at `src/vendors/libyaml/` and links into the talker; no Makefile change needed.
- No test framework exists in this repo. Verification is `make build` (must build with no new warnings under `-Wall -Wextra -Wpedantic`) plus manual telnet play-through.
- New functions must have prototypes added to `src/includes/prototypes.h` — that header is the single source of truth.
- X-macros are how the codebase generates enums + tables (`USERDB_LIST`, `SET_LIST`, `CMD_LIST`). When adding a key/command, add it to the X-macro and the matching switch case — don't introduce parallel data.
- `amsys` is the global `system_struct` singleton, declared in `src/includes/globals.h`.
- The user-file save/load is in `src/amnuts.c` around `save_user_details` (~3004) and `load_user_details` (~2612), driven by the `USERDB_LIST` X-macro at ~2575.
- `setstr[]` and `SET_LIST` live in `src/includes/commands.h` (~240); the dispatch is in `src/commands/set.c` (`set_attributes`).
- `CMD_LIST` is in `src/includes/commands.h` (~30); each WIZ command has its own file in `src/commands/`.
- The reboot reattach is in `src/reboot.c` (`possibly_reboot` → `retrieve_users`); it calls `load_user_details` per user, so a freshly loaded `user->locale` is present before any catalog re-resolution runs.

**Commit convention:** small, frequent commits, one task per commit. Match the existing repo style: subject line in present tense (`Add ...`, `Implement ...`, `Wire ...`), body explains _why_ in a sentence or two. Use the same `Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>` trailer used in Phase 1.

---

## File Structure

**New files:**
- `src/catalog.c` — catalog loader, hash table, lookup, signature extraction/validation, `lang_*` API.
- `src/yaml_util.c` — tiny helpers shared with any future YAML consumer (fail-fast error reporter, expect-event wrapper, scalar→string copy).
- `src/commands/set_lang.c` — handler called from `set_attributes` for the new `set lang` subcommand.
- `src/commands/langreload.c` — WIZ command body.
- `files/langs/en_GB/strings.yml` — initial catalog: only `meta.*` and a placeholder smoke-test key.

**Modified files:**
- `src/includes/globals.h` — add `struct lang_entry`, `struct locale_catalog`; embed catalog table inside `struct locale_state`; add `user->catalog` non-owning pointer.
- `src/includes/prototypes.h` — declare new catalog/`lang_*` API and the new command handlers.
- `src/includes/commands.h` — add `SETLANG` entry to `SET_LIST`; add `LANGRELOAD` entry to `CMD_LIST` (WIZ).
- `src/commands/set.c` — dispatch `SETLANG` to `set_user_lang(user)`.
- `src/amnuts.c` — add `LANGUAGE` to `USERDB_LIST`, save/load the field, resolve `user->catalog` after `load_user_details`, in `connect_user`, and in `possibly_reboot`'s reattach path. Extend `locale_load_all` to also load catalogs (Task 6).

**Unchanged from Phase 1:** `src/locale.c` keeps the path resolver and locale discovery exactly as it is today. Phase 2 grows `locale_load_all` (currently in `locale.c`) with a follow-up call into `catalog_load_all` (in `catalog.c`); no rewrite of existing helpers.

---

## Task 1: Add catalog data structures and forward decls to globals.h

**Files:**
- Modify: `src/includes/globals.h`

We define the catalog types before any code references them. `struct locale_catalog` is owned by `amsys->locales`; `user->catalog` is a non-owning pointer refreshed at login and on reload. The catalog table grows on top of the existing `struct locale_state` so Phase 1's `names[]` and `count` stay where they are.

- [ ] **Step 1: Add `struct lang_entry` and `struct locale_catalog` above `struct locale_state`.**

In `src/includes/globals.h`, locate the existing `struct locale_state { ... }` block (around line 91). Immediately before it, insert:

```c
/* One entry in a locale's string catalog. Format strings and keys are
 * strdup'd at load time and owned by the parent locale_catalog. */
struct lang_entry {
    char       *key;
    char       *fmt;
    uint8_t     arg_count;
    char        arg_types[8];      /* per-position: 'd','s','x','c'; 0=unused */
    struct lang_entry *next;        /* hash bucket chain */
};

/* Per-locale loaded catalog. Lives in amsys->locales.catalogs[i]. */
struct locale_catalog {
    char     name[LOCALE_NAME_LEN];
    bool     is_default;
    bool     loaded_ok;             /* false => dropped during load; do not use */
    int      bucket_count;          /* always a power of two; see CATALOG_BUCKETS */
    int      entry_count;           /* unique keys actually loaded */
    struct lang_entry **buckets;    /* heap-allocated, bucket_count entries */
};
```

Add `#include <stdbool.h>` and `#include <stdint.h>` near the top of the file if they aren't already pulled in transitively (search for `bool` near the top — if it compiles already without those headers, they're transitively included via `defines.h`; leave them be).

- [ ] **Step 2: Extend `struct locale_state` with the catalog table.**

Change:

```c
struct locale_state {
    char  names[MAX_LOCALES][LOCALE_NAME_LEN];
    int   count;
};
```

to:

```c
struct locale_state {
    char  names[MAX_LOCALES][LOCALE_NAME_LEN];
    int   count;

    /* Catalog table, populated by catalog_load_all().
     * `catalogs[i].name` mirrors `names[i]` for the same i. Entries with
     * loaded_ok == false were either parse-failures or non-default locales
     * with no strings.yml — lookups against them must fall back to default.
     * `default_index` is the index of the default catalog (0..count-1). */
    struct locale_catalog *catalogs;   /* heap; freed and replaced by langreload */
    int                    default_index;
};
```

- [ ] **Step 3: Add the non-owning catalog pointer to `struct user_struct`.**

In the same file, find `struct user_struct { ... }` (around line 99). Immediately after the existing `char locale[LOCALE_NAME_LEN];` line (added in Phase 1, around line 105), add:

```c
struct locale_catalog *catalog;  /* non-owning; refreshed at login + langreload */
```

`create_user` zero-inits via `calloc`, so the default is NULL — that's fine; `lang_*` will fall back to default on NULL.

- [ ] **Step 4: Build to confirm types are sound.**

Run: `make compile`

Expected: clean build. Nothing references the new types yet so this is a type-only addition.

- [ ] **Step 5: Commit.**

```bash
git add src/includes/globals.h
git commit -m "$(cat <<'EOF'
Add catalog types to globals.h

Introduce struct lang_entry and struct locale_catalog, extend
struct locale_state with a catalog table, and add a non-owning catalog
pointer to UR_OBJECT. Pure type declarations; no behaviour change.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 2: Declare the catalog and lang_* API in prototypes.h

**Files:**
- Modify: `src/includes/prototypes.h`

Declarations land first so later tasks can call into them without juggling forward decls. No implementation yet.

- [ ] **Step 1: Add a new prototypes block for `catalog.c`.**

Open `src/includes/prototypes.h`. Find the `locale.c` prototypes block (it should exist from Phase 1). Immediately after it, add:

```c
/* catalog.c */
int   catalog_load_all(struct locale_state *st);
void  catalog_free_all(struct locale_state *st);
struct locale_catalog *catalog_for_locale(const struct locale_state *st,
                                          const char *locale_name);
const struct lang_entry *catalog_lookup(const struct locale_catalog *cat,
                                        const char *key);

/* lang_*() — render a catalog entry to one or more users.
 * `key` resolution: user->catalog first, then amsys default catalog.
 * Missing-in-both keys emit a visible "[??? key]\n" marker and rate-limit
 * the warning to the system log. */
const char *lang(UR_OBJECT user, const char *key);
void  lang_user (UR_OBJECT user,  const char *key, ...);
void  lang_room (RM_OBJECT room,  UR_OBJECT exclude, const char *key, ...);
void  lang_level(int min_level, int notify_invis, int record_flag,
                 UR_OBJECT exclude, const char *key, ...);
int   lang_format(UR_OBJECT user, char *buf, size_t buflen,
                  const char *key, ...);

/* Lifecycle helpers reused by `set lang` and `langreload`. */
int   locale_set_user(UR_OBJECT user, const char *name);
void  locale_list    (UR_OBJECT user);
void  locale_resolve_catalog(UR_OBJECT user);  /* set user->catalog from user->locale */
```

The `lang_level` signature mirrors the existing `vwrite_level` (see `src/messages.c`) so converters can swap one for the other without re-arranging arguments — keep the same parameter order.

- [ ] **Step 2: Add the two new command handler prototypes.**

In the same file, in the section that lists per-command prototypes (one block per `src/commands/<name>.c` file), add:

```c
/* set_lang.c */
void set_user_lang(UR_OBJECT user);

/* langreload.c */
void langreload(UR_OBJECT user);
```

- [ ] **Step 3: Add the tiny shared-YAML helpers prototype block.**

```c
/* yaml_util.c */
struct yaml_parser_s;  /* opaque to most callers — actually yaml_parser_t */
void yaml_die(const char *path, void *parser, const char *fmt, ...)
    __attribute__((format(printf, 3, 4), noreturn));
void yaml_next(const char *path, void *parser, void *event_out);
const char *yaml_event_kind(int event_type);
```

Use `void *` in the prototype rather than `yaml_parser_t *` so callers that don't already include `<yaml.h>` aren't forced to. The implementation casts internally.

- [ ] **Step 4: Build.**

Run: `make compile`

Expected: clean build. Adding prototypes alone doesn't require any implementation to link, because nothing calls them yet.

- [ ] **Step 5: Commit.**

```bash
git add src/includes/prototypes.h
git commit -m "$(cat <<'EOF'
Declare catalog, lang_*, and helper API surface

Pure declarations so later tasks can implement against a fixed interface.
Mirrors vwrite_level's parameter order in lang_level to make later
sweep conversions a one-liner per call site.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 3: Implement the YAML helper module

**Files:**
- Create: `src/yaml_util.c`

A 50-line helper used by `catalog.c`. Centralises error reporting (single `file:line:col: message` format) and event walking so the catalog loader stays readable.

- [ ] **Step 1: Create `src/yaml_util.c`.**

```c
/****************************************************************************
   Amnuts — tiny libyaml helpers used by catalog.c and any future loaders.
 ***************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "globals.h"
#include "prototypes.h"

#include "../vendors/libyaml/yaml.h"

void
yaml_die(const char *path, void *parser_v, const char *fmt, ...)
{
    va_list ap;
    yaml_parser_t *parser = (yaml_parser_t *) parser_v;

    fprintf(stderr, "%s", path);
    if (parser) {
        fprintf(stderr, ":%lu:%lu",
                (unsigned long) parser->problem_mark.line + 1,
                (unsigned long) parser->problem_mark.column + 1);
    }
    fprintf(stderr, ": ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    if (parser && parser->problem) {
        fprintf(stderr, "  libyaml: %s\n", parser->problem);
    }
    boot_exit(1);
}

void
yaml_next(const char *path, void *parser_v, void *event_out_v)
{
    yaml_parser_t *parser = (yaml_parser_t *) parser_v;
    yaml_event_t  *event  = (yaml_event_t  *) event_out_v;
    if (!yaml_parser_parse(parser, event)) {
        yaml_die(path, parser, "YAML parse failure");
    }
}

const char *
yaml_event_kind(int event_type)
{
    switch ((yaml_event_type_t) event_type) {
    case YAML_NO_EVENT:             return "none";
    case YAML_STREAM_START_EVENT:   return "stream-start";
    case YAML_STREAM_END_EVENT:     return "stream-end";
    case YAML_DOCUMENT_START_EVENT: return "document-start";
    case YAML_DOCUMENT_END_EVENT:   return "document-end";
    case YAML_ALIAS_EVENT:          return "alias";
    case YAML_SCALAR_EVENT:         return "scalar";
    case YAML_SEQUENCE_START_EVENT: return "sequence-start";
    case YAML_SEQUENCE_END_EVENT:   return "sequence-end";
    case YAML_MAPPING_START_EVENT:  return "mapping-start";
    case YAML_MAPPING_END_EVENT:    return "mapping-end";
    }
    return "unknown";
}
```

- [ ] **Step 2: Build.**

Run: `make compile`

Expected: `src/yaml_util.c` compiles. The Makefile auto-picks any `src/*.c` because the existing `wildcard` rule covers it. No new warnings.

- [ ] **Step 3: Commit.**

```bash
git add src/yaml_util.c
git commit -m "$(cat <<'EOF'
Implement shared YAML helpers (yaml_util.c)

Single fail-fast error reporter and a typed event-name lookup, shared
between catalog.c and any future YAML loaders. No callers yet.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 4: Implement the catalog hash table + lookup

**Files:**
- Create: `src/catalog.c` (begin — add the static hash-table internals only)

Just the storage layer. No YAML, no `lang_*`, no validation yet. We use chained hashing because the spec specifies that layout and because Phase 2's catalogs are typically ≤500 keys — chained hashing keeps memory and code small.

- [ ] **Step 1: Create `src/catalog.c` with the skeleton + hash internals.**

```c
/****************************************************************************
   Amnuts — string catalog (Mechanism 1 of the localisation design).
   See docs/superpowers/specs/2026-05-10-localisation-design.md §4.
 ***************************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "defines.h"
#include "globals.h"
#include "prototypes.h"

#include "../vendors/libyaml/yaml.h"

/* Bucket count must be a power of two; 256 gives plenty of headroom for the
 * ~1500 keys the design anticipates at full conversion, with reasonable
 * load factor at the meta-only state we ship today. */
#define CATALOG_BUCKETS 256

/* FNV-1a 32-bit. Stable across runs; only used inside one process so we don't
 * care about cryptographic strength or cross-arch portability. */
static uint32_t
catalog_hash(const char *key)
{
    uint32_t h = 0x811c9dc5u;
    for (const unsigned char *p = (const unsigned char *) key; *p; ++p) {
        h ^= *p;
        h *= 0x01000193u;
    }
    return h;
}

static struct lang_entry **
catalog_alloc_buckets(int n)
{
    struct lang_entry **b = calloc((size_t) n, sizeof *b);
    if (!b) {
        fprintf(stderr, "Amnuts: out of memory allocating catalog buckets.\n");
        boot_exit(1);
    }
    return b;
}

static void
catalog_insert(struct locale_catalog *cat, struct lang_entry *e)
{
    int idx = (int) (catalog_hash(e->key) & (uint32_t) (cat->bucket_count - 1));
    e->next = cat->buckets[idx];
    cat->buckets[idx] = e;
    cat->entry_count++;
}

const struct lang_entry *
catalog_lookup(const struct locale_catalog *cat, const char *key)
{
    if (!cat || !cat->loaded_ok || !cat->buckets) return NULL;
    int idx = (int) (catalog_hash(key) & (uint32_t) (cat->bucket_count - 1));
    for (const struct lang_entry *e = cat->buckets[idx]; e; e = e->next) {
        if (!strcmp(e->key, key)) return e;
    }
    return NULL;
}

struct locale_catalog *
catalog_for_locale(const struct locale_state *st, const char *locale_name)
{
    if (!st || !st->catalogs || !locale_name || !*locale_name) return NULL;
    for (int i = 0; i < st->count; ++i) {
        if (!strcmp(st->catalogs[i].name, locale_name)) {
            return &st->catalogs[i];
        }
    }
    return NULL;
}

/* Stubs filled in by later tasks. Defined here so the link still works. */
int
catalog_load_all(struct locale_state *st)
{
    (void) st;
    /* Task 6 fills this in. */
    return 0;
}

void
catalog_free_all(struct locale_state *st)
{
    if (!st || !st->catalogs) return;
    for (int i = 0; i < st->count; ++i) {
        struct locale_catalog *cat = &st->catalogs[i];
        if (!cat->buckets) continue;
        for (int b = 0; b < cat->bucket_count; ++b) {
            struct lang_entry *e = cat->buckets[b];
            while (e) {
                struct lang_entry *next = e->next;
                free(e->key);
                free(e->fmt);
                free(e);
                e = next;
            }
        }
        free(cat->buckets);
        cat->buckets = NULL;
    }
    free(st->catalogs);
    st->catalogs = NULL;
}
```

- [ ] **Step 2: Build.**

Run: `make compile`

Expected: `catalog.o` compiles cleanly. Unused-static warnings on `catalog_alloc_buckets`/`catalog_insert` are acceptable for this commit only — the next tasks consume them. If the compiler treats them as errors under `-Wall -Wextra`, add `__attribute__((unused))` temporarily and remove it in Task 5.

- [ ] **Step 3: Commit.**

```bash
git add src/catalog.c
git commit -m "$(cat <<'EOF'
Catalog hash table skeleton: storage and lookup

FNV-1a hash, 256 chained buckets per locale, lookup + bulk free.
catalog_load_all is a stub; signature extraction and YAML loading
land in the following commits.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 5: Implement format-signature extraction and validation

**Files:**
- Modify: `src/catalog.c` (add static signature helpers)

The validator parses one catalog format string and returns `(arg_count, arg_types[8])` or fails. The default locale's signatures are the source of truth; non-default keys are dropped if they don't match. Reject `%n` everywhere. Positional specifiers only (`%1$s` form) — accept legacy non-positional (`%s` etc.) for the meta-only initial catalog but treat them as positional in declaration order.

- [ ] **Step 1: Add the signature parser.**

Append to `src/catalog.c`:

```c
/*
 * Walk a printf-style format string. Populates *out_count with the highest
 * argument position referenced and *out_types[8] with the conversion
 * character per position. Returns 0 on success, -1 on disallowed input
 * (errmsg points at a static string explaining what was wrong).
 *
 * Accepted conversions: d, i, u, x, X, o (numeric); s, c (string/char).
 * Reject: n (write-back), and any position > 8.
 * Positional %M$X is required when M > 1 references exist; bare %X is
 * accepted and assigned implicit positions in declaration order.
 */
static int
catalog_extract_signature(const char *fmt,
                          uint8_t *out_count, char out_types[8],
                          const char **errmsg)
{
    int implicit_pos = 0;
    int positional_mode = 0;
    int implicit_mode = 0;
    int max_pos = 0;
    char types[8] = {0};

    for (const char *p = fmt; *p; ++p) {
        if (*p != '%') continue;
        ++p;
        if (*p == '%') continue;             /* literal %% */
        if (!*p) { *errmsg = "trailing %"; return -1; }

        /* Parse optional position. */
        int pos = 0;
        if (isdigit((unsigned char) *p)) {
            const char *q = p;
            int n = 0;
            while (isdigit((unsigned char) *q)) {
                n = n * 10 + (*q - '0');
                ++q;
            }
            if (*q == '$') {
                pos = n;
                p = q + 1;
                positional_mode = 1;
            }
            /* else: digits were a width spec; pos stays 0 */
        }
        if (pos == 0) {
            pos = ++implicit_pos;
            implicit_mode = 1;
        }
        if (positional_mode && implicit_mode) {
            *errmsg = "mixed positional and implicit specifiers";
            return -1;
        }
        if (pos < 1 || pos > 8) {
            *errmsg = "argument position out of range (1..8)";
            return -1;
        }

        /* Skip flags / width / precision. */
        while (*p == '-' || *p == '+' || *p == ' ' || *p == '#' || *p == '0') ++p;
        while (isdigit((unsigned char) *p)) ++p;
        if (*p == '.') {
            ++p;
            while (isdigit((unsigned char) *p)) ++p;
        }
        /* Length modifiers we ignore for signature purposes. */
        while (*p == 'h' || *p == 'l' || *p == 'L' || *p == 'z' || *p == 'j' || *p == 't') ++p;

        char conv = *p;
        char canonical;
        switch (conv) {
        case 'd': case 'i': case 'u':
        case 'x': case 'X': case 'o':
            canonical = 'd';
            break;
        case 's':
            canonical = 's';
            break;
        case 'c':
            canonical = 'c';
            break;
        case 'n':
            *errmsg = "%n disallowed in catalog format";
            return -1;
        default:
            *errmsg = "unsupported conversion character";
            return -1;
        }
        if (types[pos - 1] && types[pos - 1] != canonical) {
            *errmsg = "argument type conflict between positions";
            return -1;
        }
        types[pos - 1] = canonical;
        if (pos > max_pos) max_pos = pos;
    }

    /* Holes are allowed (a translator could position-skip), but contiguous
     * coverage is what we'll check against later when validating a
     * translation against the default. */
    memcpy(out_types, types, sizeof types);
    *out_count = (uint8_t) max_pos;
    return 0;
}

/*
 * Compare a non-default translation's signature against the default's.
 * Returns 0 if compatible (translation refers only to positions the
 * default declares, and uses canonical-compatible types), -1 if not.
 *
 * Compatibility rules:
 *   - Translation may use FEWER positions than default (the omitted ones
 *     stay un-rendered; printf will pad with nothing — harmless for us).
 *   - Translation MUST NOT use a position > default->arg_count.
 *   - Per-position canonical types must match exactly ('d', 's', 'c').
 */
static int
catalog_signature_compatible(const struct lang_entry *def,
                             const struct lang_entry *trans,
                             const char **errmsg)
{
    if (trans->arg_count > def->arg_count) {
        *errmsg = "translation references more arguments than default";
        return -1;
    }
    for (int i = 0; i < trans->arg_count; ++i) {
        if (trans->arg_types[i] == 0) continue;
        if (def->arg_types[i] == 0) {
            *errmsg = "translation uses an argument position the default skips";
            return -1;
        }
        if (trans->arg_types[i] != def->arg_types[i]) {
            *errmsg = "translation argument type differs from default";
            return -1;
        }
    }
    return 0;
}
```

- [ ] **Step 2: Build.**

Run: `make compile`

Expected: clean. Same unused-static caveat as Task 4 — Task 6 calls these.

- [ ] **Step 3: Commit.**

```bash
git add src/catalog.c
git commit -m "$(cat <<'EOF'
Catalog format-signature extraction and compatibility check

Parses printf-style format strings into (arg_count, arg_types[]);
rejects %n unconditionally; compares non-default translations
against the default's signature so mismatched keys can be dropped
at load time rather than corrupting runtime output.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 6: Implement the catalog YAML loader

**Files:**
- Modify: `src/catalog.c` (replace the `catalog_load_all` stub)

Parses each locale's `strings.yml` into a `struct locale_catalog`. Default goes first; non-defaults validate each key against the default's signature.

- [ ] **Step 1: Add the per-file loader.**

Insert this above the existing `catalog_load_all` stub in `src/catalog.c`:

```c
static void
catalog_log_drop(const char *locale_name, const char *key,
                 const char *path, int line, int col, const char *why)
{
    write_syslog(SYSLOG | ERRLOG, 0,
                 "[locale] %s/strings.yml: key '%s' dropped at %d:%d — %s (path %s)\n",
                 locale_name, key ? key : "<top-level>", line, col, why, path);
}

/*
 * Load one locale's strings.yml into `cat`. Sets cat->loaded_ok on success
 * (including the partial-success case where some keys were dropped).
 * Sets cat->loaded_ok = false only on top-level parse failure or missing
 * file, in which case the catalog stays empty and lookups fall back.
 *
 * If `default_cat` is non-NULL, each successfully-parsed entry is validated
 * against the default's signature; mismatched entries are dropped + logged
 * but do not fail the whole locale.
 */
static void
catalog_load_one(struct locale_catalog *cat,
                 const struct locale_catalog *default_cat)
{
    char path[1024];
    snprintf(path, sizeof path, "%s/%s/strings.yml",
             LANGS_ROOT, cat->name);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        /* Missing strings.yml is allowed for non-default locales — the
         * catalog stays empty and every lookup falls back to the default.
         * For the default, the gate in catalog_load_all enforces presence. */
        cat->bucket_count = CATALOG_BUCKETS;
        cat->buckets = catalog_alloc_buckets(cat->bucket_count);
        cat->loaded_ok = true;
        return;
    }

    yaml_parser_t parser;
    yaml_event_t  event;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, fp);

    cat->bucket_count = CATALOG_BUCKETS;
    cat->buckets = catalog_alloc_buckets(cat->bucket_count);

    yaml_next(path, &parser, &event);
    if (event.type != YAML_STREAM_START_EVENT) {
        yaml_die(path, &parser, "expected stream-start, got %s",
                 yaml_event_kind(event.type));
    }
    yaml_event_delete(&event);

    yaml_next(path, &parser, &event);
    if (event.type == YAML_STREAM_END_EVENT) {
        /* empty file — same as missing. */
        yaml_event_delete(&event);
        goto done_ok;
    }
    if (event.type != YAML_DOCUMENT_START_EVENT) {
        yaml_die(path, &parser, "expected document-start, got %s",
                 yaml_event_kind(event.type));
    }
    yaml_event_delete(&event);

    yaml_next(path, &parser, &event);
    if (event.type != YAML_MAPPING_START_EVENT) {
        yaml_die(path, &parser,
                 "strings.yml top-level must be a mapping (got %s)",
                 yaml_event_kind(event.type));
    }
    yaml_event_delete(&event);

    for (;;) {
        /* key */
        yaml_event_t key_ev;
        yaml_next(path, &parser, &key_ev);
        if (key_ev.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key_ev);
            break;
        }
        if (key_ev.type != YAML_SCALAR_EVENT) {
            yaml_die(path, &parser, "expected scalar key, got %s",
                     yaml_event_kind(key_ev.type));
        }

        /* value — only scalars supported in Phase 2. Sequence/map values
         * would imply structure we don't have a use case for yet. */
        yaml_event_t val_ev;
        yaml_next(path, &parser, &val_ev);
        if (val_ev.type != YAML_SCALAR_EVENT) {
            yaml_die(path, &parser,
                     "value for key '%s' must be a scalar string (got %s)",
                     (const char *) key_ev.data.scalar.value,
                     yaml_event_kind(val_ev.type));
        }

        const char *k = (const char *) key_ev.data.scalar.value;
        const char *v = (const char *) val_ev.data.scalar.value;

        /* Extract signature. */
        struct lang_entry tmp;
        memset(&tmp, 0, sizeof tmp);
        const char *why = NULL;
        if (catalog_extract_signature(v, &tmp.arg_count, tmp.arg_types, &why) < 0) {
            catalog_log_drop(cat->name, k, path,
                             (int) parser.mark.line + 1,
                             (int) parser.mark.column + 1, why);
            yaml_event_delete(&key_ev);
            yaml_event_delete(&val_ev);
            continue;
        }

        /* If non-default, validate against default. */
        if (default_cat) {
            const struct lang_entry *def = catalog_lookup(default_cat, k);
            if (def) {
                if (catalog_signature_compatible(def, &tmp, &why) < 0) {
                    catalog_log_drop(cat->name, k, path,
                                     (int) parser.mark.line + 1,
                                     (int) parser.mark.column + 1, why);
                    yaml_event_delete(&key_ev);
                    yaml_event_delete(&val_ev);
                    continue;
                }
            }
            /* Keys not in the default catalog are allowed in translations
             * — they're harmless (no caller will look them up) but we log
             * them so translators see drift. */
        }

        /* Reject duplicate keys: first wins, log the duplicate. */
        if (catalog_lookup(cat, k)) {
            catalog_log_drop(cat->name, k, path,
                             (int) parser.mark.line + 1,
                             (int) parser.mark.column + 1,
                             "duplicate key (first definition kept)");
            yaml_event_delete(&key_ev);
            yaml_event_delete(&val_ev);
            continue;
        }

        struct lang_entry *e = calloc(1, sizeof *e);
        if (!e) {
            fprintf(stderr, "Amnuts: out of memory loading catalog %s.\n",
                    cat->name);
            boot_exit(1);
        }
        e->key       = strdup(k);
        e->fmt       = strdup(v);
        e->arg_count = tmp.arg_count;
        memcpy(e->arg_types, tmp.arg_types, sizeof tmp.arg_types);
        if (!e->key || !e->fmt) {
            fprintf(stderr, "Amnuts: out of memory loading catalog %s.\n",
                    cat->name);
            boot_exit(1);
        }
        catalog_insert(cat, e);

        yaml_event_delete(&key_ev);
        yaml_event_delete(&val_ev);
    }

    /* Drain to stream-end. */
    do {
        yaml_next(path, &parser, &event);
        yaml_event_delete(&event);
    } while (event.type != YAML_STREAM_END_EVENT);

done_ok:
    yaml_parser_delete(&parser);
    fclose(fp);
    cat->loaded_ok = true;
}
```

- [ ] **Step 2: Replace the `catalog_load_all` stub.**

Replace the stub from Task 4 with:

```c
int
catalog_load_all(struct locale_state *st)
{
    if (!st || st->count <= 0) return 0;

    st->catalogs = calloc((size_t) st->count, sizeof *st->catalogs);
    if (!st->catalogs) {
        fprintf(stderr, "Amnuts: out of memory allocating catalog array.\n");
        boot_exit(1);
    }
    st->default_index = -1;

    /* Mirror names[] into catalogs[].name; mark the default. */
    for (int i = 0; i < st->count; ++i) {
        strncpy(st->catalogs[i].name, st->names[i],
                LOCALE_NAME_LEN - 1);
        st->catalogs[i].name[LOCALE_NAME_LEN - 1] = '\0';
        if (!strcmp(st->catalogs[i].name, amsys->default_locale)) {
            st->catalogs[i].is_default = true;
            st->default_index = i;
        }
    }
    if (st->default_index < 0) {
        /* locale_load_all should have caught this; defensive. */
        fprintf(stderr, "Amnuts: default locale '%s' missing from catalog table.\n",
                amsys->default_locale);
        boot_exit(1);
    }

    /* Load default first so non-defaults can validate against it. */
    catalog_load_one(&st->catalogs[st->default_index], NULL);

    /* Default's strings.yml MUST exist and parse; otherwise hard-fail. */
    if (!st->catalogs[st->default_index].loaded_ok
        || st->catalogs[st->default_index].entry_count == 0) {
        char path[1024];
        snprintf(path, sizeof path, "%s/%s/strings.yml",
                 LANGS_ROOT, amsys->default_locale);
        if (access(path, R_OK) != 0) {
            fprintf(stderr,
                    "Amnuts: default locale '%s' has no readable strings.yml at %s.\n",
                    amsys->default_locale, path);
            boot_exit(1);
        }
        /* File exists but is empty — that's allowed for the meta-only ship state. */
    }

    /* Load non-defaults. */
    for (int i = 0; i < st->count; ++i) {
        if (i == st->default_index) continue;
        catalog_load_one(&st->catalogs[i], &st->catalogs[st->default_index]);
    }
    return 0;
}
```

At the top of `catalog.c` add `#include <unistd.h>` for `access()`.

- [ ] **Step 3: Wire `catalog_load_all` into `locale_load_all`.**

Open `src/locale.c`. After the existing `printf("Localisation: discovered ...");` line near the end of `locale_load_all`, before `return 0;`, add:

```c
    if (catalog_load_all(&amsys->locales) != 0) {
        fprintf(stderr, "Amnuts: catalog load failed; aborting.\n");
        boot_exit(1);
    }
    printf("Localisation: catalog loaded (%d locale(s)).\n",
           amsys->locales.count);
```

- [ ] **Step 4: Build.**

Run: `make compile`

Expected: clean. No call-sites use the catalog yet, but it loads at boot. The talker won't boot end-to-end without a `strings.yml` — that's intentional and matches the spec's hard-fail rule. Task 7 ships the file.

- [ ] **Step 5: Commit.**

```bash
git add src/catalog.c src/locale.c
git commit -m "$(cat <<'EOF'
Implement catalog YAML loader and wire into locale_load_all

Default locale loads first and seeds canonical signatures; non-default
locales validate per-key against those and drop incompatible entries
with a log line. Missing strings.yml is fatal only for the default.

The talker will refuse to boot until files/langs/en_GB/strings.yml is
in place — that ships in the next commit.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 7: Ship the initial en_GB/strings.yml (meta + smoke-test key)

**Files:**
- Create: `files/langs/en_GB/strings.yml`

The default catalog ships with just `meta.*` plus one tiny smoke-test key. Real translations of in-source literals arrive in Phase 5.

- [ ] **Step 1: Create the file.**

```yaml
# Amnuts default locale — British English.
# See docs/superpowers/specs/2026-05-10-localisation-design.md §4 for
# format rules. Keys are dot-namespaced; values are printf format strings
# using POSIX positional specifiers (%1$s, %2$d). The default locale is
# the canonical source of format signatures — every other locale's
# translation of a key MUST match this one's argument count and types.

# Locale metadata, used by `set lang` listings.
meta.name:        "British English"
meta.description: "The default tongue."

# A smoke-test key proves the catalog round-trip works.
# Phase 2 has no other code-side consumers; Phase 4 starts adding real ones.
ping.greeting:    "Hello, %1$s. Catalog is working.\n"
```

- [ ] **Step 2: Boot the talker to confirm the catalog loads.**

```bash
make build
./amnutsTalker -p 12345
```

Expected: startup prints both lines:

```
Localisation: discovered 2 locale(s); default = en_GB.
Localisation: catalog loaded (2 locale(s)).
```

(Two locales because Phase 1's `files/langs/fallback_test/` is still present.) No errors. Ctrl-C the talker.

- [ ] **Step 3: Commit.**

```bash
git add files/langs/en_GB/strings.yml
git commit -m "$(cat <<'EOF'
Ship en_GB/strings.yml with meta.* and a smoke-test key

Just enough content for the catalog loader to find a non-empty file
during boot. Real translations of in-source literals are Phase 5's
job; Phase 2 only proves the framework runs.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 8: Implement `lang`, `lang_format`, and `lang_user`

**Files:**
- Modify: `src/catalog.c`

The single-user render path: look up in user catalog, fall back to default, render via `vsnprintf` into a local buffer, write to the user. `lang_format` does the same but writes to a caller-provided buffer instead of the socket — used by UI builders in Phase 3.

- [ ] **Step 1: Add a rate-limited warn helper.**

Append to `src/catalog.c`:

```c
/* Rate-limit missing-key syslog noise to once per key per ~30s. We keep a
 * tiny ring buffer rather than a hash; if a hot path is missing N keys,
 * we'll see each at least once within the window. */
#define MISSING_WARN_WINDOW 30
#define MISSING_WARN_SLOTS  16

static struct {
    char     key[64];
    time_t   last;
} missing_warns[MISSING_WARN_SLOTS];

static void
catalog_warn_missing(const char *key)
{
    time_t now = time(NULL);
    int    free_slot = -1;
    int    oldest_slot = 0;
    time_t oldest_time = missing_warns[0].last;

    for (int i = 0; i < MISSING_WARN_SLOTS; ++i) {
        if (!*missing_warns[i].key) {
            if (free_slot < 0) free_slot = i;
            continue;
        }
        if (!strcmp(missing_warns[i].key, key)) {
            if (now - missing_warns[i].last < MISSING_WARN_WINDOW) return;
            missing_warns[i].last = now;
            write_syslog(SYSLOG, 0, "[locale] missing key: %s\n", key);
            return;
        }
        if (missing_warns[i].last < oldest_time) {
            oldest_time = missing_warns[i].last;
            oldest_slot = i;
        }
    }
    int slot = free_slot >= 0 ? free_slot : oldest_slot;
    strncpy(missing_warns[slot].key, key, sizeof missing_warns[slot].key - 1);
    missing_warns[slot].key[sizeof missing_warns[slot].key - 1] = '\0';
    missing_warns[slot].last = now;
    write_syslog(SYSLOG, 0, "[locale] missing key: %s\n", key);
}
```

- [ ] **Step 2: Add the catalog resolution + lookup-with-fallback helper.**

```c
static const struct locale_catalog *
catalog_default(void)
{
    if (!amsys || !amsys->locales.catalogs) return NULL;
    if (amsys->locales.default_index < 0) return NULL;
    return &amsys->locales.catalogs[amsys->locales.default_index];
}

/* Resolve a key against (user_cat, default_cat). Returns the format string
 * or NULL if missing in both. Logs missing-key the first time per window. */
static const char *
catalog_resolve(const struct locale_catalog *user_cat, const char *key)
{
    const struct lang_entry *e;
    if (user_cat && (e = catalog_lookup(user_cat, key)) != NULL) {
        return e->fmt;
    }
    const struct locale_catalog *def = catalog_default();
    if (def && (e = catalog_lookup(def, key)) != NULL) {
        return e->fmt;
    }
    catalog_warn_missing(key);
    return NULL;
}
```

- [ ] **Step 3: Implement `lang`, `lang_format`, `lang_user`.**

```c
const char *
lang(UR_OBJECT user, const char *key)
{
    return catalog_resolve(user ? user->catalog : NULL, key);
}

int
lang_format(UR_OBJECT user, char *buf, size_t buflen,
            const char *key, ...)
{
    const char *fmt = catalog_resolve(user ? user->catalog : NULL, key);
    if (!fmt) {
        return snprintf(buf, buflen, "[??? %s]\n", key);
    }
    va_list ap;
    va_start(ap, key);
    int n = vsnprintf(buf, buflen, fmt, ap);
    va_end(ap);
    return n;
}

void
lang_user(UR_OBJECT user, const char *key, ...)
{
    if (!user) return;
    const char *fmt = catalog_resolve(user->catalog, key);
    char buf[ARR_SIZE * 2];
    if (!fmt) {
        snprintf(buf, sizeof buf, "[??? %s]\n", key);
        write_user(user, buf);
        return;
    }
    va_list ap;
    va_start(ap, key);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    write_user(user, buf);
}
```

`ARR_SIZE` is the existing project-wide line buffer constant (see `defines.h`). Two of them is the same headroom `vwrite_user` itself uses (look at its implementation if you want to confirm — it's also in `src/amnuts.c`).

- [ ] **Step 4: Build.**

Run: `make compile`

Expected: clean. No call site uses `lang_*` yet, so the link doesn't pull in anything new on the binary.

- [ ] **Step 5: Commit.**

```bash
git add src/catalog.c
git commit -m "$(cat <<'EOF'
Implement lang, lang_format, and lang_user

Single-user render path: try user catalog, fall back to default,
vsnprintf into a stack buffer, write to the user. Missing keys
emit a visible [??? key] marker and rate-limited syslog warn.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 9: Implement `lang_room` and `lang_level`

**Files:**
- Modify: `src/catalog.c`

The broadcast variants. Spec §4.4 calls out that we must render per recipient because each recipient may be on a different locale; we capture the va_list once and `va_copy` for each user.

- [ ] **Step 1: Add `lang_room`.**

Append to `src/catalog.c`:

```c
void
lang_room(RM_OBJECT room, UR_OBJECT exclude, const char *key, ...)
{
    if (!room) return;

    va_list ap0;
    va_start(ap0, key);

    for (UR_OBJECT u = user_first; u; u = u->next) {
        if (u->type == CLONE_TYPE) continue;
        if (u == exclude) continue;
        if (u->room != room) continue;
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
    }
    va_end(ap0);
}
```

- [ ] **Step 2: Add `lang_level`.**

```c
void
lang_level(int min_level, int notify_invis, int record_flag,
           UR_OBJECT exclude, const char *key, ...)
{
    (void) record_flag;   /* parameter parity with vwrite_level; not used today */
    va_list ap0;
    va_start(ap0, key);

    for (UR_OBJECT u = user_first; u; u = u->next) {
        if (u->type == CLONE_TYPE) continue;
        if (u == exclude) continue;
        if (u->level < (enum lvl_value) min_level) continue;
        if (!notify_invis && u->vis == 0 && u != exclude) {
            /* If notify_invis is 0, hide the message from the invisible
             * recipient — matches the existing vwrite_level semantics
             * (cf. src/messages.c). Skip nothing for the visible case. */
        }
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
    }
    va_end(ap0);
}
```

**Important:** `vwrite_level`'s real semantics for `notify_invis` and `record_flag` live in `src/messages.c`. Before the next sweep phase converts a real call site, the implementer must read that function and update the `lang_level` body so behaviour stays identical. For now, Phase 2 has no callers, so the simplified body above is sufficient to compile and unit-smoke-test.

- [ ] **Step 3: Build.**

Run: `make compile`

Expected: clean.

- [ ] **Step 4: Commit.**

```bash
git add src/catalog.c
git commit -m "$(cat <<'EOF'
Implement lang_room and lang_level broadcast variants

Each recipient renders against their own catalog; va_copy keeps the
caller's va_list intact across iterations. lang_level mirrors
vwrite_level's parameter order so Phase 5 conversions are mechanical.

The notify_invis / record_flag handling is simplified for Phase 2
where no real call sites exist yet; the first Phase 5 conversion that
needs them must port the full semantics from src/messages.c.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 10: Persist `user->locale` to the user file

**Files:**
- Modify: `src/amnuts.c` (USERDB_LIST X-macro, save_user_details, load_user_details)

Phase 1 added the `user->locale[]` field but did NOT persist it. We extend the user-file format with a new optional line `language <name>` that round-trips through save/load. Backwards-compat: old files that don't have the line load with `user->locale[0] = '\0'` (default), exactly today's behaviour.

- [ ] **Step 1: Add `LANGUAGE` to `USERDB_LIST`.**

In `src/amnuts.c`, find the `USERDB_LIST` X-macro (around line 2575). Append a new entry immediately before the `COUNT` sentinel:

```c
  ML_ENTRY((LANGUAGE,      "language"    )) \
```

The line should be inserted between the existing `GAME_SETTINGS` entry and the `COUNT` sentinel. Order is significant only for backwards compatibility — putting `LANGUAGE` at the end means old user files (which never wrote it) still load without warnings.

- [ ] **Step 2: Write the field in `save_user_details`.**

In `src/amnuts.c`, find `save_user_details` (around line 3004). Just before the trailing `fclose(fp);` line, add:

```c
    if (*user->locale) {
        fprintf(fp, "%-13.13s %s\n", userfile_options[USERDB_LANGUAGE],
                user->locale);
    }
```

We only write the line when the user has an explicit locale set. Default-locale users persist with no `language` line — keeps existing user files byte-identical on save when no one has run `set lang`.

- [ ] **Step 3: Read the field in `load_user_details`.**

In `src/amnuts.c`, find the giant switch in `load_user_details` (around line 2668, just after `switch ((enum userdb_value) (option - userfile_options))`). Add a new case right before the closing brace of the switch (i.e., right before `case USERDB_GAME_SETTINGS:`'s neighbours — find the alphabetical neighbour or just place it last before `default:`/end-of-switch):

```c
        case USERDB_LANGUAGE:
            if (wcnt >= 2) {
                strncpy(user->locale, user_words[1], LOCALE_NAME_LEN - 1);
                user->locale[LOCALE_NAME_LEN - 1] = '\0';
            }
            break;
```

- [ ] **Step 4: Build and smoke-test.**

```bash
make build
./amnutsTalker -p 12345
```

Telnet in as a normal user, log out cleanly (so `save_user_details` runs). Inspect the user file:

```bash
ls files/userfiles/<your-test-user>.D
```

Expected: file present; should _not_ contain a `language` line (because no one set it). The file should otherwise be byte-identical to before this change.

Stop the talker.

- [ ] **Step 5: Test the round-trip manually.**

Edit `files/userfiles/<your-test-user>.D` with a text editor. Add a line near the bottom:

```
language      fallback_test
```

Boot the talker, log in as that user, then in the talker shell type `quit` to log out cleanly. Re-inspect the file — the `language fallback_test` line should still be present (round-trip preserved). Boot again, log in, then in the talker check that `user->locale` got loaded by typing any random text — at this point you can't introspect it from the in-game side (no `set lang` yet), so the verification is really just "no crash, no warning". Stop the talker.

Restore the user file by removing the manually-added line.

- [ ] **Step 6: Commit.**

```bash
git add src/amnuts.c
git commit -m "$(cat <<'EOF'
Persist user->locale to user files via USERDB_LIST

Adds a new optional 'language' line at the end of the user file.
Only written when the user has set a non-default locale, so existing
user files stay byte-identical until someone runs `set lang`.
Old files without the line still load — user->locale stays empty
and the existing default-locale path is taken.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 11: Resolve `user->catalog` after login, save, and reboot

**Files:**
- Modify: `src/catalog.c` (implement `locale_resolve_catalog`)
- Modify: `src/amnuts.c` (call it from the login + reboot paths)

`user->catalog` must point at a valid `locale_catalog` whenever `lang_*` might be called on that user — login is the obvious moment, and the reboot reattach is the second one.

- [ ] **Step 1: Implement `locale_resolve_catalog` in `src/catalog.c`.**

Append:

```c
/*
 * Point user->catalog at the user's locale's catalog, falling back to
 * default. If the user's named locale no longer exists (admin removed
 * the directory between writes), clear user->locale and log it.
 */
void
locale_resolve_catalog(UR_OBJECT user)
{
    if (!user) return;
    if (*user->locale) {
        struct locale_catalog *cat =
            catalog_for_locale(&amsys->locales, user->locale);
        if (cat) {
            user->catalog = cat;
            return;
        }
        /* User has a locale name that no longer exists. */
        write_syslog(SYSLOG, 0,
                     "[locale] user %s locale '%s' no longer available; resetting to default.\n",
                     user->name, user->locale);
        user->locale[0] = '\0';
    }
    if (amsys->locales.default_index >= 0) {
        user->catalog = &amsys->locales.catalogs[amsys->locales.default_index];
    } else {
        user->catalog = NULL;
    }
}
```

- [ ] **Step 2: Call it from the connect-user path.**

In `src/amnuts.c`, locate the connect/login code around line 4498 where `load_user_details(user)` runs and the user is being attached. Immediately after a successful `load_user_details` call in the login flow, add:

```c
        locale_resolve_catalog(user);
```

There are two call sites in `src/amnuts.c` that call `load_user_details` for an actually-connecting user (search the function for both `if (!load_user_details(u))` at ~3397 and `if (!load_user_details(user))` at ~4498). The resolve call belongs in both — but be careful: the ~3397 one is inside `delete_user` / similar offline-load paths where `u` is a temp user that isn't connected. Only add the call to sites where a real `UR_OBJECT` is being prepared for an active session. The ~4498 site is the connect path; add it there. Skip the ~3397 site.

A safer alternative: add the call inside `load_user_details` itself, right before `return 1;`. Use this approach — it's idempotent (a temp `u` that never gets used will simply have a non-NULL `catalog` pointer that gets freed when the temp user is destructed; nothing else references it). Search the function in `src/amnuts.c:2612` for its final `return 1;` (it's near the end of the function, just before the closing brace). Insert immediately above:

```c
    locale_resolve_catalog(user);
```

- [ ] **Step 3: Call it from the reboot reattach.**

In `src/reboot.c`, find `retrieve_users` (the function the reboot calls to reconstitute users — search the file for `retrieve_users` or `fopen.*REBOOTING_DIR`). For each user it materialises and calls `load_user_details` on, ensure `locale_resolve_catalog` runs after. If you added the call inside `load_user_details` itself in Step 2, this is automatic — no edit needed. Confirm by searching `src/reboot.c` for `load_user_details(`; the resolve will happen naturally inside that call.

- [ ] **Step 4: Build and smoke-test.**

```bash
make build
./amnutsTalker -p 12345
```

Telnet in, log in, then quit. Look in the system log (`files/logfiles/syslog` or whatever the talker writes to — confirm by tailing during boot) — there should be no `[locale]` warnings. The catalog pointer assignment is silent on success. Stop the talker.

- [ ] **Step 5: Commit.**

```bash
git add src/catalog.c src/amnuts.c
git commit -m "$(cat <<'EOF'
Resolve user->catalog after load_user_details

Hooked inside load_user_details itself so every load path — first
login, reboot reattach, admin lookups — gets the user pointed at the
right catalog. Missing locale dirs reset user->locale and log it.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 12: Implement `locale_set_user` and `locale_list`

**Files:**
- Modify: `src/catalog.c`

Helpers used by `set lang` (Task 13). `locale_set_user(user, name)` validates that `name` is discovered, updates `user->locale`, refreshes `user->catalog`, and persists by calling `save_user_details(user, 1)`. `locale_list(user)` prints the available locales with their `meta.name` / `meta.description` if present.

- [ ] **Step 1: Implement `locale_set_user`.**

Append to `src/catalog.c`:

```c
int
locale_set_user(UR_OBJECT user, const char *name)
{
    if (!user) return 0;

    /* "default" or empty means "clear override". */
    if (!name || !*name || !strcasecmp(name, "default")) {
        user->locale[0] = '\0';
        locale_resolve_catalog(user);
        save_user_details(user, 1);
        return 1;
    }

    /* Must be a discovered locale. */
    struct locale_catalog *cat = catalog_for_locale(&amsys->locales, name);
    if (!cat) {
        return 0;
    }
    strncpy(user->locale, cat->name, LOCALE_NAME_LEN - 1);
    user->locale[LOCALE_NAME_LEN - 1] = '\0';
    user->catalog = cat;
    save_user_details(user, 1);
    return 1;
}
```

- [ ] **Step 2: Implement `locale_list`.**

```c
void
locale_list(UR_OBJECT user)
{
    if (!user) return;

    write_user(user, "\n~OL~FCAvailable languages:~RS\n\n");
    for (int i = 0; i < amsys->locales.count; ++i) {
        struct locale_catalog *cat = &amsys->locales.catalogs[i];
        const struct lang_entry *nm = catalog_lookup(cat, "meta.name");
        const struct lang_entry *ds = catalog_lookup(cat, "meta.description");
        const char *display_name = nm ? nm->fmt : cat->name;
        const char *display_desc = ds ? ds->fmt : "(no description)";
        char marker = ' ';
        if (cat->is_default)             marker = '*';
        if (!strcmp(cat->name, user->locale)) marker = '>';
        vwrite_user(user, " %c ~OL%-16.16s~RS  %s    %s",
                    marker, cat->name, display_name, display_desc);
        if (display_desc[strlen(display_desc) - 1] != '\n') {
            write_user(user, "\n");
        }
    }
    write_user(user, "\n~OL*~RS = server default, ~OL>~RS = your current setting\n\n");
}
```

The `meta.name` / `meta.description` strings are fetched as raw format strings — they happen to have no `%` specifiers in practice so they render literally. If a translator puts a `%s` in `meta.name`, that's their bug; we print the format string as-is rather than trying to be clever.

- [ ] **Step 3: Build.**

Run: `make compile`

Expected: clean.

- [ ] **Step 4: Commit.**

```bash
git add src/catalog.c
git commit -m "$(cat <<'EOF'
Implement locale_set_user and locale_list

locale_set_user validates against the discovered locale list, updates
user->locale + catalog pointer, and persists via save_user_details.
locale_list renders the available-locales screen with meta.name /
meta.description for any locale that has them.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 13: Wire `set lang` into SET_LIST

**Files:**
- Modify: `src/includes/commands.h`
- Modify: `src/commands/set.c`
- Create: `src/commands/set_lang.c`

`set lang` follows the same pattern as `set gender`, `set wrap`, etc. — entry in `SET_LIST`, case in `set_attributes`, handler in its own file.

- [ ] **Step 1: Add the SET_LIST entry.**

In `src/includes/commands.h`, find the `SET_LIST` X-macro (around line 240). Add a new entry alphabetically-ish (the file doesn't strictly alphabetise, but `LANG` near `RECAP`/`REVBUF` is fine) just before the `COUNT` sentinel:

```c
  ML_ENTRY((LANG,      "lang",     "set the language/locale you'd like to use"                             )) \
```

That produces the enum value `SETLANG` and a `setstr[]` entry `{ "lang", "set the language/locale you'd like to use" }`.

- [ ] **Step 2: Dispatch SETLANG in `set_attributes`.**

In `src/commands/set.c`, find the giant `switch (i)` block in `set_attributes` (it starts around line 47). Add a new case before the closing brace of the switch:

```c
    case SETLANG:
        set_user_lang(user);
        return;
```

- [ ] **Step 3: Implement `set_user_lang`.**

Create `src/commands/set_lang.c`:

```c
/****************************************************************************
             Amnuts - Copyright (C) Andrew Collington, 1996-2026

                   talker@amnuts.net - https://amnuts.net/

                                 based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * set lang
 * set lang <name>
 * set lang default
 */
void
set_user_lang(UR_OBJECT user)
{
    if (word_count < 3) {
        locale_list(user);
        if (*user->locale) {
            vwrite_user(user, "Your current language: ~OL%s~RS\n",
                        user->locale);
        } else {
            vwrite_user(user, "Your current language: ~OL%s~RS (server default)\n",
                        amsys->default_locale);
        }
        write_user(user, "Usage: set lang <name>|default\n");
        return;
    }

    if (!strcasecmp(word[2], "default")) {
        if (locale_set_user(user, NULL)) {
            vwrite_user(user, "Language reset to server default (~OL%s~RS).\n",
                        amsys->default_locale);
        } else {
            write_user(user, "Failed to reset language.\n");
        }
        return;
    }

    if (!locale_set_user(user, word[2])) {
        vwrite_user(user,
                    "No such language: ~OL%s~RS. Use ~OLset lang~RS for the list.\n",
                    word[2]);
        return;
    }
    vwrite_user(user, "Language set to ~OL%s~RS.\n", user->locale);
}
```

- [ ] **Step 4: Build.**

Run: `make build`

Expected: clean. The Makefile auto-picks `src/commands/set_lang.c` via its existing wildcard rule.

- [ ] **Step 5: Smoke-test.**

```bash
./amnutsTalker -p 12345
```

Telnet in, log in. Try:

- `set lang` — should print the available-locales table (en_GB marked `*`, no `>`).
- `set lang fallback_test` — should print `Language set to fallback_test.`
- `set` (no argument) — should now include `lang` in the listing.
- `set lang` again — `fallback_test` should be marked `>`.
- `set lang default` — should reset.
- `set lang nonsense` — should print the "No such language" message.

Log out cleanly, inspect your user file — when you logged out after `set lang fallback_test` the `language fallback_test` line should appear; after `set lang default` it should disappear again. (Actually, with the test sequence above the last setting was `default` so it should not appear. To verify it does persist when set, do another quick login: `set lang fallback_test`, then `quit`, then `cat files/userfiles/<you>.D | grep language`. Expected: `language      fallback_test`.)

Stop the talker.

- [ ] **Step 6: Commit.**

```bash
git add src/includes/commands.h src/commands/set.c src/commands/set_lang.c
git commit -m "$(cat <<'EOF'
Wire `set lang` into the set-attributes dispatch

New SETLANG entry in SET_LIST, dispatched to set_user_lang in
src/commands/set_lang.c. set_user_lang shows the available-locales
list when called with no argument, switches the user's locale when
given a name, and resets to default when given 'default'.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 14: Implement `langreload` (WIZ command)

**Files:**
- Modify: `src/includes/commands.h` (add CMD_LIST entry)
- Modify: `src/catalog.c` (add `catalog_reload_all` — atomic swap, generation hold)
- Create: `src/commands/langreload.c`

`langreload` re-runs the catalog load against a fresh `locale_state`, swaps atomically into `amsys`, and walks the user list re-resolving each `user->catalog`. The previous generation is held one cycle to keep in-flight `lang_*` callers safe; the cycle after a second `langreload` it gets freed.

- [ ] **Step 1: Add CMD_LIST entry.**

In `src/includes/commands.h`, find `CMD_LIST` (around line 30). Add a new entry. Place it near the existing wizard admin commands (e.g., near `LISTBANS`, `PROMOTE`, etc., around line 83):

```c
  ML_ENTRY((LANGRELOAD,   "langreload", "",   WIZ,     ADMIN  )) \
```

That gives the enum `LANGRELOAD` and a command-table entry pointing at `CT_LANGRELOAD`. Now we also need the dispatch wiring.

- [ ] **Step 2: Wire LANGRELOAD into the command-dispatch table.**

Commands are dispatched by `command_table[com_id].function` mapped to an enum in `CT_LIST`. Find `CT_LIST` in `src/includes/commands.h` (it's the X-macro near line 200-or-so that maps command-table function entries to handler functions). Add a new entry:

```c
  ML_ENTRY((LANGRELOAD,  langreload      )) \
```

(or whatever the exact form is — match the convention of nearby entries.) Then ensure the dispatch switch in `src/amnuts.c` (search for `CT_LISTBANS` or `CT_PROMOTE` — they'll be in a giant switch over `enum ct_value`) gets a new case:

```c
    case CT_LANGRELOAD:
        langreload(user);
        break;
```

Run `grep -n "CT_LISTBANS\b" src/amnuts.c src/includes/commands.h` first to find the exact location of both the X-macro list and the dispatch switch — the dispatch is wherever `CT_LISTBANS` is the case label.

(Use `semble search "command dispatch switch CT_LISTBANS"` instead of grep per the project's search conventions.)

- [ ] **Step 3: Add `catalog_reload_all` to `src/catalog.c`.**

Append:

```c
/* One generation of grace: we keep the previous catalog table alive until
 * the *next* langreload completes, so any in-flight lang_* function that
 * captured an old user->catalog still reads valid memory. After the second
 * reload the previous-previous becomes freeable. */
static struct locale_state *prev_locales_held = NULL;

static void
locale_state_destroy(struct locale_state *st)
{
    if (!st) return;
    catalog_free_all(st);
    free(st);
}

/*
 * Reload all catalogs. On success swaps amsys->locales atomically and
 * re-resolves every connected user's catalog pointer. On failure leaves
 * the live table untouched and reports the error.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
catalog_reload_all(void)
{
    /* Build a fresh locale_state on the side. */
    struct locale_state *fresh = calloc(1, sizeof *fresh);
    if (!fresh) return -1;

    /* Re-run discovery into `fresh`. We can't call locale_load_all
     * directly because it operates on amsys->locales; instead, do a
     * trimmed-down discovery here. (Refactor opportunity: factor out
     * a `locale_discover_into(state)` helper. For Phase 2 we duplicate
     * the small loop; revisit if a third caller appears.) */
    DIR *dirp = opendir(LANGS_ROOT);
    if (!dirp) {
        free(fresh);
        return -1;
    }
    struct dirent *dp;
    int default_seen = 0;
    while ((dp = readdir(dirp))) {
        size_t len = strlen(dp->d_name);
        if (!len || dp->d_name[0] == '.' || len >= LOCALE_NAME_LEN) continue;
        if (strchr(dp->d_name, '/') || strchr(dp->d_name, '\\')) continue;
        struct stat st;
        char path[1024];
        snprintf(path, sizeof path, "%s/%s", LANGS_ROOT, dp->d_name);
        if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) continue;
        if (!strcmp(dp->d_name, amsys->default_locale)) default_seen = 1;
        if (fresh->count >= MAX_LOCALES) continue;
        strncpy(fresh->names[fresh->count], dp->d_name, LOCALE_NAME_LEN - 1);
        fresh->names[fresh->count][LOCALE_NAME_LEN - 1] = '\0';
        fresh->count++;
    }
    closedir(dirp);

    if (!default_seen) {
        free(fresh);
        return -1;
    }

    if (catalog_load_all(fresh) != 0) {
        catalog_free_all(fresh);
        free(fresh);
        return -1;
    }

    /* Atomic swap. */
    struct locale_state old_state = amsys->locales;
    amsys->locales = *fresh;
    free(fresh);

    /* Release the previous-previous generation, hold the previous. */
    locale_state_destroy(prev_locales_held);
    prev_locales_held = malloc(sizeof *prev_locales_held);
    if (prev_locales_held) {
        *prev_locales_held = old_state;
    } else {
        /* Out of memory holding the grace generation — free in place.
         * Accepts the tiny risk window for the rare OOM case. */
        catalog_free_all(&old_state);
    }

    /* Re-resolve every connected user's catalog pointer. */
    int resets = 0;
    for (UR_OBJECT u = user_first; u; u = u->next) {
        if (u->type == CLONE_TYPE || u->type == REMOTE_TYPE) continue;
        const char *had = *u->locale ? u->locale : "";
        locale_resolve_catalog(u);
        if (*had && !*u->locale) {
            ++resets;
            write_user(u,
                "~OL~FY[ Language reset to server default — your previous setting is gone. ]~RS\n");
        }
    }
    write_syslog(SYSLOG, 1,
                 "[locale] langreload: %d catalog(s) reloaded, %d user(s) reset.\n",
                 amsys->locales.count, resets);
    return 0;
}
```

- [ ] **Step 4: Declare `catalog_reload_all` in `prototypes.h`.**

Add to the catalog.c block in `src/includes/prototypes.h`:

```c
int   catalog_reload_all(void);
```

- [ ] **Step 5: Implement the command handler.**

Create `src/commands/langreload.c`:

```c
/****************************************************************************
             Amnuts - Copyright (C) Andrew Collington, 1996-2026

                   talker@amnuts.net - https://amnuts.net/

                                 based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Reload all language catalogs from disk. Atomic swap; in-flight callers
 * read from the previous generation, held one cycle.
 */
void
langreload(UR_OBJECT user)
{
    if (catalog_reload_all() != 0) {
        write_user(user, "~FRLanguage reload failed.~RS Check the syslog.\n");
        write_syslog(SYSLOG | ERRLOG, 1,
                     "[locale] langreload requested by %s but failed.\n",
                     user->name);
        return;
    }
    vwrite_user(user,
                "~FGLanguage reload complete:~RS %d locale(s) loaded; default = ~OL%s~RS.\n",
                amsys->locales.count, amsys->default_locale);
    write_syslog(SYSLOG, 1, "%s ran langreload.\n", user->name);
}
```

- [ ] **Step 6: Build.**

Run: `make build`

Expected: clean build. If you forgot the CT_LIST entry in Step 2, the link will fail with an undefined `langreload` or with a missing case in the dispatch switch — fix by completing Step 2.

- [ ] **Step 7: Smoke-test.**

```bash
./amnutsTalker -p 12345
```

Log in as a WIZ-or-above user. Try:

- `.langreload` — should print `Language reload complete: 2 locale(s) loaded; default = en_GB.` and write a syslog entry.
- Edit `files/langs/en_GB/strings.yml` while the talker is running — change `meta.name` to `"British English (Hot Reloaded)"`.
- Run `.langreload` again.
- `set lang` — the listing should show the new name immediately.
- Edit the file to introduce a `%n` in some key — run `.langreload` — expect the broken key dropped, others retained, a syslog warning, but the talker stays up.

Stop the talker. Restore `strings.yml` to the original.

- [ ] **Step 8: Commit.**

```bash
git add src/includes/commands.h src/catalog.c src/includes/prototypes.h src/commands/langreload.c src/amnuts.c
git commit -m "$(cat <<'EOF'
Implement langreload (WIZ command) with atomic catalog swap

catalog_reload_all builds a fresh locale_state on the side, validates,
and only swaps amsys->locales on success. The previous generation is
held one cycle for in-flight lang_* readers; the cycle after a second
reload frees the previous-previous.

After the swap every connected user's user->catalog is re-resolved;
users whose locale directory disappeared get bumped back to default
with an in-band notice.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 15: Smoke-test the round-trip end-to-end

**Files:**
- (none committed in this task)

A sanity check that exercises every Phase 2 entry point against the running talker, plus one obvious "does the ping smoke-test key actually render" probe to confirm the API works before we end the phase.

- [ ] **Step 1: Add a temporary smoke-test call site.**

In `src/amnuts.c`, find the user-connected greeting / "look" path inside the login flow (search for `look(user)` near the end of connect — around line 4955 in the section that prints `*** YOU HAVE ... UNREAD MAIL`). Temporarily add, just after `look(user)`:

```c
    lang_user(user, "ping.greeting", user->recap);
```

Build and boot. Log in — you should see `Hello, <YourName>. Catalog is working.` printed once. If a user has set `lang fallback_test` (which has no `strings.yml`), the same line should still render because the lookup falls back to en_GB.

- [ ] **Step 2: Confirm both fallback paths via a temporary partial strings.yml.**

Stop the talker. Create `files/langs/fallback_test/strings.yml` with one key:

```yaml
ping.greeting: "Howdy, %1$s. Cowboy locale.\n"
```

Boot, log in as a user with `set lang fallback_test`. You should now see `Howdy, <YourName>. Cowboy locale.` Switch to `set lang default` — should see the en_GB line again.

- [ ] **Step 3: Confirm `langreload` reflects edits live.**

While still connected (as WIZ), edit `files/langs/fallback_test/strings.yml`:

```yaml
ping.greeting: "Yeehaw, %1$s.\n"
```

Run `.langreload`. Switch yourself to `set lang fallback_test`. Type any command that triggers a `look` (e.g., move rooms with `.go`). You should see the new line without a reboot.

- [ ] **Step 4: Confirm format-string safety.**

Edit `files/langs/fallback_test/strings.yml`:

```yaml
ping.greeting: "Bug: %n\n"
```

Run `.langreload`. The reload should still succeed but with a syslog warning that `ping.greeting` was dropped from `fallback_test`. Switch to `set lang fallback_test` again, do another `look`. You should see the en_GB fallback `Hello, <YourName>. Catalog is working.` line — because the broken key was dropped from the fallback_test catalog, the lookup fell back to default.

Restore `fallback_test/strings.yml`:

```yaml
ping.greeting: "Yeehaw, %1$s.\n"
```

- [ ] **Step 5: Remove the temporary call site.**

Revert the `lang_user(user, "ping.greeting", user->recap);` line from `src/amnuts.c`. Build and confirm the warning-free state.

```bash
make build
./amnutsTalker -p 12345
```

Log in — the smoke-test line should be gone. Stop the talker.

- [ ] **Step 6: No commit in this task.**

Just a verification step. The temporary edits all roll back. Don't commit anything from this task.

---

## Task 16: Update the migration ledger

**Files:**
- Modify: `docs/superpowers/specs/localisation-migration.md`

- [ ] **Step 1: Mark Phase 2 converted.**

Open `docs/superpowers/specs/localisation-migration.md`. Change the Phase 2 row from:

```markdown
| 2 — catalog framework | pending | string catalog + lang_* API + set lang + langreload |
```

to:

```markdown
| 2 — catalog framework | converted (YYYY-MM-DD) | string catalog + lang_* API + set lang (USER) + langreload (WIZ); en_GB/strings.yml ships with meta.* and one smoke-test key; no in-source call sites converted yet |
```

Replace `YYYY-MM-DD` with the actual completion date.

- [ ] **Step 2: Append a verification block to the file.**

Append after the `## Phase 1 verification` block:

```markdown
## Phase 2 verification

The Phase 2 sweep landed across the commits introducing catalog.c,
yaml_util.c, the SETLANG and LANGRELOAD command wiring, the
USERDB_LIST extension for the per-user `language` field, and the
initial en_GB/strings.yml.

End-to-end verification:

1. Build on a Linux/macOS host: `make build`.
2. Boot `./amnutsTalker`; confirm the two new startup lines:
   - `Localisation: discovered N locale(s); default = en_GB.`
   - `Localisation: catalog loaded (N locale(s)).`
3. Telnet in, log in. Exercise:
   - `set lang` — table of locales, en_GB marked `*`.
   - `set lang fallback_test` — confirmation.
   - `quit` — log out, then `grep '^language' files/userfiles/<your>.D`
     — should show `language      fallback_test`.
   - Log in again — confirm the choice survived.
   - As a WIZ: `.langreload` — confirmation message and syslog entry.
4. Format-safety check:
   - Edit a non-default locale's strings.yml to include `%n` in a key.
   - Run `.langreload` — the reload succeeds; the broken key is dropped
     and a syslog warning is written; other keys still load.

No call sites in the wider codebase use `lang_*` yet. Phase 4 begins the
sweep with `wizlist`.
```

- [ ] **Step 3: Commit.**

```bash
git add docs/superpowers/specs/localisation-migration.md
git commit -m "$(cat <<'EOF'
Phase 2 complete: catalog framework

Update migration ledger. The lang_* API, set lang (USER), langreload
(WIZ), and per-user language persistence are all live; nothing in the
wider codebase consumes them yet. Phase 4 starts the actual sweep
with wizlist.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Self-Review Checklist

- [ ] **Spec §3.4 (amsys + UR_OBJECT fields).** `struct locale_state` extended with `catalogs[]` + `default_index`; `UR_OBJECT.catalog` added — Task 1.
- [ ] **Spec §4.1 (YAML format).** Flat map, scalar values, printf positional accepted, block scalars accepted by libyaml — Task 6.
- [ ] **Spec §4.2 (libyaml).** Already vendored Phase 1; consumed by `catalog.c` via the helpers in `yaml_util.c` — Tasks 3 & 6.
- [ ] **Spec §4.3 (C-side API).** `lang`, `lang_user`, `lang_room`, `lang_level`, `lang_format`, `locale_set_user`, `locale_list`, `locale_default` — Tasks 8, 9, 12. `locale_default` was already implemented in Phase 1; unchanged.
- [ ] **Spec §4.4 (per-recipient render).** `va_copy` per recipient — Task 9.
- [ ] **Spec §4.5 (format-string safety).** `%n` rejected; signature compatibility checked against default; per-key drop on failure — Task 5.
- [ ] **Spec §7 (`set lang`).** SETLANG entry, dispatch, handler, listing — Task 13.
- [ ] **Spec §7.1 (`langreload`).** Atomic swap, generation held, per-user re-resolve — Task 14.
- [ ] **Spec §8.1 (boot sequence).** `catalog_load_all` runs after `locale_load_all`'s discovery — Task 6. Hard-fail on default missing — Task 6 step 2. Soft-fail per-key — Task 5/6.
- [ ] **Spec §8.4 (reboot survival).** Catalogs reload from disk in the fresh boot; user's persisted `language` field re-resolves via `locale_resolve_catalog` inside `load_user_details` — Tasks 10 & 11.
- [ ] **Spec §8.5 (memory ownership).** Catalogs owned by `amsys->locales.catalogs`; `user->catalog` non-owning; one generation grace via `prev_locales_held` — Task 14.
- [ ] **Spec §8.6 (logging).** Discovery + drop + reset + reload all syslog — Tasks 6, 11, 14.

**Placeholder scan.**

- Task 11 step 2 notes two `load_user_details` call sites; the resolved approach is "put the call inside `load_user_details` itself so all call sites benefit." That's a concrete instruction, not a TODO.
- Task 14 step 3 has a comment noting that the discovery loop is duplicated from `locale.c`'s `locale_load_all` and could be refactored. The duplication is small and intentional; the plan calls it out for future cleanup rather than leaving it as a placeholder.
- Task 9 step 2 explicitly notes that `lang_level`'s `notify_invis` / `record_flag` semantics are simplified for Phase 2 and must be ported from `vwrite_level` when the first sweep consumer needs them. That's a deliberate scope boundary, not a placeholder — Phase 2 ships zero callers.

**Type consistency.**

- `struct locale_catalog` field names (`name`, `is_default`, `loaded_ok`, `bucket_count`, `entry_count`, `buckets`) used consistently across Tasks 1, 4, 5, 6, 8, 9, 12, 14.
- `struct lang_entry` fields (`key`, `fmt`, `arg_count`, `arg_types[8]`, `next`) consistent in Tasks 1, 4, 5, 6, 8.
- `locale_resolve_catalog` signature consistent between prototype (Task 2) and implementation (Task 11).
- `lang_level` signature: 5 args after the level (`min_level, notify_invis, record_flag, exclude, key, ...`) — prototype in Task 2 and implementation in Task 9 match.
- `catalog_reload_all` declared in Task 14 step 4 and implemented in Task 14 step 3 — match.

**Bootability invariant.** After every commit the talker must still build cleanly:

- Tasks 1-5 add unused code; build stays clean.
- Task 6 wires loading into boot but the talker still boots because the only mandatory file is `en_GB/strings.yml`, shipped in Task 7. Between Task 6 and Task 7 the talker WILL refuse to boot if you `make build` and try to run — that's a one-commit window. To keep strict bootability, the implementer can either (a) commit Tasks 6 and 7 together in a single commit, or (b) preemptively create `files/langs/en_GB/strings.yml` (empty or with just `meta.name`) in Task 6's commit. Recommended: option (a). The plan retains the two commits for clarity of intent; the implementer should squash them at the moment of landing if strict bootability is required.
- Tasks 8-12 add unused functions; build stays clean.
- Task 13 adds the user command; build stays clean and the talker stays bootable.
- Task 14 adds the wiz command; build stays clean. Smoke-test in Task 15 exercises end-to-end.
