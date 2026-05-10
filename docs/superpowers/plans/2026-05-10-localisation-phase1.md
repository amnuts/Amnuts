# Localisation Phase 1 — File Path Resolution & Directory Move

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move all server-authored prose under `files/langs/en_GB/`, add the file-path resolver (`locale_path` / `locale_default_path`) with user-locale + default-locale fallback, vendor libyaml for use in later phases. Talker behaves identically to today; only file locations and path-building helpers have changed.

**Architecture:** Six existing content categories (`adminfiles`, `datafiles`, `helpfiles`, `miscfiles`, `motds`, `textfiles`) relocate from `files/<cat>/` to `files/langs/en_GB/<cat>/`. The matching `defines.h` constants change semantics from absolute paths to bare category names. A new `src/locale.c` module exposes `locale_path(user, ...)` and `locale_default_path(...)` that build `<LANGS_ROOT>/<locale>/<category>/<name>`, with stat-based fallback from user's locale to the server default. Every existing `sprintf(path, "%s/%s", DATAFILES, name); fopen(path, ...)` site is converted to use the new helpers.

**Tech Stack:** C (clang, `-std=gnu23 -g -Wall -Wextra -MMD -Wpedantic`), GNU Make. New vendored library: libyaml (BSD). No test framework — verification is `make compile` (must build without warnings) plus manual play-through. Spec: `docs/superpowers/specs/2026-05-10-localisation-design.md`.

---

## File Structure

**New files:**
- `src/vendors/libyaml/` — vendored libyaml source tree (yaml.h + .c files + license)
- `src/locale.c` — locale discovery + path resolver
- `src/includes/locale.h` — `locale_*` declarations + `struct locale_state` opaque forward decl

**Modified files:**
- `Makefile` — vendor block for libyaml; sources auto-picked by existing `wildcard`
- `src/includes/defines.h` — re-purpose six constants from absolute to bare; add `LANGS_ROOT`
- `src/includes/globals.h` — add `default_locale[16]` and `available_locales` to `system_struct`; add forward decl of `struct locale_state`
- `src/includes/prototypes.h` — declare `locale_load_all`, `locale_discover`, `locale_path`, `locale_default_path`, `locale_set_user`, `locale_default`
- `src/amnuts.c` — add `INITOPT_DEFAULT_LANGUAGE` to the config parser's `INIT_LIST` X-macro; call `locale_load_all` early in boot
- 17 caller files identified at planning time (see Task 11): `admin.c`, `messages.c`, `games.c`, `amnuts.c`, `commands/delete_suggestions.c`, `commands/display.c`, `commands/help.c`, `commands/listbans.c`, `commands/read_board.c`, `commands/recount.c`, `commands/reload_room.c`, `commands/search_boards.c`, `commands/sfrom.c`, `commands/suggestions.c`, `commands/wipe_board.c`, `commands/write_board.c` — sweep every site that builds a path under one of the six relocated category constants

**Directory moves (Task 10):**
- `files/datafiles/`   → `files/langs/en_GB/datafiles/`
- `files/helpfiles/`   → `files/langs/en_GB/helpfiles/`
- `files/miscfiles/`   → `files/langs/en_GB/miscfiles/`
- `files/motds/`       → `files/langs/en_GB/motds/`
- `files/textfiles/`   → `files/langs/en_GB/textfiles/`
- `files/adminfiles/`  → `files/langs/en_GB/adminfiles/` (create if absent)

**Unchanged at `files/` root:** `config`, `config2`, `dumpfiles/`, `logfiles/`, `mailspool/`, `pictfiles/`, `reboot/`, `userfiles/`.

---

## Task 1: Vendor libyaml

**Files:**
- Create: `src/vendors/libyaml/` (full source drop)
- Modify: `Makefile`

The catalog mechanism in Phase 2 needs a YAML parser. Vendoring libyaml in Phase 1 means Phase 2 has fewer moving parts. No functional code in Phase 1 calls libyaml — this is build-system prep only.

- [ ] **Step 1: Fetch libyaml release.**

```bash
curl -L https://github.com/yaml/libyaml/releases/download/0.2.5/yaml-0.2.5.tar.gz -o /tmp/yaml-0.2.5.tar.gz
tar -xzf /tmp/yaml-0.2.5.tar.gz -C /tmp/
```

Inspect with `ls /tmp/yaml-0.2.5/src` and `ls /tmp/yaml-0.2.5/include`.

- [ ] **Step 2: Copy sources into the vendor tree.**

```bash
mkdir -p src/vendors/libyaml
cp /tmp/yaml-0.2.5/src/*.c src/vendors/libyaml/
cp /tmp/yaml-0.2.5/include/yaml.h src/vendors/libyaml/
cp /tmp/yaml-0.2.5/LICENSE src/vendors/libyaml/
cp /tmp/yaml-0.2.5/src/yaml_private.h src/vendors/libyaml/ 2>/dev/null || true
```

Then run `ls src/vendors/libyaml/` and confirm `api.c`, `dumper.c`, `emitter.c`, `loader.c`, `parser.c`, `reader.c`, `scanner.c`, `writer.c`, `yaml.h`, `LICENSE`. The `yaml_private.h` and `config.h` headers ship inside `src/` in the upstream release; copy any private header that the `.c` files include relative to themselves.

- [ ] **Step 3: Provide a minimal `config.h` for libyaml.**

libyaml expects an autoconf-generated `config.h`. We supply a fixed one matching the platforms we build for. Create `src/vendors/libyaml/config.h`:

```c
#ifndef LIBYAML_CONFIG_H
#define LIBYAML_CONFIG_H

#define YAML_VERSION_MAJOR  0
#define YAML_VERSION_MINOR  2
#define YAML_VERSION_PATCH  5
#define YAML_VERSION_STRING "0.2.5"

#define HAVE_CONFIG_H 1

#endif
```

- [ ] **Step 4: Add the libyaml block to the Makefile.**

After the `VENDOR_LIBTELNET_*` block in `Makefile`, insert:

```makefile
# libyaml: https://github.com/yaml/libyaml
VENDOR_LIBYAML_SRC_DIR = $(TALKER_SRC_DIR)/vendors/libyaml
VENDOR_LIBYAML_OBJ_DIR = $(TALKER_OBJ_DIR)
VENDOR_LIBYAML_SRC     = $(wildcard $(VENDOR_LIBYAML_SRC_DIR)/*.c)
VENDOR_LIBYAML_OBJS    = $(addprefix $(VENDOR_LIBYAML_OBJ_DIR)/,$(notdir $(VENDOR_LIBYAML_SRC:.c=.o)))
```

In the `clean` target, add a line:

```makefile
	rm -f $(VENDOR_LIBYAML_OBJS) $(VENDOR_LIBYAML_OBJS:.o=.d)
```

In the `distclean` target, add:

```makefile
	rm -f $(VENDOR_LIBYAML_SRC_DIR)/*.[ch]~ $(VENDOR_LIBYAML_SRC_DIR)/*.[ch].bak
```

Update the `compile` target:

```makefile
compile: $(TALKER_OBJS) $(IDENTD_OBJS) $(VENDOR_SDS_OBJS) $(VENDOR_LIBTELNET_OBJS) $(VENDOR_LIBYAML_OBJS)
```

Update the talker link rule to include libyaml objects:

```makefile
$(TALKER_BIN): $(TALKER_OBJS) $(VENDOR_SDS_OBJS) $(VENDOR_LIBTELNET_OBJS) $(VENDOR_LIBYAML_OBJS)
	@echo "Linking $@ ..."
	$(CC) $(LD_FLAGS) $^ $(TALKER_LIBS) -o $@
```

Update `vpath`:

```makefile
vpath %.c $(TALKER_SRC_DIR) $(TALKER_SRC_DIR)/commands $(IDENTD_SRC_DIR) $(VENDOR_SDS_SRC_DIR) $(VENDOR_LIBTELNET_SRC_DIR) $(VENDOR_LIBYAML_SRC_DIR)
```

Add a per-object compile rule (after the libtelnet one):

```makefile
$(VENDOR_LIBYAML_OBJS): $(VENDOR_LIBYAML_OBJ_DIR)/%.o: %.c
	@echo "Compiling libyaml library $< ... ($@)"
	@test -d $(VENDOR_LIBYAML_OBJ_DIR) || mkdir $(VENDOR_LIBYAML_OBJ_DIR)
	$(CC) -std=gnu99 -g -w -I$(VENDOR_LIBYAML_SRC_DIR) -DHAVE_CONFIG_H -c -o $@ $<
```

Note: libyaml is built with `-w` (suppress warnings) and `-std=gnu99` because it pre-dates C23 and the Amnuts compiler flags would reject several legitimate libyaml constructs. This isolation flag is just for the vendored library.

Update the `-include` dependency tracking line:

```makefile
-include $(TALKER_OBJS:.o=.d) $(IDENTD_OBJS:.o=.d) $(VENDOR_SDS_OBJS:.o=.d) $(VENDOR_LIBTELNET_OBJS:.o=.d) $(VENDOR_LIBYAML_OBJS:.o=.d)
```

- [ ] **Step 5: Build and verify.**

```bash
make clean
make build
```

Expected: builds cleanly. libyaml objects compile silently (because of `-w`); talker + identd compile with their usual warnings only. `amnutsTalker` and `amnutsIdent` binaries appear in the project root.

If libyaml fails to compile because of a missing private header: inspect the include paths in any failing `.c` file, copy the referenced private header from the upstream tarball into `src/vendors/libyaml/`, re-run.

- [ ] **Step 6: Commit.**

```bash
git add src/vendors/libyaml/ Makefile
git commit -m "$(printf 'Vendor libyaml 0.2.5\n\nNo functional code uses libyaml yet; this is build-system prep for the\nstring-catalog mechanism in Phase 2 of the localisation plan.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

---

## Task 2: Add `LANGS_ROOT` to defines.h and declare the locale API

**Files:**
- Modify: `src/includes/defines.h`
- Create: `src/includes/locale.h`
- Modify: `src/includes/prototypes.h`

We declare the API surface before implementing anything, so the declarations can be referenced by later code without forward-reference clutter. The six category constants do **not** change in this task — they still point at absolute paths. They flip to bare names in Tasks 11–16, one per task.

- [ ] **Step 1: Add `LANGS_ROOT` to `defines.h`.**

Open `src/includes/defines.h`. After the existing `BASE_STORAGE_DIR` definitions block (around line 50–60), insert:

```c
/* Localisation root — see docs/superpowers/specs/2026-05-10-localisation-design.md */
#define LANGS_ROOT          BASE_STORAGE_DIR "/langs"
#define DEFAULT_LOCALE_NAME "en_GB"
#define LOCALE_NAME_LEN     16
```

- [ ] **Step 2: Create `src/includes/locale.h`.**

```c
/****************************************************************************
   Amnuts localisation — file-path resolver and locale discovery.
   See docs/superpowers/specs/2026-05-10-localisation-design.md
 ***************************************************************************/

#ifndef AMNUTS_LOCALE_H
#define AMNUTS_LOCALE_H

#include <stddef.h>

/* Opaque to callers; defined in locale.c */
struct locale_catalog;
struct locale_state;

/* Forward decl — UR_OBJECT is a pointer typedef declared in globals.h */
struct user_struct;

/* Discover locales under files/langs/ and validate the default. Called
 * once at boot, after the config file has been parsed (so default_locale
 * is known). Returns 0 on success; nonzero values are reserved for future
 * use (in Phase 1, any failure is fatal and reported via boot_exit). */
int locale_load_all(void);

/* Look up a category file in user's locale, falling back to the server
 * default. Writes the resolved path to `out`. Returns:
 *   2  found in user's locale
 *   1  found in default locale (fallback used)
 *   0  not found in either; `out` populated with the default-locale path
 *
 * `category` is a bare category name (e.g. "helpfiles"). `name` is the
 * filename within that category. The composed path is
 * "<LANGS_ROOT>/<locale>/<category>/<name>". */
int locale_path(struct user_struct *user, char *out, size_t outlen,
                const char *category, const char *name);

/* Same, but always resolves against the server default locale. Use for
 * boot-time loaders, login banner display, and any "describes the world,
 * not a person" lookup. */
int locale_default_path(char *out, size_t outlen,
                        const char *category, const char *name);

/* Return the configured default-locale name (NUL-terminated, owned). */
const char *locale_default(void);

#endif /* AMNUTS_LOCALE_H */
```

- [ ] **Step 3: Add prototypes to `prototypes.h`.**

The existing `prototypes.h` is organised by source file. Find the section block-comment marker that suggests where new-file prototypes go (typically alphabetical by source file). Add a new block:

```c
/* locale.c */
int  locale_load_all(void);
int  locale_path(UR_OBJECT user, char *out, size_t outlen,
                 const char *category, const char *name);
int  locale_default_path(char *out, size_t outlen,
                         const char *category, const char *name);
const char *locale_default(void);
```

The `UR_OBJECT` typedef is already declared globally in `globals.h`; using it in `prototypes.h` is consistent with how other prototypes are written.

- [ ] **Step 4: Verify the build still compiles.**

```bash
make compile
```

Expected: builds cleanly. No new code references the declarations yet, so adding them is a no-op for the link.

- [ ] **Step 5: Commit.**

```bash
git add src/includes/defines.h src/includes/locale.h src/includes/prototypes.h
git commit -m "$(printf 'Declare locale API and LANGS_ROOT\n\nNo implementation yet; declarations only. Lets later tasks reference the\nlocale_*() functions without forward-decl clutter.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

---

## Task 3: Add `default_locale` to `system_struct` and parse `default_language`

**Files:**
- Modify: `src/includes/globals.h`
- Modify: `src/amnuts.c` (the `INIT_LIST` X-macro and the parser switch)

The config file gains one new `INIT`-section key: `default_language <name>`. If absent, defaults to `"en_GB"`.

- [ ] **Step 1: Add the storage field to `system_struct`.**

In `src/includes/globals.h`, find `struct system_struct { ... }`. Add (in the same area as other string fields):

```c
char default_locale[LOCALE_NAME_LEN];   /* directory name under LANGS_ROOT */
```

If `LOCALE_NAME_LEN` isn't available because `defines.h` isn't included in this header's chain, include `defines.h` at the top of `globals.h` (it usually already is — verify).

- [ ] **Step 2: Locate the `INIT_LIST` X-macro in `src/amnuts.c`.**

Search for `INIT_LIST` or for `INITOPT_FLOOD_PROTECT` (a known entry that's grep-friendly). The `INIT_LIST` is a list of `ML_ENTRY((TOKEN, "string_key", ...))` lines that drives both the enum and the parser switch.

- [ ] **Step 3: Add a new entry to `INIT_LIST`.**

Append (or insert in alphabetical order — match the file's local convention) one new entry:

```c
ML_ENTRY((DEFAULT_LANGUAGE, "default_language"))
```

Confirm the entry produces:
- An enum value `INITOPT_DEFAULT_LANGUAGE`
- A string-table entry `"default_language"` in the names array

- [ ] **Step 4: Add the parser case in the `INIT`-section switch.**

In the function that parses `INIT`-section key/value pairs (typically named `parse_init_option` or similar — look near the existing `INITOPT_FLOOD_PROTECT` case shown in the codebase), add:

```c
case INITOPT_DEFAULT_LANGUAGE:
    if (strlen(wrd[1]) >= LOCALE_NAME_LEN) {
        fprintf(stderr,
                "Amnuts: default_language value too long on line %d (max %d chars).\n",
                config_line, LOCALE_NAME_LEN - 1);
        boot_exit(1);
    }
    strncpy(amsys->default_locale, wrd[1], LOCALE_NAME_LEN - 1);
    amsys->default_locale[LOCALE_NAME_LEN - 1] = '\0';
    break;
```

- [ ] **Step 5: Set the default-of-the-default early in startup.**

Find where `amsys` is initialised at startup (search for `amsys = calloc` or for the function that zeroes/seeds it — typically called from `main()`). Add, immediately after the calloc/zero:

```c
strcpy(amsys->default_locale, DEFAULT_LOCALE_NAME);   /* may be overridden by config */
```

That ensures `amsys->default_locale` is `"en_GB"` if the config file never sets it.

- [ ] **Step 6: Build and run.**

```bash
make build
./amnutsTalker
```

Expected: starts cleanly. The new `default_language` key is silently absent from the existing `files/datafiles/config`, so the default `en_GB` value applies. (Add a `default_language en_GB` line to `files/datafiles/config` later as part of Task 10's directory move — not now, because the file is still at the old path.)

Stop the talker with Ctrl-C / `quit` once it confirms it starts.

- [ ] **Step 7: Commit.**

```bash
git add src/includes/globals.h src/amnuts.c
git commit -m "$(printf 'Parse default_language config option\n\nNew INIT-section key; defaults to en_GB if absent. Stored in\namsys->default_locale, ready for the locale resolver to consume.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

---

## Task 4: Implement `locale_default_path`

**Files:**
- Create: `src/locale.c`

The simplest of the three resolver functions. Builds `<LANGS_ROOT>/<default_locale>/<category>/<name>` and stat()s it. Returns 1 if exists, 0 otherwise.

- [ ] **Step 1: Create `src/locale.c` with the skeleton + first function.**

```c
/****************************************************************************
   Amnuts localisation — file-path resolver and locale discovery.
   See docs/superpowers/specs/2026-05-10-localisation-design.md
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "defines.h"
#include "globals.h"
#include "locale.h"

const char *
locale_default(void)
{
    return amsys->default_locale;
}

static int
file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

int
locale_default_path(char *out, size_t outlen,
                    const char *category, const char *name)
{
    int n;
    n = snprintf(out, outlen, "%s/%s/%s/%s",
                 LANGS_ROOT, amsys->default_locale, category, name);
    if (n < 0 || (size_t) n >= outlen) {
        /* truncation — caller still gets a NUL-terminated string */
        return 0;
    }
    return file_exists(out) ? 1 : 0;
}
```

- [ ] **Step 2: Build.**

```bash
make compile
```

Expected: `locale.c` compiles. The auto-discovery in the Makefile's `wildcard` picks it up because the `src/*.c` glob already matches.

- [ ] **Step 3: Inline sanity-check by linking the binary.**

```bash
make build
```

Expected: links cleanly. No call sites use the new functions yet.

- [ ] **Step 4: Commit.**

```bash
git add src/locale.c
git commit -m "$(printf 'Implement locale_default_path and locale_default\n\nBuilds <LANGS_ROOT>/<default_locale>/<category>/<name> and stats it.\nReturns 1 if present, 0 otherwise. No callers yet.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

---

## Task 5: Implement `locale_load_all` and locale discovery

**Files:**
- Modify: `src/locale.c`
- Modify: `src/includes/globals.h`
- Modify: `src/amnuts.c`

`locale_load_all` is the boot-time entry point. In Phase 1 it does only the directory discovery + hard-fail validation that the default locale's directory exists. (Catalog loading is Phase 2.)

- [ ] **Step 1: Add `struct locale_state` to globals.h.**

In `globals.h`, near `struct system_struct`, add:

```c
struct locale_state {
    char  names[64][LOCALE_NAME_LEN];   /* discovered locale names */
    int   count;
};
```

Add a field on `amsys`:

```c
struct locale_state locales;
```

(Inline rather than pointer — keeps the lifecycle simple in Phase 1; the `struct locale_catalog` array attached to it arrives in Phase 2.)

- [ ] **Step 2: Implement discovery + validation in `src/locale.c`.**

Append to `src/locale.c`:

```c
static int
is_valid_locale_dirname(const char *name)
{
    /* must be non-empty, no path separators, no leading dot */
    if (!*name || *name == '.') return 0;
    for (const char *p = name; *p; ++p) {
        if (*p == '/' || *p == '\\') return 0;
    }
    return 1;
}

int
locale_load_all(void)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat st;
    char path[1024];
    int default_seen = 0;

    amsys->locales.count = 0;

    dirp = opendir(LANGS_ROOT);
    if (!dirp) {
        fprintf(stderr, "Amnuts: cannot open %s — install is missing the localisation tree.\n",
                LANGS_ROOT);
        boot_exit(1);
    }
    for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
        if (!is_valid_locale_dirname(dp->d_name)) continue;
        snprintf(path, sizeof path, "%s/%s", LANGS_ROOT, dp->d_name);
        if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) continue;
        if (amsys->locales.count >= 64) {
            fprintf(stderr, "Amnuts: more than 64 locales discovered — ignoring %s.\n",
                    dp->d_name);
            continue;
        }
        strncpy(amsys->locales.names[amsys->locales.count],
                dp->d_name, LOCALE_NAME_LEN - 1);
        amsys->locales.names[amsys->locales.count][LOCALE_NAME_LEN - 1] = '\0';
        if (strcmp(dp->d_name, amsys->default_locale) == 0) {
            default_seen = 1;
        }
        amsys->locales.count++;
    }
    closedir(dirp);

    if (!default_seen) {
        fprintf(stderr,
                "Amnuts: default_language '%s' has no directory under %s.\n",
                amsys->default_locale, LANGS_ROOT);
        boot_exit(1);
    }
    printf("Localisation: discovered %d locale(s); default = %s.\n",
           amsys->locales.count, amsys->default_locale);
    return 0;
}
```

- [ ] **Step 3: Verify build.**

```bash
make compile
```

Expected: builds cleanly. No callers yet — wiring into boot is deferred to Task 10 (after the locale tree exists on disk), so the talker continues to boot normally against the unchanged file tree at this point.

- [ ] **Step 4: Commit.**

```bash
git add src/locale.c src/includes/globals.h
git commit -m "$(printf 'Implement locale_load_all: discover locales, validate default\n\nEnumerates files/langs/* directories into amsys->locales. Not yet\ncalled from boot — that wires in once the directory tree exists.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

---

## Task 6: Implement `locale_path`

**Files:**
- Modify: `src/locale.c`

The user-aware variant. If user has a non-empty locale and that locale's file exists, returns it. Otherwise falls back to the default-locale path.

- [ ] **Step 1: Add the function to `src/locale.c`.**

```c
int
locale_path(UR_OBJECT user, char *out, size_t outlen,
            const char *category, const char *name)
{
    int n;
    /* Try user's locale if they have one set. */
    if (user && user->locale[0]) {
        n = snprintf(out, outlen, "%s/%s/%s/%s",
                     LANGS_ROOT, user->locale, category, name);
        if (n > 0 && (size_t) n < outlen && file_exists(out)) {
            return 2;
        }
    }
    /* Fall back to default locale. */
    n = snprintf(out, outlen, "%s/%s/%s/%s",
                 LANGS_ROOT, amsys->default_locale, category, name);
    if (n < 0 || (size_t) n >= outlen) return 0;
    return file_exists(out) ? 1 : 0;
}
```

- [ ] **Step 2: Add `locale[LOCALE_NAME_LEN]` to `UR_OBJECT`.**

In `src/includes/globals.h`, find `struct user_struct { ... }`. Add (near other small char-array fields like `name`, `pass`):

```c
char locale[LOCALE_NAME_LEN];   /* "" => use default */
```

We don't yet wire user-file load/save to persist this — that's Phase 2. For Phase 1 the field is always empty at runtime, so `locale_path` always uses the default-locale branch. The point of declaring it now is that Phase 1 call sites can use `locale_path(user, ...)` without compile errors; behaviour is identical to `locale_default_path` until Phase 2 wires up persistence + `set lang`.

- [ ] **Step 3: Build.**

```bash
make compile
```

Expected: builds cleanly. The new `user->locale` field is zero-initialised in the same place all other `UR_OBJECT` fields are (the `create_user` or equivalent factory function — no edits required if it does `calloc`).

- [ ] **Step 4: Commit.**

```bash
git add src/locale.c src/includes/globals.h
git commit -m "$(printf 'Implement locale_path with user-locale fallback to default\n\nAdds user->locale field (zero-init via existing calloc). Phase 1 leaves\nthe field empty at runtime, so locale_path behaves as locale_default_path.\nPhase 2 will wire up persistence and the set lang command.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

---

## Task 7: Audit all call sites that touch the six relocated categories

**Files:**
- (read-only) all files matching the constants

We need a complete inventory before doing the sweep, so the sweep tasks can list exact sites. The output of this task is a checklist consumed by Tasks 11–16.

- [ ] **Step 1: List every site that references one of the six category constants.**

```bash
semble search "DATAFILES path filename" . --top-k 20
semble search "HELPFILES fopen filename" . --top-k 20
semble search "MOTDFILES motd filename" . --top-k 20
semble search "TEXTFILES text filename" . --top-k 20
semble search "MISCFILES filename" . --top-k 20
semble search "ADMINFILES admin filename" . --top-k 20
```

For each result, capture: file path, line number, the function it's in, and the operation (`fopen` / `more` / `opendir` / `sprintf` building a path).

- [ ] **Step 2: Cross-check via Grep for completeness.**

The Grep tool is exhaustive for literal-token matching, which is exactly what we need for a sweep audit. Run:

```
Grep pattern: \\bDATAFILES\\b|\\bHELPFILES\\b|\\bMOTDFILES\\b|\\bTEXTFILES\\b|\\bMISCFILES\\b|\\bADMINFILES\\b
glob: *.c
output_mode: content
-n: true
-C: 1
```

The literal-match count should agree with the semble results plus a small handful of definition references in headers. Any difference is an audit gap.

- [ ] **Step 3: Write the audit ledger.**

Create `docs/superpowers/plans/2026-05-10-localisation-phase1-audit.md`. For each of the six constants, list each call site as a checklist item, e.g.:

```markdown
## DATAFILES
- [ ] `src/amnuts.c:1037` — `load_and_parse_config()` — `sprintf(filename, "%s/%s", DATAFILES, confile); fopen(filename, "r");`
- [ ] `src/commands/listbans.c:NN` — ...
...

## HELPFILES
- [ ] ...
```

This audit ledger is the working checklist for Tasks 11–16. Each sweep task ticks off entries as it goes.

- [ ] **Step 4: Commit.**

```bash
git add docs/superpowers/plans/2026-05-10-localisation-phase1-audit.md
git commit -m "$(printf 'Audit ledger: call sites that touch relocated categories\n\nComplete inventory of every reference to ADMINFILES/DATAFILES/HELPFILES/\nMISCFILES/MOTDFILES/TEXTFILES in source. Drives the sweep tasks.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

---

## Task 8: Define the per-site conversion pattern

This task is documentation only — no code. It establishes the exact pattern Tasks 11–16 follow, so the sweeps don't drift in style.

- [ ] **Step 1: Capture the canonical before/after in the audit ledger.**

Append to `docs/superpowers/plans/2026-05-10-localisation-phase1-audit.md`:

````markdown
## Conversion pattern

**Before — boot-time / global lookup (no user context):**

```c
sprintf(filename, "%s/%s", DATAFILES, confile);
fp = fopen(filename, "r");
if (!fp) { ... }
```

**After:**

```c
if (!locale_default_path(filename, sizeof filename, DATAFILES, confile)) {
    /* file genuinely missing; behave as before */
}
fp = fopen(filename, "r");
if (!fp) { ... }
```

**Before — per-user lookup (user struct in scope):**

```c
sprintf(filename, "%s/%s", HELPFILES, word[1]);
more(user, user->socket, filename);
```

**After:**

```c
if (locale_path(user, filename, sizeof filename, HELPFILES, word[1])) {
    more(user, user->socket, filename);
} else {
    write_user(user, "No such help file.\n");
}
```

**Notes:**
- The constant moves from being part of the `sprintf` format string to
  being the `category` argument of `locale_*_path`.
- The constants' string values flip from absolute to bare in Task 9 — but
  the *call shape* above works with either, because the helpers always
  prepend `LANGS_ROOT/<locale>/`. Until Task 9 flips them, the helpers
  produce a doubled path that won't resolve, so Task 9 MUST come before
  the sweep tasks run with real verification.
- Pure existence checks (`stat(DATAFILES "/foo", ...)`) follow the same
  pattern: call the resolver, then operate on the path it wrote.
- For `opendir(DATAFILES)` style calls (rare — directory enumeration of
  the category root itself), use:

  ```c
  char dir[PATH_MAX];
  snprintf(dir, sizeof dir, "%s/%s/%s",
           LANGS_ROOT, locale_default(), DATAFILES);
  dirp = opendir(dir);
  ```

  Don't try to resolve via `locale_*_path` for these — the helpers resolve
  files, not directories.
````

- [ ] **Step 2: Commit.**

```bash
git add docs/superpowers/plans/2026-05-10-localisation-phase1-audit.md
git commit -m "$(printf 'Document the per-site conversion pattern\n\nLocks in the shape every sweep task uses so style doesn'\''t drift across\nthe six per-category commits.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

---

## Task 9: Move content directories + temp-update absolute constants

**Files:**
- File-system moves (tracked by `git mv`)
- Modify: `src/includes/defines.h`

We move the six content directories into `files/langs/en_GB/` AND simultaneously update the absolute-path constants in `defines.h` to point at the new locations. The constants remain absolute (just with a longer path) so every existing `sprintf(path, "%s/%s", DATAFILES, ...)` site still resolves correctly at runtime. This keeps the talker bootable at every commit. The bare-name flip for each constant happens piece-meal in Tasks 11–16 (alongside that category's per-site sweep).

- [ ] **Step 1: Create the locale tree.**

```bash
mkdir -p files/langs/en_GB
```

- [ ] **Step 2: Move the directories.**

```bash
git mv files/datafiles  files/langs/en_GB/datafiles
git mv files/helpfiles  files/langs/en_GB/helpfiles
git mv files/miscfiles  files/langs/en_GB/miscfiles
git mv files/motds      files/langs/en_GB/motds
git mv files/textfiles  files/langs/en_GB/textfiles
mkdir -p files/langs/en_GB/adminfiles
touch files/langs/en_GB/adminfiles/.gitkeep
```

- [ ] **Step 3: Update absolute constants in `defines.h`.**

Open `src/includes/defines.h`. Change the six category lines so they point at the new tree:

```c
/* Translatable categories — temporarily absolute paths pointing at the
 * default-locale tree. Tasks 11-16 flip each one to a bare category name
 * as their call sites are converted to use locale_*_path. */
#define ADMINFILES  LANGS_ROOT "/" DEFAULT_LOCALE_NAME "/adminfiles"
#define DATAFILES   LANGS_ROOT "/" DEFAULT_LOCALE_NAME "/datafiles"
#define HELPFILES   LANGS_ROOT "/" DEFAULT_LOCALE_NAME "/helpfiles"
#define MISCFILES   LANGS_ROOT "/" DEFAULT_LOCALE_NAME "/miscfiles"
#define MOTDFILES   LANGS_ROOT "/" DEFAULT_LOCALE_NAME "/motds"
#define TEXTFILES   LANGS_ROOT "/" DEFAULT_LOCALE_NAME "/textfiles"
```

(`LANGS_ROOT` and `DEFAULT_LOCALE_NAME` were added in Task 2, so this composes cleanly. The C preprocessor concatenates these adjacent string literals at compile time.)

The non-translatable constants (`DUMPFILES`, `LOGFILES`, `MAILSPOOL`, `PICTFILES`, `USERFILES`) stay unchanged.

- [ ] **Step 4: Add `default_language` to the config file.**

The config file just moved to `files/langs/en_GB/datafiles/config`. Open it and add (in the `INIT` section, alongside other init options):

```
default_language en_GB
```

- [ ] **Step 5: Build + run.**

```bash
make build
./amnutsTalker
```

Expected: boots cleanly. Every `fopen("files/langs/en_GB/datafiles/<file>")` resolves to the moved location. The `Localisation: discovered ...` line does NOT appear yet — that's wired in Task 10. Connect via telnet, log in, run a few commands, verify nothing's broken, then stop.

- [ ] **Step 6: Commit.**

```bash
git add files/langs/ src/includes/defines.h -A
git commit -m "$(printf 'Relocate content directories; point constants at new tree\n\nThe six translatable category directories move under files/langs/en_GB/.\nThe category constants stay absolute for now, pointing at the new\nlocations, so all existing fopen sites continue to work. Tasks 11-16\nflip each constant to a bare name as its call sites are converted.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

---

## Task 10: Wire `locale_load_all` into boot

**Files:**
- Modify: `src/amnuts.c`

Now that `files/langs/en_GB/` exists, the discovery + validation can run without hard-failing.

- [ ] **Step 1: Call `locale_load_all` after config parsing.**

In `src/amnuts.c`, find where `load_and_parse_config()` is called from `main()`. Immediately AFTER that call (so `amsys->default_locale` is populated), add:

```c
locale_load_all();
```

If `locale.h` isn't already included in `amnuts.c`, add `#include "locale.h"` to the existing include block at the top of the file.

- [ ] **Step 2: Build + boot.**

```bash
make build
./amnutsTalker
```

Expected: boots cleanly. The startup log now contains a line like:

```
Localisation: discovered 1 locale(s); default = en_GB.
```

Connect, log in, run a few commands, verify nothing's broken, stop.

- [ ] **Step 3: Commit.**

```bash
git add src/amnuts.c
git commit -m "$(printf 'Wire locale_load_all into boot sequence\n\nDiscovery + default-locale validation runs after config parsing.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

---

## Tasks 11–16: Per-constant sweep + flip

These six tasks follow the same shape. For each constant in turn (alphabetical: `ADMINFILES`, `DATAFILES`, `HELPFILES`, `MISCFILES`, `MOTDFILES`, `TEXTFILES`):

1. Convert every call site identified in the Task 7 audit ledger to use `locale_path` or `locale_default_path` per the conversion pattern in Task 8.
2. Flip the constant in `defines.h` from its temporary absolute path (`LANGS_ROOT "/" DEFAULT_LOCALE_NAME "/<cat>"`) to a bare category name (`"<cat>"`).
3. Build, smoke-test, commit.

Sweeping site-conversion and constant-flip in the same commit keeps the talker bootable at every commit: as long as a constant is still absolute, unconverted sites still work; once the constant flips to bare, every site for that constant must already be using the resolver.

### Task 11: ADMINFILES

**Files:** every site in the audit ledger's `## ADMINFILES` section; `src/includes/defines.h`.

- [ ] **Step 1: Convert each ADMINFILES site.** Use `locale_default_path` for boot/admin-internal sites, `locale_path(user, ...)` when a `UR_OBJECT user` is in scope.

- [ ] **Step 2: Flip the constant.** In `src/includes/defines.h`, change:

```c
#define ADMINFILES  LANGS_ROOT "/" DEFAULT_LOCALE_NAME "/adminfiles"
```

to:

```c
#define ADMINFILES  "adminfiles"
```

- [ ] **Step 3: Build + run.**

```bash
make build
./amnutsTalker
```

Expected: builds cleanly, boots cleanly. Smoke-test any admin command that touches adminfiles. Stop the talker.

- [ ] **Step 4: Verify there are no stragglers.** Use the Grep tool with pattern `ADMINFILES.*%s` (multiline mode). Expect no results — every site should now pass `ADMINFILES` as a category argument, not as part of a sprintf format.

- [ ] **Step 5: Commit.**

```bash
git add -u
git commit -m "$(printf 'Sweep ADMINFILES sites and flip constant to bare\n\nEvery ADMINFILES call site now resolves through locale_*_path; the\nconstant flips from an absolute path to the bare category name.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

### Task 12: DATAFILES

**Files:** every site in the audit ledger's `## DATAFILES` section; `src/includes/defines.h`. **Note:** the boot-time config load lives here (around `src/amnuts.c:1037` in `load_and_parse_config`); take extra care.

- [ ] **Step 1: Convert each DATAFILES site.** For the boot-time config load specifically:

```c
/* Before */
sprintf(filename, "%s/%s", DATAFILES, confile);
fp = fopen(filename, "r");

/* After */
locale_default_path(filename, sizeof filename, DATAFILES, confile);
fp = fopen(filename, "r");
```

(The return value of `locale_default_path` indicates whether the file currently exists, but the existing `fopen` failure handling below covers the missing-file case adequately — no need to act on the return value.)

For `opendir(DATAFILES)` style calls in `parse_user_rooms` or similar, follow the pattern from Task 8 Step 1's "Notes" subsection — manually compose `<LANGS_ROOT>/<default_locale>/<category>` since the helpers are for files, not directories.

- [ ] **Step 2: Flip the constant.**

```c
#define DATAFILES   "datafiles"
```

- [ ] **Step 3: Build + boot.**

```bash
make build
./amnutsTalker
```

Expected: boots cleanly, config loads from the new location. Connect, log in, walk through a couple of rooms (verifies the .R files load from the new tree), stop.

- [ ] **Step 4: Verify no stragglers via Grep** (pattern `DATAFILES.*%s`).

- [ ] **Step 5: Commit.**

```bash
git add -u
git commit -m "$(printf 'Sweep DATAFILES sites and flip constant to bare\n\nIncludes the boot-time config load and room-file loaders. The constant\nis now a bare category name; absolute path resolution moves entirely\ninto the locale helpers.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

### Task 13: HELPFILES

**Files:** sites from the audit ledger's `## HELPFILES`; `src/includes/defines.h`.

- [ ] **Step 1: Convert each HELPFILES site.** Use `locale_path(user, ...)` — help files are always shown to a connected user.

- [ ] **Step 2: Flip the constant.**

```c
#define HELPFILES   "helpfiles"
```

- [ ] **Step 3: Build + smoke-test.**

```bash
make build
./amnutsTalker
```

Connect via telnet, log in, run `help` on a few topics, verify content. Stop.

- [ ] **Step 4: Verify no stragglers** via Grep (pattern `HELPFILES.*%s`).

- [ ] **Step 5: Commit.**

```bash
git add -u
git commit -m "$(printf 'Sweep HELPFILES sites and flip constant to bare\n\nUses the per-user resolver so future non-default locales automatically\npick up their translated help articles.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

### Task 14: MISCFILES

**Files:** sites from the audit ledger's `## MISCFILES`; `src/includes/defines.h`.

- [ ] **Step 1: Convert each MISCFILES site.** Choose `locale_path` vs `locale_default_path` per whether the file is user-visible content (per-user) or admin/boot lookup (default).

- [ ] **Step 2: Flip the constant.**

```c
#define MISCFILES   "miscfiles"
```

- [ ] **Step 3: Build + smoke-test** any command that touches miscfiles.

- [ ] **Step 4: Verify no stragglers** via Grep (pattern `MISCFILES.*%s`).

- [ ] **Step 5: Commit.**

```bash
git add -u
git commit -m "$(printf 'Sweep MISCFILES sites and flip constant to bare\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

### Task 15: MOTDFILES

**Files:** sites from the audit ledger's `## MOTDFILES`; `src/includes/defines.h`.

- [ ] **Step 1: Convert each MOTDFILES site.** Use `locale_path(user, ...)` — MOTD is shown to a user; the user's locale (when one is set in Phase 2) will then automatically pick up the per-locale MOTD.

- [ ] **Step 2: Flip the constant.**

```c
#define MOTDFILES   "motds"
```

- [ ] **Step 3: Build + connect** to verify the MOTD displays correctly on login.

- [ ] **Step 4: Verify no stragglers** via Grep (pattern `MOTDFILES.*%s`).

- [ ] **Step 5: Commit.**

```bash
git add -u
git commit -m "$(printf 'Sweep MOTDFILES sites and flip constant to bare\n\nMOTD now resolves via the per-user helper, ready for per-locale MOTDs\nonce Phase 2 wires up set lang.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

### Task 16: TEXTFILES

**Files:** sites from the audit ledger's `## TEXTFILES`; `src/includes/defines.h`.

- [ ] **Step 1: Convert each TEXTFILES site.** Use `locale_path` for user-pager calls (`news`, `rules`, `info`), `locale_default_path` for any boot-time lookup.

- [ ] **Step 2: Flip the constant.**

```c
#define TEXTFILES   "textfiles"
```

- [ ] **Step 3: Build + smoke-test** the textfile-display commands.

- [ ] **Step 4: Verify no stragglers** via Grep (pattern `TEXTFILES.*%s`).

- [ ] **Step 5: Commit.**

```bash
git add -u
git commit -m "$(printf 'Sweep TEXTFILES sites and flip constant to bare\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

---

## Task 17: End-to-end verification with a partial fallback locale

**Files:**
- Create: `files/langs/fallback_test/motds/<one motd file>` (one MOTD only, with visibly-different content)

This task proves mechanism 2's fallback works end-to-end — the central new behaviour Phase 1 introduces.

- [ ] **Step 1: Create a partial fallback test locale.**

```bash
mkdir -p files/langs/fallback_test/motds
```

Copy one MOTD file from `files/langs/en_GB/motds/` into `files/langs/fallback_test/motds/`, then edit it to contain a single visibly-distinct line such as `THIS IS THE FALLBACK_TEST MOTD\n`.

- [ ] **Step 2: Build + boot.**

```bash
make build
./amnutsTalker
```

Expected: the boot log shows `"Localisation: discovered 2 locale(s); default = en_GB."`. The fallback_test locale is discovered alongside en_GB but is not the default.

- [ ] **Step 3: Smoke-test as a normal user.**

Connect, log in. Without setting any locale (you can't yet — Phase 2 adds `set lang`), all paths resolve via the default (en_GB). Verify a few commands render correctly.

- [ ] **Step 4: Force a user into the fallback_test locale by direct DB edit, just for this verification.**

This is a temporary verification hack — Phase 2 will replace it with `set lang`. Stop the talker. Edit `files/userfiles/<your test user>.D` (the user data file — find it via `ls files/userfiles/`), and locate the field corresponding to `locale` (it's the new field added in Task 6 — likely the last char-array field in the user struct). Set its value to `fallback_test`.

Or, easier: temporarily, in `src/locale.c`, set `user->locale` to `"fallback_test"` in a debug `printf` injection at the top of `locale_path`, just for this verification. Remove the injection after.

Boot the talker, log in as that user, run the MOTD display command. Expected: see `"THIS IS THE FALLBACK_TEST MOTD"` (because `motds/<that file>` exists in `fallback_test/`). Run a `help` command. Expected: see the English help content (because `helpfiles/` is empty in `fallback_test/`, so the resolver falls back to `en_GB/helpfiles/`).

- [ ] **Step 5: Revert any debug injection** and re-run `make build`.

- [ ] **Step 6: Remove the verification artifact** (or keep `files/langs/fallback_test/` if you want to keep a test locale around — it's harmless. Recommendation: keep it as a permanent dev/test fixture):

```bash
# Either:
rm -rf files/langs/fallback_test
# OR keep it and commit it as a permanent test fixture
git add files/langs/fallback_test
```

- [ ] **Step 7: Write the migration ledger entry.**

Create `docs/superpowers/specs/localisation-migration.md` if it doesn't exist:

```markdown
# Localisation Migration Ledger

| Phase | Status | Notes |
|-------|--------|-------|
| 1 — file-path mechanism + directory move | converted (YYYY-MM-DD) | All six relocated categories swept; libyaml vendored; locale_load_all wired into boot |
| 2 — catalog framework | pending | |
| 3 — UI builders | pending | |
| 4 — frame-heavy commands | pending | |
| 5 — bulk inline-string conversion | pending | per-command sub-ledger below |
| 6 — first non-default locale | pending | |
```

Replace `YYYY-MM-DD` with the actual completion date.

- [ ] **Step 8: Final commit.**

```bash
git add docs/superpowers/specs/localisation-migration.md files/langs/fallback_test/ 2>/dev/null
git add -u
git commit -m "$(printf 'Phase 1 complete: file-path mechanism + directory move verified\n\nFallback resolution verified end-to-end with a partial test locale.\nMigration ledger seeded.\n\nCo-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>\n')"
```

---

## Self-review checklist

- [ ] Six category constants flip from absolute to bare names piece-meal in Tasks 11–16: `ADMINFILES`, `DATAFILES`, `HELPFILES`, `MISCFILES`, `MOTDFILES`, `TEXTFILES`. ✓ Listed correctly.
- [ ] `LANGS_ROOT`, `DEFAULT_LOCALE_NAME`, `LOCALE_NAME_LEN` added to `defines.h` (Task 2).
- [ ] `default_locale` field on `amsys` parsed from `default_language` config option (Task 3).
- [ ] `locale_default_path` (Task 4), `locale_load_all` + discovery (Task 5), `locale_path` (Task 6) implemented.
- [ ] `user->locale` field declared (Task 6); persistence + `set lang` deferred to Phase 2 — documented.
- [ ] Audit ledger captures all call sites (Task 7); conversion pattern documented (Task 8).
- [ ] Directory move + temp absolute constants keeps talker bootable (Task 9); boot wiring added once tree exists (Task 10).
- [ ] Six sweep+flip tasks (11–16) each fully commit a single category, talker bootable after each.
- [ ] End-to-end verification with a real fallback locale (Task 17).
- [ ] libyaml vendored but unused in Phase 1 (Task 1) — Phase 2 will consume it.
- [ ] Spec sections covered: §3 (architecture), §5 (file path mechanism), §3.3 (config), boot wiring per §8.1. Spec §4 (catalog), §6 (UI builders), §7 (user commands) are Phase 2/3 — out of scope here.
- [ ] Talker bootable at every commit after Task 9 (verified by the build+run step in each subsequent task).
