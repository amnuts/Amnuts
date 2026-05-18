/*
 * yaml_help.c
 *
 * YAML loader for the talker's help text file.
 *
 * Parses files/datafiles/help.yaml using libyaml's event-stream API
 * and builds a sorted in-memory lookup table (help_table) keyed by
 * command name. help_lookup() does a binary search over this table.
 *
 * Each entry can have an optional `usage` (scalar OR sequence of
 * strings), an optional `aliases` (sequence), and an optional
 * `description` (multi-line scalar).
 */

#include "defines.h"
#include "globals.h"
#include "prototypes.h"
#include "yaml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *
scalar_str(yaml_event_t *ev)
{
    return (char *)ev->data.scalar.value;
}

/*
 * Read a YAML sequence of scalar strings, returning a heap array of
 * strdup'd strings. *out_count receives the count. The caller has
 * already consumed the SEQUENCE_START event; we consume up to and
 * including SEQUENCE_END.
 */
static char **
read_string_seq(const char *path, yaml_parser_t *p, int *out_count)
{
    char **out = NULL;
    int cap = 0;
    int n = 0;

    for (;;) {
        yaml_event_t ev;
        yaml_next(path, p, &ev);

        if (ev.type == YAML_SEQUENCE_END_EVENT) {
            yaml_event_delete(&ev);
            break;
        }
        if (ev.type != YAML_SCALAR_EVENT) {
            yaml_die(path, p, "expected scalar in sequence");
        }
        if (n >= cap) {
            cap = cap ? cap * 2 : 4;
            out = realloc(out, cap * sizeof *out);
            if (!out) {
                yaml_die(path, p, "out of memory reading sequence");
            }
        }
        out[n++] = strdup(scalar_str(&ev));
        yaml_event_delete(&ev);
    }
    *out_count = n;
    return out;
}

/*
 * Parse one help entry's body mapping. The MAPPING_START for the body
 * has already been consumed by the caller. Populates the fields of `e`.
 */
static void
load_one_entry(const char *path, yaml_parser_t *p, struct help_entry *e)
{
    for (;;) {
        yaml_event_t key;
        yaml_next(path, p, &key);

        if (key.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key);
            break;
        }
        if (key.type != YAML_SCALAR_EVENT) {
            yaml_die(path, p,
                     "expected scalar key in help entry '%s'", e->command);
        }

        char k[64];
        snprintf(k, sizeof k, "%s", scalar_str(&key));
        yaml_event_delete(&key);

        if (!strcmp(k, "usage")) {
            yaml_event_t v;
            yaml_next(path, p, &v);
            if (v.type == YAML_SCALAR_EVENT) {
                e->usage = malloc(sizeof *e->usage);
                if (!e->usage) {
                    yaml_die(path, p,
                             "out of memory for usage in '%s'", e->command);
                }
                e->usage[0] = strdup(scalar_str(&v));
                e->usage_count = 1;
                yaml_event_delete(&v);
            } else if (v.type == YAML_SEQUENCE_START_EVENT) {
                yaml_event_delete(&v);
                e->usage = read_string_seq(path, p, &e->usage_count);
            } else {
                yaml_die(path, p,
                         "expected scalar or sequence for 'usage' in '%s'",
                         e->command);
            }
        } else if (!strcmp(k, "aliases")) {
            yaml_event_t v = yaml_expect(path, p, YAML_SEQUENCE_START_EVENT,
                                         "aliases sequence");
            yaml_event_delete(&v);
            e->aliases = read_string_seq(path, p, &e->alias_count);
        } else if (!strcmp(k, "description")) {
            yaml_event_t v = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                         "description value");
            e->description = strdup(scalar_str(&v));
            yaml_event_delete(&v);
        } else {
            yaml_die(path, p,
                     "unknown help entry key '%s' in '%s'", k, e->command);
        }
    }
}

static int
help_cmp(const void *a, const void *b)
{
    const struct help_entry *ha = a;
    const struct help_entry *hb = b;
    return strcmp(ha->command, hb->command);
}

void
yaml_load_help(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Amnuts: Cannot open help file '%s'\n", path);
        boot_exit(1);
    }

    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        fprintf(stderr,
                "Amnuts: Failed to initialise YAML parser for '%s'\n", path);
        fclose(fp);
        boot_exit(1);
    }
    yaml_parser_set_input_file(&parser, fp);

    yaml_event_t ev;

    ev = yaml_expect(path, &parser, YAML_STREAM_START_EVENT, "stream");
    yaml_event_delete(&ev);
    ev = yaml_expect(path, &parser, YAML_DOCUMENT_START_EVENT, "document");
    yaml_event_delete(&ev);
    ev = yaml_expect(path, &parser, YAML_MAPPING_START_EVENT,
                     "top-level mapping");
    yaml_event_delete(&ev);

    int cap = 0;
    help_table = NULL;
    help_table_count = 0;

    for (;;) {
        yaml_event_t name_ev;
        yaml_next(path, &parser, &name_ev);

        if (name_ev.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&name_ev);
            break;
        }
        if (name_ev.type != YAML_SCALAR_EVENT) {
            yaml_die(path, &parser, "expected command name (scalar)");
        }

        if (help_table_count >= cap) {
            cap = cap ? cap * 2 : 64;
            help_table = realloc(help_table, cap * sizeof *help_table);
            if (!help_table) {
                yaml_die(path, &parser,
                         "out of memory growing help_table");
            }
        }
        struct help_entry *e = &help_table[help_table_count++];
        memset(e, 0, sizeof *e);
        e->command = strdup(scalar_str(&name_ev));
        yaml_event_delete(&name_ev);

        ev = yaml_expect(path, &parser, YAML_MAPPING_START_EVENT,
                         "help entry body");
        yaml_event_delete(&ev);

        load_one_entry(path, &parser, e);
    }

    ev = yaml_expect(path, &parser, YAML_DOCUMENT_END_EVENT, "document end");
    yaml_event_delete(&ev);
    ev = yaml_expect(path, &parser, YAML_STREAM_END_EVENT, "stream end");
    yaml_event_delete(&ev);

    yaml_parser_delete(&parser);
    fclose(fp);

    qsort(help_table, help_table_count, sizeof *help_table, help_cmp);
}

const struct help_entry *
help_lookup(const char *command)
{
    if (!help_table || !help_table_count) {
        return NULL;
    }
    struct help_entry key;
    memset(&key, 0, sizeof key);
    key.command = (char *)command;
    return bsearch(&key, help_table, help_table_count,
                   sizeof *help_table, help_cmp);
}
