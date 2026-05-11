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
