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
           int align, int width,
           const char *fmt, ...)
{
    char rendered[ARR_SIZE * 2];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(rendered, sizeof rendered, fmt, ap);
    va_end(ap);

    int visible = visible_strlen(rendered);

    /* Truncation: walk byte by byte, advance a visible-column counter, stop
     * emitting bytes once we'd cross `width`. Don't bisect a colour escape. */
    char trimmed[ARR_SIZE * 2];
    int  trim_visible = 0;
    size_t trim_bytes = 0;
    if (visible <= width) {
        size_t n = strlen(rendered);
        if (n + 1 > sizeof trimmed) n = sizeof trimmed - 1;
        memcpy(trimmed, rendered, n);
        trimmed[n] = '\0';
        trim_visible = visible;
        trim_bytes   = n;
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
    if (pad_total < 0) pad_total = 0;
    int pad_left  = 0, pad_right = 0;
    switch (align) {
    case ALIGN_LEFT:   pad_right = pad_total; break;
    case ALIGN_RIGHT:  pad_left  = pad_total; break;
    case ALIGN_CENTRE: pad_left  = pad_total / 2;
                       pad_right = pad_total - pad_left; break;
    }

    if (trim_bytes + (size_t) pad_total + 1 > outlen) {
        if (outlen) out[0] = '\0';
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
    pos += snprintf(out + pos, sizeof out - pos, "%s", lcap);
    if (label_visible > 0) {
        int used_visible = 0;
        int lp = label_lpad;
        if (lp > inner) lp = inner;
        pos += fill_pattern(out + pos, sizeof out - pos, fill, lp);
        used_visible += lp;
        if (used_visible + 1 <= inner) {
            out[pos++] = ' '; ++used_visible;
        }
        if (used_visible + label_visible <= inner) {
            size_t lblen = strlen(rendered_label);
            if (pos + lblen < sizeof out) {
                memcpy(out + pos, rendered_label, lblen);
                pos += lblen;
                used_visible += label_visible;
            }
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
    pos += snprintf(out + pos, sizeof out - pos, "%s\n", rcap);

    write_user(user, out);
}

struct box_struct {
    UR_OBJECT user;
    int       width;        /* total visible cols including sides */
    int       inner;        /* width - visible_strlen(lside) - visible_strlen(rside) */
    char      lside[16];
    char      rside[16];
};

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
        int lpad = 5;
        pos += fill_pattern(out + pos, sizeof out - pos, fill, lpad);
        out[pos++] = ' ';
        size_t tlen = strlen(rendered_title);
        if (pos + tlen < sizeof out) {
            memcpy(out + pos, rendered_title, tlen);
            pos += tlen;
        }
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

#define TABLE_MAX_COLUMNS 8

struct table_struct {
    BOX    box;
    int    col_count;
    int    col_widths[TABLE_MAX_COLUMNS];   /* visible cols per column */
};

/*
 * Consume up to `width` visible columns from text starting at *pos.
 * Word-wrap at whitespace when possible; hard-break inside a word
 * otherwise. Append spaces so the output reaches exactly `width`
 * visible columns. Update *pos past the bytes emitted, and past any
 * trailing whitespace at the wrap point (so the next chunk does not
 * begin with leading spaces left over from the wrap).
 *
 * Colour escapes (~XX) count as zero visible columns and are never
 * bisected.
 *
 * Leading whitespace at *pos is NOT skipped: it is part of the cell's
 * content. Whitespace is only "swallowed" at a wrap boundary inside
 * the cell.
 *
 * Returns the visible width emitted (which equals `width` whenever
 * `out` is large enough).
 */
static int
wrap_chunk(char *out, size_t outlen, const char *text, size_t *pos, int width)
{
    if (!out || outlen == 0) return 0;
    out[0] = '\0';
    if (width <= 0 || !text || !pos) return 0;

    size_t start    = *pos;
    size_t i        = start;          /* current byte index into text */
    int    visible  = 0;              /* visible cols consumed */

    /* Last whitespace seen *after* some non-space content. Validity
     * is tracked by `have_ws` (have_ws == 0 means the byte/visible
     * fields are unset). */
    size_t last_ws_byte    = 0;
    int    last_ws_visible = 0;
    int    have_ws         = 0;
    int    seen_nonspace   = 0;

    while (text[i] != '\0' && visible < width) {
        /* Colour escape: consume 3 bytes, 0 visible columns. */
        if (text[i] == '~' && text[i + 1] && text[i + 2]
            && isalnum((unsigned char) text[i + 1])
            && isalnum((unsigned char) text[i + 2])) {
            i += 3;
            continue;
        }
        if (text[i] == ' ') {
            if (seen_nonspace) {
                last_ws_byte    = i;
                last_ws_visible = visible;
                have_ws         = 1;
            }
        } else {
            seen_nonspace = 1;
        }
        ++i;
        ++visible;
    }

    size_t emit_end = i;          /* one past last byte to emit */
    int    emit_visible = visible;
    size_t advance_to   = i;      /* where to leave *pos */

    if (visible >= width && text[i] != '\0' && text[i] != ' ' && have_ws) {
        /* Column filled mid-word and we have a recorded wrap point:
         * rewind to it and skip the run of whitespace at that point. */
        emit_end     = last_ws_byte;
        emit_visible = last_ws_visible;
        advance_to   = last_ws_byte;
        while (text[advance_to] == ' ') ++advance_to;
    } else if (visible >= width && text[i] == ' ') {
        /* Column filled exactly at a word boundary: emit what we have
         * and step past the run of trailing whitespace so the next
         * chunk does not start with leftover spaces. */
        advance_to = i;
        while (text[advance_to] == ' ') ++advance_to;
    }
    /* Else: end-of-input, or hard-break (no whitespace recorded), or
     * column not yet full — emit_end already correct, advance_to == i. */

    /* Copy bytes [start, emit_end) into out, then pad with spaces to
     * reach `width` visible columns. Bail out cleanly if outlen is too
     * tight to hold the result. */
    size_t span = emit_end - start;
    int    pad  = width - emit_visible;
    if (pad < 0) pad = 0;

    if (span + (size_t) pad + 1 > outlen) {
        /* Not enough room — emit nothing rather than half. Caller's
         * buffer is sized via ARR_SIZE; this branch is defensive. */
        out[0] = '\0';
        *pos = advance_to;
        return 0;
    }

    memcpy(out, text + start, span);
    for (int k = 0; k < pad; ++k) {
        out[span + k] = ' ';
    }
    out[span + pad] = '\0';

    *pos = advance_to;
    return emit_visible + pad;
}

TABLE
table_open(UR_OBJECT user, int total_width)
{
    if (!user || total_width <= 0) return NULL;
    struct table_struct *t = calloc(1, sizeof *t);
    if (!t) return NULL;
    t->box = box_open(user, total_width, NULL);
    if (!t->box) {
        free(t);
        return NULL;
    }
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
table_separator(TABLE t)
{
    if (!t) return;
    box_separator(t->box);
}

void
table_close(TABLE t)
{
    if (!t) return;
    box_close(t->box);
    free(t);
}

/*
 * Emit one logical row's worth of body lines. Each cell is wrapped
 * independently into its column width; we keep emitting lines until
 * every cell has been fully consumed. Cells that finish early just
 * contribute padded blanks on subsequent lines so the column edges
 * stay aligned.
 */
static void
table_emit_row(TABLE t, va_list cells_ap)
{
    if (!t || t->col_count == 0) return;

    const char *cells[TABLE_MAX_COLUMNS];
    size_t      positions[TABLE_MAX_COLUMNS];
    for (int i = 0; i < t->col_count; ++i) {
        cells[i] = va_arg(cells_ap, const char *);
        if (!cells[i]) cells[i] = "";
        positions[i] = 0;
    }

    /* First iteration always emits (handles the empty-cells case
     * sensibly). Subsequent iterations only run if at least one cell
     * had bytes remaining after the previous line was emitted. */
    int first = 1;
    for (;;) {
        int any_remaining_before = 0;
        for (int c = 0; c < t->col_count; ++c) {
            if (cells[c][positions[c]] != '\0') {
                any_remaining_before = 1;
                break;
            }
        }
        if (!first && !any_remaining_before) break;

        char rowbuf[ARR_SIZE * 4];
        size_t rowpos = 0;
        for (int c = 0; c < t->col_count; ++c) {
            char cell_chunk[ARR_SIZE];
            wrap_chunk(cell_chunk, sizeof cell_chunk,
                       cells[c], &positions[c], t->col_widths[c]);
            size_t clen = strlen(cell_chunk);
            if (rowpos + clen + 2 < sizeof rowbuf) {
                memcpy(rowbuf + rowpos, cell_chunk, clen);
                rowpos += clen;
                if (c + 1 < t->col_count) {
                    rowbuf[rowpos++] = ' ';
                }
            }
        }
        rowbuf[rowpos] = '\0';
        box_line(t->box, "%s", rowbuf);
        first = 0;
    }
}

void
table_header(TABLE t, ...)
{
    if (!t) return;
    va_list ap;
    va_start(ap, t);
    table_emit_row(t, ap);
    va_end(ap);
    table_separator(t);
}

void
table_row(TABLE t, ...)
{
    if (!t) return;
    va_list ap;
    va_start(ap, t);
    table_emit_row(t, ap);
    va_end(ap);
}
