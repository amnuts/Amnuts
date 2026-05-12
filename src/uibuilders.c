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
