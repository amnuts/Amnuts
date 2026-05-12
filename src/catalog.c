/****************************************************************************
   Amnuts — string catalog (Mechanism 1 of the localisation design).
   See docs/superpowers/specs/2026-05-10-localisation-design.md §4.
 ***************************************************************************/

#include <ctype.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

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

static void
catalog_log_drop(const char *locale_name, const char *key,
                 const char *path, int line, int col, const char *why)
{
    /* write_syslog uses vsprintf into a bounded global buffer, so truncate
     * the YAML-supplied key into a fixed stack buffer before formatting. */
    char kbuf[64];
    if (key) {
        snprintf(kbuf, sizeof kbuf, "%s", key);
    } else {
        kbuf[0] = '\0';
    }
    write_syslog(SYSLOG | ERRLOG, 0,
                 "[locale] %s/strings.yml: key '%s' dropped at %d:%d — %s (path %s)\n",
                 locale_name, key ? kbuf : "<top-level>", line, col, why, path);
}

/*
 * Load one locale's strings.yml into `cat`. Always sets cat->loaded_ok = true
 * on completion — including the missing-file and empty-file paths (the
 * catalog stays empty and lookups fall back). Top-level parse failure
 * (malformed YAML, wrong shape) is fatal via yaml_die → boot_exit.
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

        /* value — only scalars supported in Phase 2. */
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
        /* On OOM we call boot_exit; the OS reclaims any partial alloc. */
        if (!e->key || !e->fmt) {
            fprintf(stderr, "Amnuts: out of memory loading catalog %s.\n",
                    cat->name);
            boot_exit(1);
        }
        catalog_insert(cat, e);

        yaml_event_delete(&key_ev);
        yaml_event_delete(&val_ev);
    }

    /* Drain to stream-end. yaml_event_delete() memsets the event to zero,
     * so we must capture the type before deleting. */
    for (;;) {
        yaml_next(path, &parser, &event);
        yaml_event_type_t t = event.type;
        yaml_event_delete(&event);
        if (t == YAML_STREAM_END_EVENT) break;
    }

done_ok:
    yaml_parser_delete(&parser);
    fclose(fp);
    cat->loaded_ok = true;
}

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
        fprintf(stderr, "Amnuts: default locale '%s' missing from catalog table.\n",
                amsys->default_locale);
        boot_exit(1);
    }

    /* Load default first so non-defaults can validate against it. */
    catalog_load_one(&st->catalogs[st->default_index], NULL);

    /* catalog_load_one always sets loaded_ok = true on non-fatal completion
     * (fatal parse failures call boot_exit), so an empty catalog is the only
     * remaining signal that the default's strings.yml is missing or empty. */
    if (st->catalogs[st->default_index].entry_count == 0) {
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

/*
 * Per-locale equivalent of vwrite_level (src/amnuts.c). Renders `key`
 * with each recipient's own catalog before delivery; otherwise faithfully
 * mirrors write_level's behaviour so Phase 5 migrations from
 *   vwrite_level(lvl, above, dorecord, sender, fmt, ...)
 * to
 *   lang_level(lvl, above, dorecord, sender, key, ...)
 * are observationally equivalent.
 *
 * Parameter names are historical: the signature was frozen in Phase 2
 * with second/third params spelled `notify_invis` / `record_flag`, but
 * they map 1:1 onto vwrite_level's `above` / `dorecord` arguments.
 *   - notify_invis == 1 (above=true)  → recipients with level >= min_level
 *   - notify_invis == 0 (above=false) → recipients with level <= min_level
 *   - record_flag (RECORD/NORECORD) → write_user diversions to
 *     record_afk/record_edit, plus a record_tell on successful delivery.
 *
 * `exclude` plays the dual role vwrite_level's `user` parameter does: it is
 * both the "sender" (used for ignore-list/level checks and as the `from`
 * for record_* entries) and the recipient to skip.
 */
void
lang_level(int min_level, int notify_invis, int record_flag,
           UR_OBJECT exclude, const char *key, ...)
{
    va_list ap0;
    va_start(ap0, key);

    for (UR_OBJECT u = user_first; u; u = u->next) {
        /* Recipient on the sender's ignore list — skip unless sender is GOD.
         * check_igusers tolerates a NULL second arg (returns 0), so the
         * short-circuit also handles the exclude == NULL case. */
        if (check_igusers(u, exclude) && exclude && exclude->level < GOD) {
            continue;
        }
        /* Per-recipient ignore toggles for wiz-channel chatter / logons. */
        if ((u->ignwiz && (com_num == WIZSHOUT || com_num == WIZEMOTE))
                || (u->ignlogons && logon_flag)) {
            continue;
        }
        if (u == exclude) continue;
        if (u->login) continue;
        if (u->type == CLONE_TYPE) continue;

        /* Direction: notify_invis carries vwrite_level's `above` flag. */
        if (notify_invis) {
            if (u->level < (enum lvl_value) min_level) continue;
        } else {
            if (u->level > (enum lvl_value) min_level) continue;
        }

#ifdef NETLINKS
        if (!u->socket) continue;
#endif

        /* Render in the recipient's locale. */
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

        /* AFK and line-editor recipients get the text diverted to their
         * review buffer (when record_flag is set) instead of seeing it
         * inline. Mirrors write_level exactly. */
        if (u->afk) {
            if (record_flag) {
                record_afk(exclude, u, buf);
            }
            continue;
        }
        if (u->malloc_start) {
            if (record_flag) {
                record_edit(exclude, u, buf);
            }
            continue;
        }
        if (!u->ignall) {
            write_user(u, buf);
        }
        if (record_flag) {
            record_tell(exclude, u, buf);
        }
    }
    va_end(ap0);
}

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
    /* If the caller named the server default explicitly, treat it as a
     * clear-override so the listing shows '*' (default) rather than '>'
     * shadowing it — the user wanted the default, not a pinned override
     * that happens to match the default's current value. */
    if (cat->is_default) {
        user->locale[0] = '\0';
        locale_resolve_catalog(user);
        save_user_details(user, 1);
        return 1;
    }
    strncpy(user->locale, cat->name, LOCALE_NAME_LEN - 1);
    user->locale[LOCALE_NAME_LEN - 1] = '\0';
    user->catalog = cat;
    save_user_details(user, 1);
    return 1;
}


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
        size_t dlen = strlen(display_desc);
        if (dlen == 0 || display_desc[dlen - 1] != '\n') {
            write_user(user, "\n");
        }
    }
    write_user(user, "\n~OL*~RS = server default, ~OL>~RS = your current setting\n\n");
}


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
    struct locale_state *fresh = calloc(1, sizeof *fresh);
    if (!fresh) return -1;

    /* Discover locales into `fresh` — duplicates the small loop from
     * locale_load_all() (in src/locale.c). Refactor opportunity if a
     * third caller appears. */
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
        /* Out of memory holding the grace generation — free in place. */
        catalog_free_all(&old_state);
    }

    /* Re-resolve every connected user's catalog pointer. */
    int resets = 0;
    for (UR_OBJECT u = user_first; u; u = u->next) {
        if (u->type == CLONE_TYPE || u->type == REMOTE_TYPE) continue;
        int had_locale = (u->locale[0] != '\0');
        locale_resolve_catalog(u);
        if (had_locale && !u->locale[0]) {
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
