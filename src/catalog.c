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
