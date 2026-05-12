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
