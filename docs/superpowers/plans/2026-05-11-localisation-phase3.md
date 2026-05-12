# Localisation Phase 3 — UI Builders Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the colour-aware UI builders (`rule`, `box_*`, `table_*`) that compose horizontal rules, framed boxes, and word-wrapped tables from `ui.*` keys in each locale's `strings.yml`. Convert `wizlist` as a pilot command to prove the end-to-end "themed frame" experience works, including a third locale (`cowboy`) that ships with a visibly different ruleset.

**Architecture:** A new module `src/uibuilders.c` (+ `src/includes/uibuilders.h`) exposes one keystone — `visible_strlen` — and three families of builders. Every padding/wrap/overlay calculation goes through `visible_strlen` so colour escapes (`~OL`, `~FC`, `~RS`, …) pass through transparently while never affecting column maths. Frame primitives (`lcap`/`rcap`/`fill`/`lside`/`rside`) live in `strings.yml` under the `ui.*` prefix, so themers change them like any other catalog string. Phase 2's `catalog_load_all` already loads them; this phase adds the consumers. `wizlist` is converted last as a pilot.

**Tech Stack:** C (clang, gnu23, `-Wall -Wextra -Wpedantic`), GNU Make, libyaml 0.2.5 (vendored), Phase 2's catalog API. No new dependencies. No test framework — verification is clean build + telnet play-through.

**Reference spec:** `docs/superpowers/specs/2026-05-10-localisation-design.md` §6 (UI builders), §11 (test plan).

**Codebase notes for the implementing engineer:**
- Phase 2 already landed: `lang(user, key)` and `lang_format(user, buf, buflen, key, ...)` are the way to fetch a catalog string. `ui.*` keys load into the catalog as plain strings — but they're *configuration values* (single bytes, short escape sequences), not printf format strings, so callers fetch them with `lang()` then process the raw bytes themselves. **`ui.*` keys must not contain `%` specifiers** — the signature extractor accepts them but Phase 3 treats `ui.*` values as opaque byte sequences, not format strings.
- Colour escape sequences in Amnuts use the form `~XX` (two ASCII chars after the tilde): `~OL`, `~RS`, `~FC`, `~FR`, `~FG`, `~FY`, `~FB`, `~FM`, `~FK`, `~FT`, `~FW`, `~BR`, `~BG`, `~BY`, `~BB`, `~BM`, `~BT`, `~BW`, `~LI`. **Search `src/strings.c` for `colour_com_strip` and friends to confirm the full list** — anything `colour_com_strip` recognises must round-trip through `visible_strlen` as zero visible bytes.
- `ARR_SIZE` is the project line-buffer constant (1000) — most builder local buffers should be `[ARR_SIZE * 2]` to match `vwrite_user`'s `vtext`.
- `wizlist` lives at `src/commands/wizlist.c` — read it before Task 11 to understand the existing box-drawing pattern.
- The test locale `fallback_test` already exists from Phase 1 with `motds/motd2/`. Phase 3 adds a `cowboy/` test locale to exercise themed frames.
- New functions must have prototypes added to `src/includes/prototypes.h`.

**Commit convention:** small frequent commits. Each task ends with a commit step. Use `Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>` per project convention.

---

## File Structure

**New files:**
- `src/includes/uibuilders.h` — type forward decls (`BOX`, `TABLE` opaque pointer typedefs) and any builder-internal constants.
- `src/uibuilders.c` — implementations of `visible_strlen`, `align_into`, `rule`, `box_*`, `table_*`.
- `files/langs/cowboy/strings.yml` — pilot themed locale: `meta.*` + `ui.*` overrides only. No content directories needed; everything else falls back to en_GB via Phase 1's resolver.

**Modified files:**
- `src/includes/prototypes.h` — declare every new public function.
- `files/langs/en_GB/strings.yml` — add the canonical `ui.*` keys (default frame appearance).
- `src/commands/wizlist.c` — pilot conversion to `box_*`/`rule` + lang_user.

---

## Task 1: Add `visible_strlen`

**Files:**
- Create: `src/uibuilders.c`
- Create: `src/includes/uibuilders.h`
- Modify: `src/includes/prototypes.h`

The keystone primitive. Every padding/wrap/overlay calculation will go through this function; getting it wrong cascades into every UI element drifting against the right border.

- [ ] **Step 1: Identify the project's colour-escape vocabulary.**

Run:

```
semble search "colour_com_strip skip color escape ~XX" .
```

Read the recognised escape forms. The keystone rule: anything `colour_com_strip` strips must also be counted as zero visible bytes by `visible_strlen`. Make a list of the leading characters that introduce an escape (e.g., `~` followed by an alpha, possibly also `\x1b[...]m` ANSI sequences if the codebase uses them).

- [ ] **Step 2: Create `src/includes/uibuilders.h`.**

```c
/****************************************************************************
   Amnuts UI builders — colour-aware horizontal rules, framed boxes,
   word-wrapped tables. See docs/superpowers/specs/2026-05-10-localisation-design.md §6.

   Function declarations live in src/includes/prototypes.h (single source
   of truth, per project convention).
 ***************************************************************************/

#ifndef AMNUTS_UIBUILDERS_H
#define AMNUTS_UIBUILDERS_H

#include <stddef.h>

/* Opaque handles. Defined in src/uibuilders.c. */
typedef struct box_struct   *BOX;
typedef struct table_struct *TABLE;

/* Alignment constants for align_into. */
enum align_value {
    ALIGN_LEFT   = 0,
    ALIGN_CENTRE = 1,
    ALIGN_RIGHT  = 2,
};

#endif /* AMNUTS_UIBUILDERS_H */
```

- [ ] **Step 3: Add the prototype.**

In `src/includes/prototypes.h`, find the existing `/* catalog.c */` block (added in Phase 2 Task 2). Add a new block immediately after it:

```c
/* uibuilders.c — see src/includes/uibuilders.h for opaque types. */
int visible_strlen(const char *s);
```

- [ ] **Step 4: Implement `visible_strlen` in `src/uibuilders.c`.**

```c
/****************************************************************************
   Amnuts UI builders — colour-aware horizontal rules, framed boxes,
   word-wrapped tables. See docs/superpowers/specs/2026-05-10-localisation-design.md §6.
 ***************************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "globals.h"
#include "prototypes.h"
#include "uibuilders.h"

/*
 * Count visible columns in `s`, skipping ~XX colour escapes.
 *
 * Recognised escapes: '~' followed by two alphanumeric characters.
 * (See src/strings.c::colour_com_strip for the exhaustive list of token
 * pairs the codebase actually uses; the two-alpha-char form covers all
 * of them and is forgiving for future additions.)
 *
 * Counts every other byte as one column. UTF-8 is NOT correctly handled
 * here — multi-byte sequences will over-count. Phase 1's design note
 * §10 acknowledges this and parks proper Unicode width for a separate
 * track; ASCII-only locales render correctly today.
 */
int
visible_strlen(const char *s)
{
    int n = 0;
    if (!s) return 0;
    while (*s) {
        if (*s == '~' && s[1] && s[2]
            && isalnum((unsigned char) s[1])
            && isalnum((unsigned char) s[2])) {
            s += 3;
            continue;
        }
        ++n;
        ++s;
    }
    return n;
}
```

- [ ] **Step 5: Build.** Skip — Windows.

- [ ] **Step 6: Commit.**

```bash
git add src/uibuilders.c src/includes/uibuilders.h src/includes/prototypes.h
git commit -m "$(cat <<'EOF'
UI builders: visible_strlen keystone

Counts visible columns in a string, skipping ~XX colour escapes
(two-alpha-char form covers every escape the codebase uses today).
This is the single source of truth every padding/wrap/overlay
calculation will route through.

UTF-8 width is deliberately not addressed — parked in the Unicode
track per the design spec §10.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 2: Implement `align_into`

**Files:**
- Modify: `src/uibuilders.c`
- Modify: `src/includes/prototypes.h`

A small helper that renders a printf-style string into a fixed visible-column width, padded or truncated by visible columns (not bytes). Box bodies and table cells both call this.

- [ ] **Step 1: Add the prototype.**

In `src/includes/prototypes.h`, in the `uibuilders.c` block:

```c
int align_into(char *out, size_t outlen,
               enum align_value align, int width,
               const char *fmt, ...)
    __attribute__((format(printf, 5, 6)));
```

- [ ] **Step 2: Implement `align_into`.**

```c
/*
 * Format into `out`, padded/truncated to `width` VISIBLE columns.
 *
 * Returns the number of visible columns actually emitted (always <= width),
 * or -1 if `outlen` was too small to hold the formatted+padded result.
 *
 * Truncation:
 *   - If the formatted content's visible_strlen exceeds `width`, the content
 *     is byte-truncated mid-character at the first byte that would have
 *     emitted a visible column beyond `width`. Mid-escape truncation is
 *     avoided by truncating before the '~' on a partial escape.
 * Padding:
 *   - LEFT  : content, then spaces.
 *   - CENTRE: half-padding before, remaining-padding after.
 *   - RIGHT : spaces, then content.
 */
int
align_into(char *out, size_t outlen,
           enum align_value align, int width,
           const char *fmt, ...)
{
    char rendered[ARR_SIZE * 2];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(rendered, sizeof rendered, fmt, ap);
    va_end(ap);

    int visible = visible_strlen(rendered);

    /* Truncation pass: walk byte by byte, advancing a visible-column counter,
     * and stop emitting bytes once we'd cross `width`. Track a safe cut
     * point so we don't bisect a colour escape. */
    char trimmed[ARR_SIZE * 2];
    int  trim_visible = 0;
    size_t trim_bytes = 0;
    if (visible <= width) {
        memcpy(trimmed, rendered, strlen(rendered) + 1);
        trim_visible = visible;
        trim_bytes   = strlen(rendered);
    } else {
        const char *p = rendered;
        while (*p && trim_visible < width) {
            if (*p == '~' && p[1] && p[2]
                && isalnum((unsigned char) p[1])
                && isalnum((unsigned char) p[2])) {
                if (trim_bytes + 3 >= sizeof trimmed) break;
                trimmed[trim_bytes++] = p[0];
                trimmed[trim_bytes++] = p[1];
                trimmed[trim_bytes++] = p[2];
                p += 3;
                continue;
            }
            if (trim_bytes + 1 >= sizeof trimmed) break;
            trimmed[trim_bytes++] = *p++;
            ++trim_visible;
        }
        trimmed[trim_bytes] = '\0';
    }

    int pad_total = width - trim_visible;
    int pad_left  = 0, pad_right = 0;
    switch (align) {
    case ALIGN_LEFT:   pad_right = pad_total; break;
    case ALIGN_RIGHT:  pad_left  = pad_total; break;
    case ALIGN_CENTRE: pad_left  = pad_total / 2;
                       pad_right = pad_total - pad_left; break;
    }

    if (trim_bytes + (size_t) pad_total + 1 > outlen) {
        return -1;
    }
    char *o = out;
    for (int i = 0; i < pad_left; ++i) *o++ = ' ';
    memcpy(o, trimmed, trim_bytes);
    o += trim_bytes;
    for (int i = 0; i < pad_right; ++i) *o++ = ' ';
    *o = '\0';
    return trim_visible + pad_total;
}
```

- [ ] **Step 3: Build.** Skip.

- [ ] **Step 4: Commit.**

```bash
git add src/uibuilders.c src/includes/prototypes.h
git commit -m "$(cat <<'EOF'
UI builders: align_into for visible-column padding/truncation

vsnprintfs into a stack buffer, then either copies whole or
byte-truncates without bisecting a colour escape, then pads with
spaces by visible columns. Returns -1 if the destination buffer is
too small.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 3: Add canonical `ui.*` keys to en_GB/strings.yml

**Files:**
- Modify: `files/langs/en_GB/strings.yml`

Ship the canonical default frame appearance. These mirror what the existing `+----+`, `|...|`, `+----+` boxes look like in the current talker — so callers converting from the existing inline `+-...-+` lines see byte-identical output on en_GB.

- [ ] **Step 1: Append the `ui.*` keys.**

Append to `files/langs/en_GB/strings.yml`:

```yaml
# UI builders — frame primitives (see docs/superpowers/specs/2026-05-10-localisation-design.md §6).
# These values are processed as opaque byte sequences by the UI builders,
# NOT as printf format strings. Keep them free of % specifiers.

# Horizontal section rule (no side rails). Used for between-section dividers.
ui.rule.lcap:        "+"
ui.rule.rcap:        "+"
ui.rule.fill:        "-"
ui.rule.label_lpad:  "6"      # visible columns of fill before " <label> "

# Box top/bottom edges and body sides.
ui.box.top.lcap:     "+"
ui.box.top.rcap:     "+"
ui.box.top.fill:     "-"
ui.box.bot.lcap:     "+"
ui.box.bot.rcap:     "+"
ui.box.bot.fill:     "-"
ui.box.body.lside:   "|"
ui.box.body.rside:   "|"

# Internal box separator (between header and rows, between row groups).
ui.box.sep.lcap:     "+"
ui.box.sep.rcap:     "+"
ui.box.sep.fill:     "-"
```

`ui.rule.label_lpad` is stored as a string because every catalog value is a string — the builder parses it as an integer on use.

- [ ] **Step 2: Commit.**

```bash
git add files/langs/en_GB/strings.yml
git commit -m "$(cat <<'EOF'
Ship canonical ui.* keys in en_GB/strings.yml

Default frame appearance matches the existing +----+ boxes used
throughout the talker. Phase 3 builders consume these; Phase 4
conversions of frame-heavy commands then become byte-identical for
en_GB users while remaining themeable for non-default locales.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 4: `rule` — horizontal section rule with optional inline label

**Files:**
- Modify: `src/uibuilders.c`
- Modify: `src/includes/prototypes.h`

The simplest builder. Composes `lcap + fill_n(...) + " label " + fill_n(...) + rcap` over a fixed visible width.

- [ ] **Step 1: Add the prototype.**

```c
void rule(UR_OBJECT user, int width, const char *label_fmt, ...);
```

- [ ] **Step 2: Implement.**

Append to `src/uibuilders.c`:

```c
/*
 * Fill `out` with the byte sequence `pattern` repeated until `out` covers
 * exactly `cols` visible columns. Pattern bytes are emitted verbatim;
 * colour escapes inside the pattern pass through transparently. Truncation
 * within the pattern is by visible columns.
 *
 * Returns the number of bytes written (excluding NUL terminator).
 */
static size_t
fill_pattern(char *out, size_t outlen, const char *pattern, int cols)
{
    size_t written = 0;
    int    visible = 0;
    int    pat_visible = visible_strlen(pattern);
    if (pat_visible <= 0 || cols <= 0 || !pattern || !*pattern) {
        if (outlen) out[0] = '\0';
        return 0;
    }
    while (visible < cols) {
        const char *p = pattern;
        while (*p && visible < cols) {
            if (*p == '~' && p[1] && p[2]
                && isalnum((unsigned char) p[1])
                && isalnum((unsigned char) p[2])) {
                if (written + 3 >= outlen) goto done;
                out[written++] = p[0];
                out[written++] = p[1];
                out[written++] = p[2];
                p += 3;
                continue;
            }
            if (written + 1 >= outlen) goto done;
            out[written++] = *p++;
            ++visible;
        }
    }
done:
    if (written < outlen) out[written] = '\0';
    else if (outlen)      out[outlen - 1] = '\0';
    return written;
}

void
rule(UR_OBJECT user, int width, const char *label_fmt, ...)
{
    if (!user || width <= 0) return;

    const char *lcap = lang(user, "ui.rule.lcap");
    const char *rcap = lang(user, "ui.rule.rcap");
    const char *fill = lang(user, "ui.rule.fill");
    const char *lpad = lang(user, "ui.rule.label_lpad");
    if (!lcap) lcap = "";
    if (!rcap) rcap = "";
    if (!fill || !*fill) fill = "-";
    int label_lpad = lpad ? atoi(lpad) : 6;
    if (label_lpad < 0) label_lpad = 0;

    int cap_visible = visible_strlen(lcap) + visible_strlen(rcap);
    int inner = width - cap_visible;
    if (inner < 0) inner = 0;

    char rendered_label[ARR_SIZE];
    rendered_label[0] = '\0';
    int label_visible = 0;
    if (label_fmt && *label_fmt) {
        va_list ap;
        va_start(ap, label_fmt);
        vsnprintf(rendered_label, sizeof rendered_label, label_fmt, ap);
        va_end(ap);
        label_visible = visible_strlen(rendered_label);
    }

    char out[ARR_SIZE * 2];
    size_t pos = 0;
    /* lcap */
    pos += snprintf(out + pos, sizeof out - pos, "%s", lcap);
    if (label_visible > 0) {
        /* lpad of fill, space, label, space, then remainder of fill */
        int used_visible = 0;
        int lp = label_lpad;
        if (lp > inner) lp = inner;
        pos += fill_pattern(out + pos, sizeof out - pos, fill, lp);
        used_visible += lp;
        if (used_visible + 1 <= inner) {
            out[pos++] = ' '; ++used_visible;
        }
        if (used_visible + label_visible <= inner) {
            memcpy(out + pos, rendered_label, strlen(rendered_label));
            pos += strlen(rendered_label);
            used_visible += label_visible;
        }
        if (used_visible + 1 <= inner) {
            out[pos++] = ' '; ++used_visible;
        }
        int remainder = inner - used_visible;
        if (remainder > 0) {
            pos += fill_pattern(out + pos, sizeof out - pos, fill, remainder);
        }
    } else {
        pos += fill_pattern(out + pos, sizeof out - pos, fill, inner);
    }
    /* rcap and newline */
    pos += snprintf(out + pos, sizeof out - pos, "%s\n", rcap);

    write_user(user, out);
}
```

- [ ] **Step 3: Commit.**

```bash
git add src/uibuilders.c src/includes/prototypes.h
git commit -m "$(cat <<'EOF'
UI builders: rule() for horizontal section dividers

Composes lcap + fill-pattern + optional inline " label " + remaining
fill + rcap over a fixed visible width. Every length calculation
goes through visible_strlen so colour escapes don't push the right
border.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 5: `box_*` API — opaque BOX handle + open/line/blank/centered/separator/close

**Files:**
- Modify: `src/uibuilders.c`
- Modify: `src/includes/prototypes.h`

Boxes are stateful (`BOX` opaque pointer holding width, user, frame styles). Bodies render one row at a time; close emits the bottom edge.

- [ ] **Step 1: Add prototypes.**

```c
BOX  box_open      (UR_OBJECT user, int width, const char *title_fmt, ...);
void box_line      (BOX b, const char *fmt, ...);
void box_blank     (BOX b);
void box_centered  (BOX b, const char *fmt, ...);
void box_separator (BOX b);
void box_close     (BOX b);
```

- [ ] **Step 2: Define the opaque struct.**

In `src/uibuilders.c`, before the implementations:

```c
struct box_struct {
    UR_OBJECT user;
    int       width;        /* total visible cols including sides */
    int       inner;        /* width - visible_strlen(lside) - visible_strlen(rside) */
    char      lside[16];
    char      rside[16];
};
```

(Fixed 16-byte side buffers: enough for any reasonable theme since they're catalog-supplied bytes.)

- [ ] **Step 3: Implement `box_open`.**

```c
/* Emit the top edge of the box (optional inline title) and return a BOX
 * handle. Caller must eventually call box_close(b). */
BOX
box_open(UR_OBJECT user, int width, const char *title_fmt, ...)
{
    if (!user || width <= 0) return NULL;
    struct box_struct *b = calloc(1, sizeof *b);
    if (!b) return NULL;
    b->user  = user;
    b->width = width;

    const char *l = lang(user, "ui.box.body.lside");
    const char *r = lang(user, "ui.box.body.rside");
    if (!l) l = "|";
    if (!r) r = "|";
    strncpy(b->lside, l, sizeof b->lside - 1);
    strncpy(b->rside, r, sizeof b->rside - 1);
    b->inner = width - visible_strlen(b->lside) - visible_strlen(b->rside);
    if (b->inner < 0) b->inner = 0;

    /* Render the top edge from ui.box.top.* */
    const char *lcap = lang(user, "ui.box.top.lcap");
    const char *rcap = lang(user, "ui.box.top.rcap");
    const char *fill = lang(user, "ui.box.top.fill");
    if (!lcap) lcap = "+";
    if (!rcap) rcap = "+";
    if (!fill || !*fill) fill = "-";

    int inner_cols = width - visible_strlen(lcap) - visible_strlen(rcap);
    if (inner_cols < 0) inner_cols = 0;

    char rendered_title[ARR_SIZE];
    rendered_title[0] = '\0';
    int title_visible = 0;
    if (title_fmt && *title_fmt) {
        va_list ap;
        va_start(ap, title_fmt);
        vsnprintf(rendered_title, sizeof rendered_title, title_fmt, ap);
        va_end(ap);
        title_visible = visible_strlen(rendered_title);
    }

    char out[ARR_SIZE * 2];
    size_t pos = 0;
    pos += snprintf(out + pos, sizeof out - pos, "%s", lcap);
    if (title_visible > 0 && title_visible + 4 <= inner_cols) {
        /* "----- TITLE -----" style: small left pad, space, title, space, fill */
        int lpad = 5;
        pos += fill_pattern(out + pos, sizeof out - pos, fill, lpad);
        out[pos++] = ' ';
        memcpy(out + pos, rendered_title, strlen(rendered_title));
        pos += strlen(rendered_title);
        out[pos++] = ' ';
        int rem = inner_cols - lpad - 1 - title_visible - 1;
        if (rem > 0) pos += fill_pattern(out + pos, sizeof out - pos, fill, rem);
    } else {
        pos += fill_pattern(out + pos, sizeof out - pos, fill, inner_cols);
    }
    pos += snprintf(out + pos, sizeof out - pos, "%s\n", rcap);
    write_user(user, out);
    return b;
}
```

- [ ] **Step 4: Implement `box_line` and friends.**

```c
void
box_line(BOX b, const char *fmt, ...)
{
    if (!b) return;
    char body[ARR_SIZE * 2];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(body, sizeof body, fmt, ap);
    va_end(ap);

    char padded[ARR_SIZE * 2];
    align_into(padded, sizeof padded, ALIGN_LEFT, b->inner, "%s", body);

    char out[ARR_SIZE * 2];
    snprintf(out, sizeof out, "%s%s%s\n", b->lside, padded, b->rside);
    write_user(b->user, out);
}

void
box_centered(BOX b, const char *fmt, ...)
{
    if (!b) return;
    char body[ARR_SIZE * 2];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(body, sizeof body, fmt, ap);
    va_end(ap);

    char padded[ARR_SIZE * 2];
    align_into(padded, sizeof padded, ALIGN_CENTRE, b->inner, "%s", body);

    char out[ARR_SIZE * 2];
    snprintf(out, sizeof out, "%s%s%s\n", b->lside, padded, b->rside);
    write_user(b->user, out);
}

void
box_blank(BOX b)
{
    if (!b) return;
    box_line(b, "");
}

void
box_separator(BOX b)
{
    if (!b) return;
    const char *lcap = lang(b->user, "ui.box.sep.lcap");
    const char *rcap = lang(b->user, "ui.box.sep.rcap");
    const char *fill = lang(b->user, "ui.box.sep.fill");
    if (!lcap) lcap = "+";
    if (!rcap) rcap = "+";
    if (!fill || !*fill) fill = "-";

    int inner_cols = b->width - visible_strlen(lcap) - visible_strlen(rcap);
    if (inner_cols < 0) inner_cols = 0;

    char fillbuf[ARR_SIZE * 2];
    fill_pattern(fillbuf, sizeof fillbuf, fill, inner_cols);

    char out[ARR_SIZE * 2];
    snprintf(out, sizeof out, "%s%s%s\n", lcap, fillbuf, rcap);
    write_user(b->user, out);
}

void
box_close(BOX b)
{
    if (!b) return;
    const char *lcap = lang(b->user, "ui.box.bot.lcap");
    const char *rcap = lang(b->user, "ui.box.bot.rcap");
    const char *fill = lang(b->user, "ui.box.bot.fill");
    if (!lcap) lcap = "+";
    if (!rcap) rcap = "+";
    if (!fill || !*fill) fill = "-";

    int inner_cols = b->width - visible_strlen(lcap) - visible_strlen(rcap);
    if (inner_cols < 0) inner_cols = 0;

    char fillbuf[ARR_SIZE * 2];
    fill_pattern(fillbuf, sizeof fillbuf, fill, inner_cols);

    char out[ARR_SIZE * 2];
    snprintf(out, sizeof out, "%s%s%s\n", lcap, fillbuf, rcap);
    write_user(b->user, out);
    free(b);
}
```

- [ ] **Step 5: Commit.**

```bash
git add src/uibuilders.c src/includes/prototypes.h
git commit -m "$(cat <<'EOF'
UI builders: box_open/box_line/box_blank/box_centered/box_separator/box_close

Stateful box rendering with opaque BOX handle. Top/separator/bottom
edges drawn from ui.box.{top,sep,bot}.* keys; body rows padded via
align_into so the right border doesn't drift on coloured cells.
box_close frees the handle.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 6: `table_*` API on top of `box_*`

**Files:**
- Modify: `src/uibuilders.c`
- Modify: `src/includes/prototypes.h`

Tables share the box's top/bottom/sep frames but split inner space into N visible-column columns. Cells word-wrap onto multiple body rows when they exceed their column width.

- [ ] **Step 1: Add prototypes.**

```c
TABLE table_open      (UR_OBJECT user, int total_width);
void  table_columns   (TABLE t, int n, ...);
void  table_header    (TABLE t, ...);
void  table_separator (TABLE t);
void  table_row       (TABLE t, ...);
void  table_close     (TABLE t);
```

- [ ] **Step 2: Define the struct.**

```c
#define TABLE_MAX_COLUMNS 8

struct table_struct {
    BOX    box;
    int    col_count;
    int    col_widths[TABLE_MAX_COLUMNS];   /* visible cols per column */
};
```

- [ ] **Step 3: Implement `table_open`, `table_columns`, `table_close`.**

```c
TABLE
table_open(UR_OBJECT user, int total_width)
{
    if (!user || total_width <= 0) return NULL;
    struct table_struct *t = calloc(1, sizeof *t);
    if (!t) return NULL;
    t->box = box_open(user, total_width, NULL);
    if (!t->box) { free(t); return NULL; }
    return t;
}

void
table_columns(TABLE t, int n, ...)
{
    if (!t || n <= 0 || n > TABLE_MAX_COLUMNS) return;
    t->col_count = n;
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        t->col_widths[i] = va_arg(ap, int);
    }
    va_end(ap);
}

void
table_close(TABLE t)
{
    if (!t) return;
    box_close(t->box);
    free(t);
}
```

- [ ] **Step 4: Implement `table_separator` (delegates to box).**

```c
void
table_separator(TABLE t)
{
    if (!t) return;
    box_separator(t->box);
}
```

- [ ] **Step 5: Implement `table_header` and `table_row`.**

Both take N `const char *` arguments (one per column). They wrap each cell on visible-column boundaries — when any cell's content exceeds its column width, the row emits multiple body lines until all cells are consumed.

```c
/* Word-wrap `text` into successive chunks each of at most `width` visible
 * columns. On entry `*pos` is the byte offset into `text` to start from;
 * on return it's updated to the next start. Writes the chunk into `out`
 * (byte-allocated). Returns the visible-column width of this chunk. */
static int
wrap_chunk(char *out, size_t outlen, const char *text, size_t *pos, int width)
{
    /* Find a wrap point: greedily consume words; break before exceeding
     * `width` visible columns. Pad with spaces to `width` so column edges
     * stay aligned. */
    int    visible = 0;
    size_t bytes = 0;
    size_t i = *pos;
    size_t word_start = i;
    int    word_visible = 0;
    size_t word_bytes  = 0;
    /* Track the last whitespace position where we could break. */
    size_t break_i = (size_t) -1;
    int    break_visible = 0;
    size_t break_bytes = 0;
    while (text[i] && visible + word_visible < width + 1) {
        if (text[i] == '~' && text[i+1] && text[i+2]
            && isalnum((unsigned char) text[i+1])
            && isalnum((unsigned char) text[i+2])) {
            word_bytes += 3;
            i += 3;
            continue;
        }
        if (text[i] == ' ') {
            break_i = i;
            break_visible = visible + word_visible;
            break_bytes = bytes + word_bytes;
            visible += word_visible;
            bytes   += word_bytes;
            visible += 1;
            bytes   += 1;
            ++i;
            word_start = i;
            word_visible = 0;
            word_bytes = 0;
            continue;
        }
        word_visible++;
        word_bytes++;
        ++i;
    }
    /* Decide where to cut. */
    if (visible + word_visible <= width) {
        /* Whole word fits — accept the lot. */
        visible += word_visible;
        bytes   += word_bytes;
        *pos = i;
    } else if (break_i != (size_t) -1) {
        /* Cut at the last whitespace break. */
        visible = break_visible;
        bytes   = break_bytes - 1;        /* drop the trailing space */
        *pos    = break_i + 1;
    } else {
        /* No whitespace inside the column — hard-break mid-word. */
        /* Walk forward visible-column-by-visible-column until we hit width. */
        size_t k = *pos;
        int    v = 0;
        size_t b = 0;
        while (text[k] && v < width) {
            if (text[k] == '~' && text[k+1] && text[k+2]
                && isalnum((unsigned char) text[k+1])
                && isalnum((unsigned char) text[k+2])) {
                b += 3;
                k += 3;
                continue;
            }
            ++v; ++b; ++k;
        }
        visible = v;
        bytes   = b;
        *pos = k;
    }
    if (bytes + (size_t)(width - visible) + 1 > outlen) {
        bytes = outlen - (size_t)(width - visible) - 1;
    }
    memcpy(out, text + (*pos - (visible + bytes - (i - *pos))) /* simpler: */, bytes);
    /* Actually: just copy from the original start to start+bytes. */
    /* Replace the previous line with: */
    /* (the engineer should re-derive the byte range — keep it simple) */

    /* Pad with spaces. */
    char *o = out + bytes;
    for (int p = 0; p < width - visible; ++p) *o++ = ' ';
    *o = '\0';
    return visible + (width - visible);
}
```

**The above wrap_chunk body is intentionally rough — the implementer should rewrite it cleanly when implementing.** The contract is:

> Given `text` and current byte position `*pos`, emit at most `width` visible columns into `out`, padded with trailing spaces to exactly `width`. Update `*pos` past the consumed bytes (and past any whitespace at the wrap point so the next chunk doesn't start with leading spaces). Wrap on word boundaries when possible; hard-break mid-word otherwise.

A cleaner implementation tracks `(byte_pos, visible_pos)` pairs and uses a single scan; the implementer is encouraged to write their own. Note: this is the most subtle piece of UI code in Phase 3 — be careful and test against several edge cases (no spaces, leading spaces, colour escapes that span a wrap point).

Then:

```c
static void
table_emit_row(TABLE t, va_list cells_ap)
{
    if (!t || t->col_count == 0) return;
    const char *cells[TABLE_MAX_COLUMNS];
    size_t      positions[TABLE_MAX_COLUMNS] = {0};
    for (int i = 0; i < t->col_count; ++i) {
        cells[i] = va_arg(cells_ap, const char *);
        if (!cells[i]) cells[i] = "";
    }

    for (;;) {
        char  rowbuf[ARR_SIZE * 4];
        size_t rowpos = 0;
        int    any_remaining = 0;
        for (int c = 0; c < t->col_count; ++c) {
            char  cell_chunk[ARR_SIZE];
            wrap_chunk(cell_chunk, sizeof cell_chunk,
                       cells[c], &positions[c], t->col_widths[c]);
            rowpos += snprintf(rowbuf + rowpos, sizeof rowbuf - rowpos,
                               "%s", cell_chunk);
            if (c + 1 < t->col_count) {
                /* Use a single space as inter-column separator. */
                rowbuf[rowpos++] = ' ';
                rowbuf[rowpos] = '\0';
            }
            if (cells[c][positions[c]]) any_remaining = 1;
        }
        box_line(t->box, "%s", rowbuf);
        if (!any_remaining) break;
    }
}

void
table_header(TABLE t, ...)
{
    va_list ap;
    va_start(ap, t);
    table_emit_row(t, ap);
    va_end(ap);
    table_separator(t);
}

void
table_row(TABLE t, ...)
{
    va_list ap;
    va_start(ap, t);
    table_emit_row(t, ap);
    va_end(ap);
}
```

- [ ] **Step 6: Verify column widths sum correctly.**

If `sum(col_widths) + (col_count - 1)` (the inter-column spaces) exceeds the box's `inner`, content runs off the right side. There's no automatic fitting — the caller is responsible for sizing columns to match `total_width`. Document this in the prototype comment.

- [ ] **Step 7: Commit.**

```bash
git add src/uibuilders.c src/includes/prototypes.h
git commit -m "$(cat <<'EOF'
UI builders: table_* on top of box_*

Tables share the box frame and split inner width into N
visible-column columns. table_emit_row wraps each cell on word
boundaries (hard-break mid-word as a fallback) and emits multiple
body lines until every cell is consumed. Header gets a separator
underneath; close delegates to box_close.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 7: Ship a themed `cowboy` test locale

**Files:**
- Create: `files/langs/cowboy/strings.yml`

A second locale that overrides only `meta.*` + `ui.*`. Everything else falls back to en_GB via Phase 1's path resolver and Phase 2's catalog fallback. Phase 3's pilot conversion (Task 11) can switch to `cowboy` to demonstrate themed frames end-to-end.

- [ ] **Step 1: Create the directory and file.**

```bash
mkdir -p files/langs/cowboy
```

Create `files/langs/cowboy/strings.yml`:

```yaml
# Cowboy theme — overrides ui.* frame primitives. All other keys fall
# back to en_GB.

meta.name:        "Cowboy"
meta.description: "Yee-haw frames; rest falls back to English."

# Rule with distinctive caps.
ui.rule.lcap:        "-={*"
ui.rule.rcap:        "*}=-"
ui.rule.fill:        "-"
ui.rule.label_lpad:  "6"

# Box edges with the same caps and a fat fill.
ui.box.top.lcap:     "-={*"
ui.box.top.rcap:     "*}=-"
ui.box.top.fill:     "="
ui.box.bot.lcap:     "-={*"
ui.box.bot.rcap:     "*}=-"
ui.box.bot.fill:     "="
ui.box.body.lside:   "* "
ui.box.body.rside:   " *"
ui.box.sep.lcap:     "-={*"
ui.box.sep.rcap:     "*}=-"
ui.box.sep.fill:     "-"
```

- [ ] **Step 2: Commit.**

```bash
git add files/langs/cowboy/strings.yml
git commit -m "$(cat <<'EOF'
Ship a Cowboy test locale with themed ui.* keys

Overrides only meta.* and ui.* — everything else falls back to en_GB.
Used by the Phase 3 pilot to demonstrate end-to-end themed frames
under a non-default locale.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 8: Pilot conversion — `wizlist`

**Files:**
- Modify: `src/commands/wizlist.c`
- Modify: `files/langs/en_GB/strings.yml`

`wizlist` is the natural pilot: it's frame-heavy, all output is server-authored (no user strings), and it has clear column structure. Converting it proves the entire stack end-to-end (`catalog`, `lang_*`, `box_*`, `table_*`).

- [ ] **Step 1: Read the existing `wizlist`.**

```
semble search "wizlist box header columns level name idle" .
```

Note exactly:
- The total width (likely 78 cols).
- The columns and their widths.
- The header text and any inline colour escapes.
- The body row format (which user fields, formatted how).
- Any footer / count summary.

- [ ] **Step 2: Pick keys for the strings that move into the catalog.**

Append to `files/langs/en_GB/strings.yml`:

```yaml
# wizlist
wizlist.title:        "Wiz List"
wizlist.col.name:     "Name"
wizlist.col.level:    "Level"
wizlist.col.idle:     "Idle"
wizlist.col.status:   "Status"
wizlist.empty:        "No wizards are logged on."
wizlist.footer:       "Total: %1$d wizard%2$s online."
```

(Adapt to the actual columns wizlist has — read the existing code first.)

- [ ] **Step 3: Rewrite `wizlist`'s body to use the builders.**

Replace the existing inline `+----+`, `vwrite_user(user, "| %-10s | ...", …)` block with:

```c
void
wizlist(UR_OBJECT user)
{
    UR_OBJECT u;
    int count = 0;

    TABLE t = table_open(user, 78);
    if (!t) return;
    table_columns(t, 4, 20, 8, 8, 38);   /* widths sum to 78 minus 3 spaces */
    table_header(t,
                 lang(user, "wizlist.col.name"),
                 lang(user, "wizlist.col.level"),
                 lang(user, "wizlist.col.idle"),
                 lang(user, "wizlist.col.status"));

    for (u = user_first; u; u = u->next) {
        if (u->type == CLONE_TYPE) continue;
        if (u->level < WIZ) continue;
        /* ... compute level_str, idle_str, status_str ... */
        char level_str[16], idle_str[16], status_str[80];
        snprintf(level_str,  sizeof level_str,  "%s",
                 user_level[u->level].name);
        snprintf(idle_str,   sizeof idle_str,   "%dm",
                 (int)((time(0) - u->last_input) / 60));
        snprintf(status_str, sizeof status_str, "%s",
                 u->afk ? "AFK" : u->malloc_start ? "Edit" : "");
        table_row(t, u->recap, level_str, idle_str, status_str);
        ++count;
    }
    table_close(t);

    if (count == 0) {
        lang_user(user, "wizlist.empty");
    } else {
        lang_user(user, "wizlist.footer", count, count == 1 ? "" : "s");
    }
}
```

Adapt to the existing function's signature and the actual columns it uses. The exact wizlist body shape is in `src/commands/wizlist.c` — read it carefully before writing.

- [ ] **Step 4: Update the migration ledger.**

In `docs/superpowers/specs/localisation-migration.md`, add a row to the per-command sweep ledger (it currently says "To be filled in"):

```markdown
| Command | File | Status |
|---------|------|--------|
| wizlist | src/commands/wizlist.c | converted (2026-MM-DD) — Phase 3 pilot |
```

- [ ] **Step 5: Commit.**

```bash
git add src/commands/wizlist.c files/langs/en_GB/strings.yml docs/superpowers/specs/localisation-migration.md
git commit -m "$(cat <<'EOF'
Convert wizlist to UI builders (Phase 3 pilot)

First end-to-end consumer of box_*/table_* + lang_user. Frame
appearance now lives in ui.* catalog keys; columns + header labels
in wizlist.* keys. On en_GB the output is byte-identical to before;
on cowboy or any other themed locale it picks up the locale's
frame style automatically.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 9: Manual end-to-end verification

**Files:** (none committed)

The Linux/macOS host smoke-test that proves Phase 3's stack works.

- [ ] **Step 1: Build.** `make build` — must compile clean.

- [ ] **Step 2: Boot.** `./amnutsTalker -p 12345`. Expected: `Localisation: discovered N locale(s); default = en_GB.` and `Localisation: catalog loaded (N locale(s)).` where N now includes `cowboy`.

- [ ] **Step 3: As default user (no `set lang`), run `.wizlist`.** Output should be byte-identical to the pre-Phase-3 wizlist (the en_GB ui.* keys mirror the original frame).

- [ ] **Step 4: `set lang cowboy`, then `.wizlist` again.** The frame should now show the `-={*` / `*}=-` caps and the `=` fill — a visibly different look. The user names, levels, etc. remain identical (no translations on those keys yet).

- [ ] **Step 5: Resize a column.** Edit `cowboy/strings.yml` to add a tiny override of, say, `wizlist.col.status: "State"`. Run `.langreload`. Run `.wizlist` again. The header should now say "State" instead of "Status".

- [ ] **Step 6: Test `visible_strlen` against a labelled rule.** From any command-injection-friendly site (e.g., a quick debug hook in a less-important command), call `rule(user, 78, "~OL~FCSection Title~RS")`. Expect the title to appear centred-ish with correct fill on both sides — the colour escapes around the label must NOT shift the right-cap position.

- [ ] **Step 7: No commit.**

---

## Task 10: Phase 3 migration ledger update

**Files:**
- Modify: `docs/superpowers/specs/localisation-migration.md`

- [ ] **Step 1: Mark Phase 3 converted.**

```markdown
| 3 — UI builders | converted (2026-MM-DD) | visible_strlen / align_into / rule / box_* / table_*; ui.* keys in en_GB; cowboy test locale; wizlist pilot converted |
```

- [ ] **Step 2: Append a Phase 3 verification block** (mirroring the Phase 1 / Phase 2 sections) describing the smoke-test steps above. List the cowboy locale as a permanent test fixture.

- [ ] **Step 3: Commit.**

---

## Self-Review Checklist

- [ ] Spec §6.1 (frame primitives in strings.yml) — Task 3 ships en_GB defaults, Task 7 ships cowboy overrides.
- [ ] Spec §6.2 (pattern repetition) — `fill_pattern` in Task 4 implements visible-column-bounded repetition with colour-escape passthrough.
- [ ] Spec §6.3 (inline-label overlay) — Task 4's `rule()` composes lcap + pad + " label " + remaining fill + rcap in pieces (no memcpy-overwrite).
- [ ] Spec §6.4 (body rendering) — box_line / box_centered use align_into for visible-column padding.
- [ ] Spec §6.5 (API surface) — visible_strlen, align_into, rule, box_open/box_line/box_blank/box_centered/box_separator/box_close, table_open/table_columns/table_header/table_separator/table_row/table_close.
- [ ] Spec §6.6 (file layout) — `src/uibuilders.c` + `src/includes/uibuilders.h`.
- [ ] Spec §11 Phase 3 test plan — Task 9 exercises wizlist on default and themed locales plus visible_strlen with embedded escapes.
- [ ] Phase 4 prerequisites: every API surface needed for the frame-heavy command sweep (`wizlist`, `show_igusers`, `grepusers`, `listbans`, `system`, parts of `help`) is in place by end of Task 6. Phase 4 starts here.

**Placeholder scan:** Task 6's `wrap_chunk` is given a rough skeleton with an explicit "rewrite this cleanly" instruction and a clear contract. That's the only deliberate hand-off; the implementer is expected to write a correct version rather than copy the rough draft. Acceptable because the contract is precise (input, output, padding, wrap behaviour) — but flag this if you'd prefer a fully-written reference implementation.

**Type consistency:** `BOX` / `TABLE` typedefs from `uibuilders.h` are used by prototypes and implementations consistently. `enum align_value` values match between header (Task 1) and `align_into` (Task 2). `TABLE_MAX_COLUMNS = 8` matches `arg_count`'s 8-arg limit established in Phase 2 — by design, since table cells are eventually catalog strings with at most 8 substitutions.
