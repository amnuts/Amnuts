# Amnuts Localisation & Theming — Design

**Status:** approved by user 2026-05-10, ready for implementation planning.
**Scope:** introduce per-user language/theme support for all server-authored
output (source-code literals + on-disk content), with a stable fallback to
the configured server default. User-typed content is out of scope.

---

## 1. Goals

- Per-user locale selection with a server-wide default.
- Two fallback levels: user's locale → server default. No deeper chain.
- Easy authoring: translators edit text files (YAML + plain files), no
  binary tools required.
- Locale identity = directory name. No registry, no validation list.
- Themes and translations use the same mechanism: `cowboy` and `fr` are both
  just locales. The "feel" of frames/dividers is part of the locale, not a
  parallel system.
- Partial locales are first-class: `de-DE/strings.yml` with three keys is
  a legal locale; everything else falls back.
- The talker keeps working at every step of the migration. No big-bang.

## 2. Non-goals

- Not retrofitting Unicode / UTF-8 cleanliness. That's a separate parked
  track. Catalogs are byte-strings; ASCII-safe locales work today, non-ASCII
  locales depend on the Unicode work landing first.
- Not translating `pictfiles/`, `config`, log files, dump files, or any
  user-typed content (mail, profile, history, macros, reminders).
- Not converting all 1,600+ source-code call sites in one shot.
- Not adding an in-game translation UI. Files on disk are the interface.
- Not adding a curses/terminfo dependency.
- Not solving variable-width terminals — assumes the existing 78-column
  convention.

## 3. Architecture

Two cooperating mechanisms, both keyed off `user->locale`:

```
                 user->locale (e.g. "fr" or "cowboy")
                          │
        ┌─────────────────┴─────────────────┐
        │                                   │
   String catalog                    File path resolution
   (in-source literals)              (existing on-disk content)
        │                                   │
   lang_user(user, "key", …)         locale_path(user, CAT, "name")
        │                                   │
   look up "key" in:                  open first found:
     langs/<user-locale>/strings.yml    langs/<user-locale>/<cat>/<name>
     → fall back to                     → fall back to
     langs/<default>/strings.yml        langs/<default>/<cat>/<name>
        │                                   │
   format with vsnprintf,             read file as-is
   write to user's socket
```

### 3.1 Locale identity

A locale is a directory under `files/langs/`. Its identifier *is* its
directory name — `en_GB`, `fr`, `pt_BR`, `de-DE`, `cowboy`, `oldy_timey`,
anything the filesystem permits as a name. No registry; no format check
beyond "usable directory name". Available locales are *discovered* at boot
by enumerating `files/langs/`.

### 3.2 Directory layout

```
files/
  config           config2
  dumpfiles/       logfiles/        mailspool/        pictfiles/
  reboot/          userfiles/
  langs/
    en_GB/
      strings.yml
      adminfiles/  datafiles/   helpfiles/   miscfiles/   motds/   textfiles/
    de-DE/
      strings.yml          # may contain only a handful of keys
    cowboy/
      strings.yml
      motds/welcome        # only overrides this one file; rest falls back
```

A locale can be partial. Sparse `strings.yml`, sparse content directories,
or both — every missing key and every missing file falls back to the
default locale.

### 3.3 Configuration

- **Server.** New line in `files/config`: `default_language en_GB`. Read at
  boot; validated by `locale_load_all`. Not a hot-tunable; not stored in
  `config2`.
- **Per-user.** New field in the user file: `language` (directory name, or
  empty meaning "use server default"). Settable via `set lang`. Saved by
  the existing user-file save path.

### 3.4 amsys and UR_OBJECT

```c
/* amsys gains: */
char                       default_locale[16];
struct locale_state       *locales;          /* discovered + loaded */

/* UR_OBJECT gains: */
char                       locale[16];        /* "" => use default */
struct locale_catalog     *catalog;           /* resolved cache */
```

`locale_state` owns the catalog table; `UR_OBJECT.catalog` is a non-owning
pointer, refreshed at login, on `set lang`, and on `langreload`.

## 4. String catalog (Mechanism 1)

### 4.1 Format

Flat YAML map. Keys are dot-namespaced (`<area>.<context>.<variant>`).
Values are `printf`-style format strings using POSIX positional specifiers
(`%1$s`, `%2$d`). Block scalars for multi-line strings.

```yaml
shout.you_shouted:        "You shouted: %1$s\n"
shout.heard:              "%1$s shouts %2$s\n"
shout.afk:                "%1$s is AFK and cannot hear shouts.\n"

money.give.usage:         "Usage: money -l/-g/-t [<user> <amount>]\n"
money.give.success:       "You give $%1$d to %2$s~RS.\n"
money.give.received:      "%1$s~RS kindly gives $%2$d.\n"

set.gender.male:          "Gender set to Male\n"
set.gender.female:        "Gender set to Female\n"
set.wrap.on:              "Word wrap now ON\n"
set.wrap.off:             "Word wrap now OFF\n"

common.not_logged_on:     "That user isn't logged on.\n"
common.usage_prefix:      "Usage: "

# Optional metadata for `set lang` listings
meta.name:                "British English"
meta.description:         "The default tongue."
```

Colour escape sequences (`~OL`, `~FC`, `~RS`, `~BR`, etc.) are part of the
string and are the translator's choice — they may emphasise differently or
not at all. The default `en_GB` reproduces the colour codes that appear in
the source today.

### 4.2 YAML reader

`libyaml` (BSD, ~5k LOC) is added under `src/vendors/libyaml/` and linked
into the talker. The same library is available for any other YAML work.

### 4.3 C-side API

Implemented in `src/locale.c`, declared in `src/includes/locale.h`,
prototypes added to `src/includes/prototypes.h`.

```c
/* Look up a key in the user's resolved catalog. Returns catalog-owned
 * pointer; do not free. NULL only if missing in BOTH user's locale and
 * the server default. */
const char *lang(UR_OBJECT user, const char *key);

/* Drop-in replacement for write_user/vwrite_user. */
void lang_user (UR_OBJECT user, const char *key, ...);

/* Broadcast variants — each recipient renders in THEIR OWN locale. */
void lang_room (RM_OBJECT room, UR_OBJECT exclude, const char *key, ...);
void lang_level(int min_level, const char *key, ...);

/* Render into a caller-provided buffer (for composing into align_string,
 * sprintf, table builders, etc.). */
int  lang_format(UR_OBJECT user, char *buf, size_t buflen,
                 const char *key, ...);

/* Lifecycle. */
int  locale_load_all (void);                    /* boot + langreload */
int  locale_set_user (UR_OBJECT user, const char *name);
const char *locale_default(void);
void locale_list     (UR_OBJECT user);          /* `set lang` listing */
```

### 4.4 Per-recipient broadcast loop

`lang_room` and `lang_level` cannot pre-format once and broadcast many,
because each recipient may be on a different locale:

```c
for (u = first_user; u; u = u->next) {
    if (!in_scope(u)) continue;
    fmt = catalog_lookup(u->catalog, key);
    if (!fmt) fmt = catalog_lookup(amsys->default_catalog, key);
    va_copy(ap, original);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    write_socket(u, buf);
}
```

Cost: one `vsnprintf` per recipient per broadcast. Profile after first
conversions land; expected to be noise vs the per-user socket write.

### 4.5 Format-string safety

Format strings come from a file written by translators. Two protections:

1. **Reject `%n`** at catalog load time — known sprintf-attack vector with
   no legitimate translator use.
2. **Validate translation signatures against the default catalog at load
   time.** For each key, parse the default's value into
   `(arg_count, arg_types[])`. Each translation of that key must:
   - Reference only positions the default declares.
   - Use compatible conversion characters (`d`/`i`/`u`/`x` interchangeable;
     `s`, `c` strict).
   - Not contain `%n`.
   Failures: log a one-line warning, drop *that key* from *that locale*,
   fall back to default at lookup. Talker stays up.

We do not get compile-time `__attribute__((format))` — format strings are
data. The two protections above plus boot-time validation cover the
realistic risk surface.

## 5. File path resolution (Mechanism 2)

### 5.1 Re-purposed `defines.h` constants

Six existing category constants change semantics from absolute paths
under `BASE_STORAGE_DIR` to bare category names. The remaining
`BASE_STORAGE_DIR`-prefixed constants (`DUMPFILES`, `LOGFILES`,
`MAILSPOOL`, `PICTFILES`, `USERFILES`, and the `REBOOTING_DIR`-family)
stay unchanged.

```c
/* before */
#define BASE_STORAGE_DIR "files"
#define ADMINFILES  BASE_STORAGE_DIR "/adminfiles"
#define DATAFILES   BASE_STORAGE_DIR "/datafiles"
#define HELPFILES   BASE_STORAGE_DIR "/helpfiles"
#define MISCFILES   BASE_STORAGE_DIR "/miscfiles"
#define MOTDFILES   BASE_STORAGE_DIR "/motds"
#define TEXTFILES   BASE_STORAGE_DIR "/textfiles"

/* after */
#define BASE_STORAGE_DIR "files"
#define LANGS_ROOT      BASE_STORAGE_DIR "/langs"

#define ADMINFILES  "adminfiles"
#define DATAFILES   "datafiles"
#define HELPFILES   "helpfiles"
#define MISCFILES   "miscfiles"
#define MOTDFILES   "motds"
#define TEXTFILES   "textfiles"
```

Re-purposing forces every existing `sprintf(path, "%s/%s", DATAFILES, ...)`
+ `fopen` site to be ported. Better than a silent fall-through.

### 5.2 Resolver helpers

```c
/* User-aware: tries user's locale, falls back to default.
 * Returns 2 (user's), 1 (default fallback), or 0 (neither; out is
 * populated with the default-locale path so callers that fopen()-for-write
 * still target a sane location). */
int locale_path(UR_OBJECT user, char *out, size_t outlen,
                const char *category, const char *name);

/* Default-only: for boot, login banner, and any "describes the world,
 * not a person" lookup. Skips the user-locale step. */
int locale_default_path(char *out, size_t outlen,
                        const char *category, const char *name);
```

### 5.3 Read-only invariant

No server code writes to any of the relocated categories at runtime. There
are no MOTD-edit or room-content-edit commands. Files in
`langs/<locale>/{adminfiles,datafiles,helpfiles,miscfiles,motds,textfiles}/`
are file-edited only. The implementation audit in Phase 1 confirms this; any
exception is relocated to a non-translated category, or writes to the
default-locale path explicitly.

Personal rooms live under `userfiles/` (user state, root-level) — not in
any relocated category, not translated.

## 6. UI builders (boxes, tables, alignment)

### 6.1 Frame primitives in `strings.yml`

Each rule is `lcap + repeating fill pattern + rcap`. Caps and patterns can
each be any width (including zero). Stored in `strings.yml` under `ui.*`,
so themers change them like any other catalog string.

```yaml
# Section rule (no side rails)
ui.rule.lcap:        "+"
ui.rule.rcap:        "+"
ui.rule.fill:        "-"
ui.rule.label_lpad:  6        # visible cols of fill before " <label> "

# Box edges and body
ui.box.top.lcap:     "+"
ui.box.top.rcap:     "+"
ui.box.top.fill:     "-"
ui.box.bot.lcap:     "+"
ui.box.bot.rcap:     "+"
ui.box.bot.fill:     "-"
ui.box.body.lside:   "|"
ui.box.body.rside:   "|"
ui.box.sep.lcap:     "+"
ui.box.sep.rcap:     "+"
ui.box.sep.fill:     "-"
```

Themed examples:

```yaml
# Cowboy
ui.rule.lcap:    "-={*"
ui.rule.rcap:    "*}=-"
ui.rule.fill:    "-"

# Wave
ui.rule.lcap: ""
ui.rule.rcap: ""
ui.rule.fill: "=~"

# Sine
ui.rule.lcap: ""
ui.rule.rcap: ""
ui.rule.fill: "\\_/~"
```

### 6.2 Pattern repetition

```
inner_cols = width - visible_strlen(lcap) - visible_strlen(rcap)
emit lcap (bytes as-is)
remaining = inner_cols
while remaining >= visible_strlen(fill):
    emit fill (bytes as-is); remaining -= visible_strlen(fill)
if remaining > 0:
    emit fill but stop after `remaining` visible cols
    (skip ~XX escape sequences when counting; emit them as-is)
emit rcap
```

So `fill = "=~"` over 9 columns → `=~=~=~=~=`. Truncation is by visible
columns; colour escapes pass through transparently.

### 6.3 Inline-label overlay

Compose in pieces, not memcpy-overwrite — sidesteps byte-vs-visible offset
problems when both fill and label contain colour escapes:

```
output = lcap
       + fill_n(label_lpad)        ; visible cols
       + " "
       + label                     ; bytes, including its own escapes
       + " "
       + fill_n(remaining_cols)
       + rcap
```

Examples:

| Catalog state                              | Output                                     |
|--------------------------------------------|--------------------------------------------|
| `lcap=+ rcap=+ fill=-`                     | `+----- Wiz List -------------------------+` |
| `lcap=-={* rcap=*}=- fill=-`               | `-={*----- Wiz List -----------------*}=-`   |
| `lcap='' rcap='' fill==~` (no label)       | `=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~`   |
| `lcap='' rcap='' fill=\_/~` (no label)     | `\_/~\_/~\_/~\_/~\_/~\_/~\_/~\_/~\_/~\_/~`   |

### 6.4 Body rendering

Each body row: `lside + content (visible-padded to inner_cols) + rside`,
where `inner_cols = width - visible_strlen(lside) - visible_strlen(rside)`.
Multi-character sides are fine. Padding uses `visible_strlen` so colour
escapes don't drift the right border. For wrapped table cells, wrap point
is on visible columns within the column's allocated width.

### 6.5 API

```c
/* Section rule: horizontal line, optional inline label, no side rails. */
void rule(UR_OBJECT user, int width, const char *label_fmt, ...);

/* Framed box. */
typedef struct box_struct *BOX;
BOX  box_open      (UR_OBJECT user, int width, const char *title_fmt, ...);
void box_line      (BOX b, const char *fmt, ...);   /* body row, left-aligned */
void box_blank     (BOX b);
void box_centered  (BOX b, const char *fmt, ...);
void box_separator (BOX b);                          /* internal divider */
void box_close     (BOX b);

/* Table — built on top of box; cells word-wrap to column width. */
typedef struct table_struct *TABLE;
TABLE table_open      (UR_OBJECT user, int total_width);
void  table_columns   (TABLE t, int n, ...);        /* widths */
void  table_header    (TABLE t, ...);
void  table_separator (TABLE t);
void  table_row       (TABLE t, ...);
void  table_close     (TABLE t);

/* Lower-level. */
int   visible_strlen  (const char *s);              /* counts bytes excluding ~XX */
int   align_into      (char *out, size_t outlen,
                       int align, int width, const char *fmt, ...);
```

`visible_strlen` is the keystone. Every padding/wrap/overlay calculation
uses it, never `strlen`.

### 6.6 Files

- New: `src/uibuilders.c`, `src/includes/uibuilders.h`.
- Prototypes added to `src/includes/prototypes.h`.

## 7. User commands

`set lang` is the obvious home — sits next to `set gender`, `set wrap`,
etc. One new `SETLANG` entry in the `SET_LIST` X-macro. Level: `USER`.

```
set lang                  show current locale + list of available locales
set lang <name>           switch to <name>; must be a discovered locale
set lang default          clear override, fall back to server default
```

Listing uses `meta.name` and `meta.description` from each locale's
`strings.yml` if present; falls back to the directory name. Switching
swaps the cached catalog pointer immediately and persists to the user
file.

### 7.1 langreload (WIZ)

```
langreload                reload all locale catalogs from disk
```

1. `locale_load_all()` into a new `locale_state`, with full validation.
2. If well-formed: atomic pointer swap on `amsys`.
3. Walk user list; re-resolve each `user->catalog`. If a user's locale
   directory is gone, clear it, point at default, send a one-line in-band
   notification.
4. Old state kept one generation behind; next reload (or shutdown) frees
   the previous-previous.

### 7.2 Login banner vs MOTD

Pre-auth banner: server default locale (no user yet). Post-login MOTD: the
user's locale via `locale_path` — French user gets the French MOTD without
any extra wiring.

## 8. Lifecycle and error handling

### 8.1 Boot sequence

```
main()
 ├── load config              (read default_language)
 ├── locale_discover()        enumerate files/langs/* → available_locales
 ├── locale_load_all()
 │    ├── parse default first; abort startup on YAML / missing required
 │    │   ui.* keys / missing default_language directory
 │    ├── parse each non-default; on parse failure, drop + log, keep going
 │    └── format-signature validation pass
 ├── load rooms / maps / etc. via locale_default_path()
 ├── reboot reattach (if recovering): re-resolve each user's catalog
 │   pointer from their persisted `locale` string
 └── enter select() loop
```

**Hard-fail at boot** (talker refuses to start):
- `files/langs/` missing.
- `default_language` directory missing.
- Default's `strings.yml` doesn't parse.
- Default's catalog missing required `ui.*` keys.

**Soft-fail at boot** (log and continue):
- Non-default `strings.yml` parse error → drop that locale.
- Non-default missing a key → key falls back at lookup.
- Non-default signature mismatch on a key → drop that key from that
  locale.

### 8.2 Catalog data structure

```c
struct locale_catalog {
    char  name[16];
    bool  is_default;
    int   bucket_count;
    struct lang_entry **buckets;
};

struct lang_entry {
    const char *key;
    const char *fmt;
    uint8_t     arg_count;
    char        arg_types[8];   /* per-position 'd','s','x','c'; '\0'=unused */
    struct lang_entry *next;
};
```

Loaded once, immutable thereafter. `lang_*` functions read fmt and
`vsnprintf` into a local buffer immediately — never retain the catalog
pointer past return.

### 8.3 Runtime missing-key

```c
const char *fmt = catalog_lookup(user->catalog, key);
if (!fmt) fmt = catalog_lookup(amsys->default_catalog, key);
if (!fmt) {
    log_warn_rate_limited("missing lang key: %s", key);
    vwrite_user(user, "[??? %s]\n", key);   /* visible bug marker */
    return;
}
```

`[??? key]` in output is intentionally ugly. Rate-limited logging avoids
flooding when a hot path is missing a key.

### 8.4 Reboot survival

The catalog is **not** serialised across the seamless re-exec — it's
reconstructed from disk by the new binary. Each user's persisted `locale`
string is read from the dump and re-resolved against the freshly-loaded
catalog table. Locale gone since serialise → clear, point at default,
log it. New binary's signature validation runs on the fresh catalogs, so
incompatible translations are caught before reattach completes.

### 8.5 Memory ownership

- Catalogs owned by `amsys->locales`.
- Format strings pointed-into are immutable for the catalog generation.
- `lang_*` never retains pointers across calls.
- One generation of grace on `langreload` for in-flight safety.
- `UR_OBJECT.catalog` is non-owning; cleared and re-resolved on reload +
  reboot.

### 8.6 Logging

All to `LOGFILES` via existing logging functions:

```
[locale] discovered 4 locales: en_GB (default), fr, de-DE, cowboy
[locale] de-DE/strings.yml: parse error at line 47 — locale dropped
[locale] cowboy: key 'shout.heard' has %3$s but default declares 2 args — key dropped
[locale] missing key 'set.gender.unknown' (1 of N rate-limited reports)
[locale] user Andy's locale 'fr' no longer available; reset to default
[locale] langreload: 4 catalogs reloaded, 1 user reset
```

Nothing surfaces to non-wiz users unless their own locale was the one
reset.

## 9. Migration

### 9.1 Phasing

Each phase ships independently; the talker keeps working at every step.

- **Phase 1 — Mechanism 2 + directory move.** Vendor libyaml, relocate the
  six categories (`adminfiles`, `datafiles`, `helpfiles`, `miscfiles`,
  `motds`, `textfiles`) under `files/langs/en_GB/`, re-purpose the matching
  `*_FILES` constants, implement `locale_path` / `locale_default_path`,
  sweep every path-building call site. After this phase the talker is byte-identical
  to before from a user's perspective; only file locations and
  path-building helpers have changed.
- **Phase 2 — Catalog framework.** Implement catalog data structure, YAML
  loader, signature validation, the `lang` / `lang_user` / `lang_room` /
  `lang_level` / `lang_format` API. Add per-user `language` field.
  Add `set lang` (USER) and `langreload` (WIZ). `en_GB/strings.yml` ships
  empty (or with `meta.*` only). No call site uses `lang_*` yet.
- **Phase 3 — UI builders.** Implement `src/uibuilders.c`. Add `ui.*` to
  `en_GB/strings.yml`. Convert one or two pilot commands (wizlist is
  natural) to verify end-to-end; demonstrates a cowboy theme working.
- **Phase 4 — Frame-heavy commands.** Convert `wizlist`, `show_igusers`,
  `grepusers`, `listbans`, `system`, parts of `help`. Each conversion is
  its own commit.
- **Phase 5 — Bulk inline-string conversion.** Per-command-file sweep of
  the remaining ~1,500 `write_user`/`vwrite_user` sites. Each command is
  independently mergeable; sites not yet converted continue to use
  `write_user` and aren't translatable until converted — graceful partial
  state.
- **Phase 6 — First non-default locale.** Translator's job, not a code
  phase. Drop `fr/strings.yml`, run `langreload`.

### 9.2 Tooling

In `tools/locale/` (Python, dev-only, not bundled with the talker):

- **`extract.py`** — given a `.c` file, finds output literals, suggests
  keys, emits a YAML stub.
- **`refs.py`** — walks source for `lang_*("key", ...)` and walks loaded
  catalogs; reports missing keys and orphans.
- **`check.py`** — runs the format-signature validation as a standalone
  CLI for translators to validate before committing.

### 9.3 Migration tracking

`docs/superpowers/specs/localisation-migration.md` ledger lists each
command file with status (`converted` / `pending` / `not-applicable`).
Updated as part of each conversion commit.

### 9.4 Testing

No test framework exists in the repo; not adding one as part of this work.

- Per-phase manual test plan in this doc's appendix (see §11).
- Tiny standalone `amnutsLangCheck` binary that loads a catalog directory
  and runs the validation passes — useful for quick "did I break the
  YAML" checks and for a future CI.

## 10. Risk register

- **Per-recipient broadcast cost.** `lang_room` does one `vsnprintf` per
  recipient. Likely noise vs the per-user socket write. *Mitigation:
  measure when Phase 4 lands; revisit if it shows up.*
- **Bad format-string crashing the talker.** *Mitigation: load-time
  signature validation rejects the key, falls back to default.*
- **Malicious `%n` from a translator with file access.** Locale dirs are
  admin-curated; defence in depth is cheap. *Mitigation: load-time
  rejection.*
- **Long-running Phase 5 partial state.** Some commands translatable,
  others not, indefinitely. *Mitigation: that's a feature — partial
  conversion is intentionally OK.*
- **Key-naming inconsistency across many converters.** Different
  contributors invent different conventions. *Mitigation: convention
  documented in `CLAUDE.md`; `refs.py` orphan report flags drift.*
- **Unicode dependency for non-Latin locales.** `fr` / `pt_BR` / etc. need
  the parked UTF-8 telnet work landed for non-ASCII bytes to render
  correctly. *Mitigation: framework ships UTF-8-ready (no per-byte width
  assumptions in `visible_strlen`); ASCII-safe locales (`en_GB`, `cowboy`,
  themes) are unblocked.*

## 11. Appendix — Manual test plan per phase

**Phase 1 (file paths):**
- Connect; play `help` on a few topics; verify same content as before.
- View MOTD on login; verify content unchanged.
- `more <textfile>` on a known textfile; verify content.
- Wizard: confirm room descriptions load (rooms/.R files were relocated).
- Drop a stub `fr/` dir with one helpfile; `set lang fr`; verify that one
  helpfile shows in French and others fall back to English.

**Phase 2 (catalog framework):**
- `set lang` shows discovered locales.
- `set lang fr` (where `fr/strings.yml` exists with one key); confirm
  switch persists across logout/login.
- `set lang nonsense` rejects with a sensible message.
- `langreload` (as WIZ) succeeds; modify a `strings.yml` key, reload, see
  change without reboot.
- Catalog with a `%n` specifier rejected at load (check log).

**Phase 3 (UI builders):**
- `wizlist` renders correctly with default `ui.*`.
- Modify `ui.rule.lcap` to `*===*` in a test locale; `set lang test`;
  re-run `wizlist`; verify the cowboy aesthetic.
- `visible_strlen` test cases via the standalone check binary.

**Phases 4–5 (per-command conversion):**
- For each converted command: compare output before/after on the default
  locale; should be byte-identical.
- For each converted command: switch to a partially-translated locale;
  verify translated keys render and missing keys fall back cleanly.

**Phase 6 (a real translation):**
- Drop a translator-authored `fr/strings.yml`; run `check.py`; switch a
  user to `fr`; play through major commands; report cosmetic issues to
  the translator.
