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
           enum align_value align, int width,
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
