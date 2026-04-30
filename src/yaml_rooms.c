/*
 * yaml_rooms.c
 *
 * YAML loader for the talker's rooms file.
 *
 * Parses files/datafiles/rooms.yaml using libyaml's event-stream API
 * and builds the global room linked list. Performs the same validation
 * the legacy parse_rooms_section() in src/amnuts.c performs (length
 * checks, duplicate name/label, link self-reference).
 *
 * Two-pass design: pass 1 creates each room and stashes its link names
 * in a temporary heap-allocated array; pass 2 resolves those names to
 * RM_OBJECT pointers once every room has been created.
 */

#include "defines.h"
#include "globals.h"
#include "prototypes.h"
#include "yaml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Pending-link bookkeeping for pass 2                                 */
/* ------------------------------------------------------------------ */

struct pending_links {
    RM_OBJECT room;
    char **names;
    int count;
};

/* ------------------------------------------------------------------ */
/* Tiny helpers                                                        */
/* ------------------------------------------------------------------ */

static char *
scalar_str(yaml_event_t *ev)
{
    return (char *)ev->data.scalar.value;
}

static int
parse_access(const char *path, yaml_event_t *ev)
{
    const char *v = scalar_str(ev);
    if (!strcmp(v, "BOTH")) {
        return 0;
    }
    if (!strcmp(v, "PUB")) {
        return FIXED;
    }
    if (!strcmp(v, "PRIV")) {
        return FIXED | PRIVATE;
    }
    yaml_die(path, NULL,
             "unknown access '%s' (expected BOTH, PUB or PRIV)", v);
}

/*
 * Read a YAML sequence of scalar strings into a heap array. Caller
 * frees both the array and each entry. Sets *out_count. We expect the
 * caller to already have consumed the SEQUENCE_START event; we consume
 * up to and including SEQUENCE_END.
 */
static char **
read_string_sequence(const char *path, yaml_parser_t *p, int *out_count,
                     int max)
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
        if (n >= max) {
            yaml_die(path, p, "too many entries in sequence (max %d)", max);
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
 * Skip a netlink value when NETLINKS is disabled. v has already been
 * read by the caller. Consumes a trailing MAPPING_END if v was a
 * MAPPING_START.
 */
#ifndef NETLINKS
static void
skip_netlink_value(const char *path, yaml_parser_t *p, yaml_event_t *v)
{
    if (v->type == YAML_SCALAR_EVENT) {
        return;
    }
    if (v->type == YAML_MAPPING_START_EVENT) {
        int depth = 1;
        while (depth > 0) {
            yaml_event_t e;
            yaml_next(path, p, &e);
            if (e.type == YAML_MAPPING_START_EVENT) {
                ++depth;
            } else if (e.type == YAML_MAPPING_END_EVENT) {
                --depth;
            }
            yaml_event_delete(&e);
        }
        return;
    }
    yaml_die(path, p, "expected scalar or mapping for netlink");
}
#endif

/* ------------------------------------------------------------------ */
/* Parse one room body                                                  */
/* ------------------------------------------------------------------ */

/*
 * Parse the body of a single room. The MAPPING_START for the body has
 * already been consumed by the caller. Returns a pending_links record
 * whose `names` array is a heap-allocated list of link names; the
 * caller is responsible for resolving those into pointers and freeing
 * the array (in pass 2).
 */
static struct pending_links
load_one_room(const char *path, yaml_parser_t *p, const char *room_name)
{
    /* Length and duplicate-name checks BEFORE we allocate the room. */
    if (strlen(room_name) > ROOM_NAME_LEN) {
        yaml_die(path, p,
                 "room name '%s' too long (max %d)", room_name, ROOM_NAME_LEN);
    }
    for (RM_OBJECT existing = room_first; existing; existing = existing->next) {
        if (!strcmp(existing->name, room_name)) {
            yaml_die(path, p, "duplicate room name '%s'", room_name);
        }
    }

    RM_OBJECT rm = create_room();
    if (!rm) {
        yaml_die(path, p, "failed to allocate room '%s'", room_name);
    }
    strcpy(rm->name, room_name);
    strcpy(rm->show_name, rm->name);

    struct pending_links pl = { .room = rm, .names = NULL, .count = 0 };
    int saw_links = 0;
    int saw_map = 0;
    int saw_label = 0;
    int saw_desc = 0;

    for (;;) {
        yaml_event_t key;
        yaml_next(path, p, &key);

        if (key.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key);
            break;
        }
        if (key.type != YAML_SCALAR_EVENT) {
            yaml_die(path, p,
                     "expected scalar key in room '%s'", room_name);
        }

        char k[64];
        snprintf(k, sizeof k, "%s", scalar_str(&key));
        yaml_event_delete(&key);

        if (!strcmp(k, "map")) {
            yaml_event_t v = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                         "map value");
            const char *vs = scalar_str(&v);
            if (strlen(vs) > ROOM_NAME_LEN) {
                yaml_die(path, p,
                         "room map name '%s' too long (max %d)",
                         vs, ROOM_NAME_LEN);
            }
            strcpy(rm->map, vs);
            saw_map = 1;
            yaml_event_delete(&v);
        } else if (!strcmp(k, "label")) {
            yaml_event_t v = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                         "label value");
            const char *vs = scalar_str(&v);
            if (strlen(vs) > ROOM_LABEL_LEN) {
                yaml_die(path, p,
                         "room label '%s' too long (max %d)",
                         vs, ROOM_LABEL_LEN);
            }
            for (RM_OBJECT existing = room_first; existing;
                 existing = existing->next) {
                if (existing != rm && !strcmp(existing->label, vs)) {
                    yaml_die(path, p, "duplicate room label '%s'", vs);
                }
            }
            strcpy(rm->label, vs);
            saw_label = 1;
            yaml_event_delete(&v);
        } else if (!strcmp(k, "links")) {
            yaml_event_t v = yaml_expect(path, p, YAML_SEQUENCE_START_EVENT,
                                         "links sequence");
            yaml_event_delete(&v);
            pl.names = read_string_sequence(path, p, &pl.count, MAX_LINKS);
            saw_links = 1;
        } else if (!strcmp(k, "access")) {
            yaml_event_t v = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                         "access value");
            rm->access = parse_access(path, &v);
            yaml_event_delete(&v);
        } else if (!strcmp(k, "topic")) {
            yaml_event_t v = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                         "topic value");
            const char *vs = scalar_str(&v);
            if (strlen(vs) > TOPIC_LEN) {
                yaml_die(path, p,
                         "topic too long for room '%s' (max %d)",
                         room_name, TOPIC_LEN);
            }
            strcpy(rm->topic, vs);
            yaml_event_delete(&v);
        } else if (!strcmp(k, "description")) {
            yaml_event_t v = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                         "description value");
            const char *d = scalar_str(&v);
            size_t dlen = strlen(d);
            if (dlen > ROOM_DESC_LEN) {
                yaml_die(path, p,
                         "description too long for room '%s' (%zu > %d)",
                         room_name, dlen, ROOM_DESC_LEN);
            }
            memcpy(rm->desc, d, dlen);
            rm->desc[dlen] = '\0';
            saw_desc = 1;
            yaml_event_delete(&v);
        } else if (!strcmp(k, "netlink")) {
#ifdef NETLINKS
            yaml_event_t v;
            yaml_next(path, p, &v);
            if (v.type == YAML_SCALAR_EVENT) {
                const char *vs = scalar_str(&v);
                if (!strcmp(vs, "ACCEPT")) {
                    rm->inlink = 1;
                } else {
                    yaml_die(path, p,
                             "netlink scalar must be 'ACCEPT' (got '%s')", vs);
                }
                yaml_event_delete(&v);
            } else if (v.type == YAML_MAPPING_START_EVENT) {
                yaml_event_delete(&v);
                char type_buf[32] = "";
                char name_buf[SERV_NAME_LEN + 1] = "";
                for (;;) {
                    yaml_event_t ik;
                    yaml_next(path, p, &ik);
                    if (ik.type == YAML_MAPPING_END_EVENT) {
                        yaml_event_delete(&ik);
                        break;
                    }
                    if (ik.type != YAML_SCALAR_EVENT) {
                        yaml_die(path, p,
                                 "expected scalar key in netlink mapping"
                                 " for room '%s'", room_name);
                    }
                    char ikey[32];
                    snprintf(ikey, sizeof ikey, "%s", scalar_str(&ik));
                    yaml_event_delete(&ik);

                    yaml_event_t iv = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                                  "netlink mapping value");
                    const char *ivs = scalar_str(&iv);
                    if (!strcmp(ikey, "type")) {
                        snprintf(type_buf, sizeof type_buf, "%s", ivs);
                    } else if (!strcmp(ikey, "name")) {
                        if (strlen(ivs) > SERV_NAME_LEN) {
                            yaml_die(path, p,
                                     "netlink name too long for room '%s'"
                                     " (max %d)", room_name, SERV_NAME_LEN);
                        }
                        snprintf(name_buf, sizeof name_buf, "%s", ivs);
                    } else {
                        yaml_die(path, p,
                                 "unknown netlink key '%s' for room '%s'",
                                 ikey, room_name);
                    }
                    yaml_event_delete(&iv);
                }
                if (strcmp(type_buf, "CONNECT")) {
                    yaml_die(path, p,
                             "netlink mapping for room '%s' must have"
                             " type=CONNECT (got '%s')",
                             room_name, type_buf);
                }
                if (!*name_buf) {
                    yaml_die(path, p,
                             "netlink CONNECT for room '%s' missing 'name'",
                             room_name);
                }
                strcpy(rm->netlink_name, name_buf);
            } else {
                yaml_die(path, p,
                         "expected scalar or mapping for netlink in room '%s'",
                         room_name);
            }
#else
            yaml_event_t v;
            yaml_next(path, p, &v);
            skip_netlink_value(path, p, &v);
            yaml_event_delete(&v);
#endif
        } else {
            yaml_die(path, p,
                     "unknown room key '%s' for room '%s'", k, room_name);
        }
    }

    if (!saw_map || !saw_label || !saw_links || !saw_desc) {
        yaml_die(path, p,
                 "room '%s' missing required field(s):%s%s%s%s",
                 room_name,
                 saw_map   ? "" : " map",
                 saw_label ? "" : " label",
                 saw_links ? "" : " links",
                 saw_desc  ? "" : " description");
    }

    /* Self-link check: walk the names we collected, compare against
     * the room's own name (the legacy parser compared link labels to
     * the room's label; in YAML the 'links' sequence is by name). */
    for (int i = 0; i < pl.count; ++i) {
        if (!strcmp(pl.names[i], rm->name)) {
            yaml_die(path, p,
                     "room '%s' has a link to itself", rm->name);
        }
    }

    return pl;
}

/* ------------------------------------------------------------------ */
/* Top-level entry point                                               */
/* ------------------------------------------------------------------ */

void
yaml_load_rooms(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Amnuts: Cannot open rooms file '%s'\n", path);
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

    /* Top-level mapping has exactly one key: 'rooms'. */
    yaml_event_t k = yaml_expect(path, &parser, YAML_SCALAR_EVENT,
                                 "top-level key");
    if (strcmp(scalar_str(&k), "rooms")) {
        yaml_die(path, &parser,
                 "expected top-level key 'rooms' (got '%s')",
                 scalar_str(&k));
    }
    yaml_event_delete(&k);

    ev = yaml_expect(path, &parser, YAML_MAPPING_START_EVENT,
                     "rooms mapping");
    yaml_event_delete(&ev);

    /* Pass 1: parse every room. */
    struct pending_links *pending = NULL;
    int pcap = 0;
    int pn = 0;

    for (;;) {
        yaml_event_t name_ev;
        yaml_next(path, &parser, &name_ev);

        if (name_ev.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&name_ev);
            break;
        }
        if (name_ev.type != YAML_SCALAR_EVENT) {
            yaml_die(path, &parser, "expected room name (scalar)");
        }

        char rname[ROOM_NAME_LEN + 2]; /* +2 to detect over-length without truncating */
        snprintf(rname, sizeof rname, "%s", scalar_str(&name_ev));
        yaml_event_delete(&name_ev);

        ev = yaml_expect(path, &parser, YAML_MAPPING_START_EVENT,
                         "room body");
        yaml_event_delete(&ev);

        if (pn >= pcap) {
            pcap = pcap ? pcap * 2 : 16;
            pending = realloc(pending, pcap * sizeof *pending);
            if (!pending) {
                yaml_die(path, &parser,
                         "out of memory tracking pending room links");
            }
        }
        pending[pn++] = load_one_room(path, &parser, rname);
    }

    /* Top-level mapping end + document/stream end */
    ev = yaml_expect(path, &parser, YAML_MAPPING_END_EVENT,
                     "top-level mapping end");
    yaml_event_delete(&ev);
    ev = yaml_expect(path, &parser, YAML_DOCUMENT_END_EVENT,
                     "document end");
    yaml_event_delete(&ev);
    ev = yaml_expect(path, &parser, YAML_STREAM_END_EVENT, "stream end");
    yaml_event_delete(&ev);

    yaml_parser_delete(&parser);
    fclose(fp);

    /* Pass 2: resolve link names to room pointers. */
    for (int i = 0; i < pn; ++i) {
        struct pending_links *pl = &pending[i];
        for (int j = 0; j < pl->count; ++j) {
            RM_OBJECT target = NULL;
            for (RM_OBJECT cand = room_first; cand; cand = cand->next) {
                if (!strcmp(cand->name, pl->names[j])) {
                    target = cand;
                    break;
                }
            }
            if (!target) {
                fprintf(stderr,
                        "Amnuts: Room %s has undefined link '%s'.\n",
                        pl->room->name, pl->names[j]);
                boot_exit(1);
            }
            if (target == pl->room) {
                fprintf(stderr,
                        "Amnuts: Room %s cannot link to itself.\n",
                        pl->room->name);
                boot_exit(1);
            }
            pl->room->link[j] = target;
            free(pl->names[j]);
        }
        free(pl->names);
    }
    free(pending);
}


/*
 * Reload descriptions for one or all non-personal rooms from rooms.yaml.
 *
 * Unlike the loaders called at boot, this function never calls boot_exit:
 * a malformed rooms.yaml at runtime should not take down the talker.
 * Errors are reported via errbuf (if non-NULL); the function returns the
 * number of in-memory rooms whose descriptions were updated, or -1 on
 * any parse/IO error.
 *
 * If `target` is NULL or empty, every room in the YAML is considered;
 * otherwise only the named room is touched. Personal rooms are always
 * skipped (their descriptions live in files/userfiles/rooms/, not here).
 *
 * Only the description field is refreshed. Topic, access, links, and
 * netlink wiring are preserved as they are in memory — runtime
 * mutations to those (via .topic, .private, etc.) are not clobbered.
 */
int
yaml_reload_descriptions(const char *path, const char *target,
                         char *errbuf, size_t errbuf_size)
{
    FILE *fp = NULL;
    yaml_parser_t parser;
    int parser_initialized = 0;
    char *new_desc = NULL;
    int updated = 0;
    yaml_event_t ev;

#define R_FAIL(...) do { \
        if (errbuf) snprintf(errbuf, errbuf_size, __VA_ARGS__); \
        goto fail; \
    } while (0)

#define R_NEXT(EV) do { \
        if (!yaml_parser_parse(&parser, EV)) { \
            R_FAIL("%s:%lu: parse failure: %s", path, \
                   (unsigned long)parser.problem_mark.line + 1, \
                   parser.problem ? parser.problem : "?"); \
        } \
    } while (0)

#define R_EXPECT(EV, TYPE, CTX) do { \
        R_NEXT(EV); \
        if ((EV)->type != (TYPE)) { \
            yaml_event_delete(EV); \
            R_FAIL("%s: expected %s while parsing %s", path, #TYPE, CTX); \
        } \
    } while (0)

    fp = fopen(path, "r");
    if (!fp) {
        R_FAIL("Cannot open %s", path);
    }

    yaml_parser_initialize(&parser);
    parser_initialized = 1;
    yaml_parser_set_input_file(&parser, fp);

    R_EXPECT(&ev, YAML_STREAM_START_EVENT, "stream");
    yaml_event_delete(&ev);
    R_EXPECT(&ev, YAML_DOCUMENT_START_EVENT, "document");
    yaml_event_delete(&ev);
    R_EXPECT(&ev, YAML_MAPPING_START_EVENT, "top-level mapping");
    yaml_event_delete(&ev);

    R_EXPECT(&ev, YAML_SCALAR_EVENT, "top-level key");
    if (strcmp((const char *)ev.data.scalar.value, "rooms")) {
        yaml_event_delete(&ev);
        R_FAIL("%s: expected top-level key 'rooms'", path);
    }
    yaml_event_delete(&ev);

    R_EXPECT(&ev, YAML_MAPPING_START_EVENT, "rooms mapping");
    yaml_event_delete(&ev);

    /* Walk each room mapping. */
    for (;;) {
        R_NEXT(&ev);
        if (ev.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&ev);
            break;
        }
        if (ev.type != YAML_SCALAR_EVENT) {
            yaml_event_delete(&ev);
            R_FAIL("%s: expected scalar room name", path);
        }
        char rname[ROOM_NAME_LEN + 1];
        snprintf(rname, sizeof rname, "%s",
                 (const char *)ev.data.scalar.value);
        yaml_event_delete(&ev);

        R_EXPECT(&ev, YAML_MAPPING_START_EVENT, "room body");
        yaml_event_delete(&ev);

        int match = !target || !*target || !strcmp(target, rname);
        free(new_desc);
        new_desc = NULL;

        for (;;) {
            yaml_event_t key;
            R_NEXT(&key);
            if (key.type == YAML_MAPPING_END_EVENT) {
                yaml_event_delete(&key);
                break;
            }
            if (key.type != YAML_SCALAR_EVENT) {
                yaml_event_delete(&key);
                R_FAIL("%s: expected scalar key in room body", path);
            }
            int wants_desc = match && new_desc == NULL
                    && !strcmp((const char *)key.data.scalar.value,
                               "description");
            yaml_event_delete(&key);

            if (wants_desc) {
                yaml_event_t v;
                R_EXPECT(&v, YAML_SCALAR_EVENT, "description value");
                new_desc = strdup((const char *)v.data.scalar.value);
                yaml_event_delete(&v);
            } else {
                /* Skip the value, handling nested mappings/sequences. */
                int depth = 0;
                yaml_event_t v;
                R_NEXT(&v);
                if (v.type == YAML_SEQUENCE_START_EVENT
                        || v.type == YAML_MAPPING_START_EVENT) {
                    depth = 1;
                }
                yaml_event_delete(&v);
                while (depth > 0) {
                    R_NEXT(&v);
                    if (v.type == YAML_SEQUENCE_START_EVENT
                            || v.type == YAML_MAPPING_START_EVENT) {
                        ++depth;
                    } else if (v.type == YAML_SEQUENCE_END_EVENT
                            || v.type == YAML_MAPPING_END_EVENT) {
                        --depth;
                    }
                    yaml_event_delete(&v);
                }
            }
        }

        if (match && new_desc) {
            for (RM_OBJECT rm = room_first; rm; rm = rm->next) {
                if (!strcmp(rm->name, rname) && !is_personal_room(rm)) {
                    size_t len = strlen(new_desc);
                    if (len > ROOM_DESC_LEN) {
                        len = ROOM_DESC_LEN;
                    }
                    memcpy(rm->desc, new_desc, len);
                    rm->desc[len] = '\0';
                    ++updated;
                    break;
                }
            }
        }
    }

    yaml_parser_delete(&parser);
    fclose(fp);
    free(new_desc);
    return updated;

fail:
    if (parser_initialized) {
        yaml_parser_delete(&parser);
    }
    if (fp) {
        fclose(fp);
    }
    free(new_desc);
    return -1;

#undef R_FAIL
#undef R_NEXT
#undef R_EXPECT
}
