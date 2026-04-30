/*
 * yaml_config.c
 *
 * YAML loader for the talker's main config file.
 *
 * Parses files/datafiles/config.yaml using libyaml's event-stream API and
 * populates the global SYS_OBJECT amsys (and optionally creates netlink
 * entries for sites).
 *
 * The structure of the YAML mirrors the legacy "INIT" / "SITES" sections
 * of the flat config; see parse_init_section() and parse_sites_section()
 * in src/amnuts.c for the source-of-truth validation rules that this
 * loader mirrors.
 */

#include "defines.h"
#include "globals.h"
#include "prototypes.h"
#include "yaml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* Forward declarations                                                */
/* ------------------------------------------------------------------ */

static void load_server(const char *path, yaml_parser_t *p);
static void load_ports(const char *path, yaml_parser_t *p);
static void load_timeouts(const char *path, yaml_parser_t *p);
static void load_defaults(const char *path, yaml_parser_t *p);
static void load_moderation(const char *path, yaml_parser_t *p);
static void load_system(const char *path, yaml_parser_t *p);
static void load_users(const char *path, yaml_parser_t *p);
static void load_messages(const char *path, yaml_parser_t *p);
#ifdef NETLINKS
static void load_sites(const char *path, yaml_parser_t *p);
#endif

static void skip_value(const char *path, yaml_parser_t *p);

static int  parse_crash_action(const char *path, yaml_event_t *ev);
static int  parse_resolve_ip(const char *path, yaml_event_t *ev);
static int  parse_ban_swearing(const char *path, yaml_event_t *ev);
#ifdef NETLINKS
static enum nl_allow parse_nl_allow(const char *path, yaml_event_t *ev);
#endif

static int  level_required(const char *path, yaml_event_t *ev,
                           const char *field);

/* ------------------------------------------------------------------ */
/* Tiny helpers                                                        */
/* ------------------------------------------------------------------ */

static char *
scalar_str(yaml_event_t *ev)
{
    return (char *)ev->data.scalar.value;
}

/*
 * Consume one value: scalar, sequence (recursively), or mapping
 * (recursively). Used as a generic skip but currently only invoked on
 * unknown-key error paths if we ever want to soften the parser.
 */
static void
skip_value(const char *path, yaml_parser_t *p)
{
    yaml_event_t ev;
    int depth = 0;

    do {
        yaml_next(path, p, &ev);
        switch (ev.type) {
        case YAML_SEQUENCE_START_EVENT:
        case YAML_MAPPING_START_EVENT:
            ++depth;
            break;
        case YAML_SEQUENCE_END_EVENT:
        case YAML_MAPPING_END_EVENT:
            --depth;
            break;
        default:
            break;
        }
        yaml_event_delete(&ev);
    } while (depth > 0);
}

static int
level_required(const char *path, yaml_event_t *ev, const char *field)
{
    int lvl = yaml_scalar_to_level(path, ev);
    if (lvl == NUM_LEVELS) {
        yaml_die(path, NULL,
                 "%s does not allow NONE; expected a level name", field);
    }
    return lvl;
}

static int
parse_crash_action(const char *path, yaml_event_t *ev)
{
    const char *v = scalar_str(ev);
    if (!strcasecmp(v, "NONE"))     return 0;
    if (!strcasecmp(v, "SHUTDOWN")) return 1;
    if (!strcasecmp(v, "REBOOT"))   return 2;
    if (!strcasecmp(v, "SEAMLESS")) return 3;
    yaml_die(path, NULL,
             "system.crash_action must be NONE, SHUTDOWN, REBOOT or SEAMLESS"
             " (got '%s')", v);
}

static int
parse_resolve_ip(const char *path, yaml_event_t *ev)
{
    const char *v = scalar_str(ev);
    if (!strcasecmp(v, "OFF"))    return 0;
    if (!strcasecmp(v, "AUTO"))   return 1;
    if (!strcasecmp(v, "MANUAL")) return 2;
    if (!strcasecmp(v, "IDENTD")) return 3;
    yaml_die(path, NULL,
             "system.resolve_ip must be OFF, AUTO, MANUAL or IDENTD"
             " (got '%s')", v);
}

static int
parse_ban_swearing(const char *path, yaml_event_t *ev)
{
    const char *v = scalar_str(ev);
    if (!strcasecmp(v, "OFF")) return SBOFF;
    if (!strcasecmp(v, "MIN")) return SBMIN;
    if (!strcasecmp(v, "MAX")) return SBMAX;
    yaml_die(path, NULL,
             "moderation.ban_swearing must be OFF, MIN or MAX (got '%s')", v);
}

#ifdef NETLINKS
static enum nl_allow
parse_nl_allow(const char *path, yaml_event_t *ev)
{
    const char *v = scalar_str(ev);
    if (!strcasecmp(v, "ALL")) return ALL;
    if (!strcasecmp(v, "IN"))  return IN;
    if (!strcasecmp(v, "OUT")) return OUT;
    yaml_die(path, NULL,
             "site.allow must be ALL, IN or OUT (got '%s')", v);
}
#endif

/* ------------------------------------------------------------------ */
/* server: section                                                     */
/* ------------------------------------------------------------------ */

static void
load_ports(const char *path, yaml_parser_t *p)
{
    yaml_event_t ev = yaml_expect(path, p, YAML_MAPPING_START_EVENT,
                                  "server.ports mapping");
    yaml_event_delete(&ev);

    for (;;) {
        yaml_event_t key;
        yaml_next(path, p, &key);
        if (key.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key);
            return;
        }
        if (key.type != YAML_SCALAR_EVENT) {
            yaml_die(path, p, "expected scalar key in server.ports");
        }

        char k[64];
        snprintf(k, sizeof k, "%s", scalar_str(&key));
        yaml_event_delete(&key);

        yaml_event_t val = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                       "server.ports value");

        const char *v = scalar_str(&val);
        if (strlen(v) >= MAXSERV) {
            yaml_die(path, NULL,
                     "server.ports.%s: port string too long", k);
        }

        if (!strcmp(k, "main")) {
            strcpy(amsys->mport_port, v);
        } else if (!strcmp(k, "wiz")) {
#ifdef WIZPORT
            strcpy(amsys->wport_port, v);
#endif
        } else if (!strcmp(k, "link")) {
#ifdef NETLINKS
            strcpy(amsys->nlink_port, v);
#endif
        } else {
            yaml_die(path, NULL,
                     "unknown key 'server.ports.%s'", k);
        }
        yaml_event_delete(&val);
    }
}

static void
load_server(const char *path, yaml_parser_t *p)
{
    yaml_event_t ev = yaml_expect(path, p, YAML_MAPPING_START_EVENT,
                                  "server mapping");
    yaml_event_delete(&ev);

    for (;;) {
        yaml_event_t key;
        yaml_next(path, p, &key);
        if (key.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key);
            return;
        }
        if (key.type != YAML_SCALAR_EVENT) {
            yaml_die(path, p, "expected scalar key in server mapping");
        }

        char k[64];
        snprintf(k, sizeof k, "%s", scalar_str(&key));
        yaml_event_delete(&key);

        if (!strcmp(k, "ports")) {
            load_ports(path, p);
            continue;
        }

        yaml_event_t val = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                       "server.* value");

        if (!strcmp(k, "verification")) {
            const char *v = scalar_str(&val);
            if (strlen(v) > VERIFY_LEN) {
                yaml_die(path, NULL,
                         "server.verification too long (max %d chars)",
                         VERIFY_LEN);
            }
#ifdef NETLINKS
            strcpy(amsys->verification, v);
#endif
        } else if (!strcmp(k, "max_users")) {
            int n = yaml_scalar_to_int(path, &val);
            if (n < 1) {
                yaml_die(path, NULL,
                         "server.max_users must be >= 1 (got %d)", n);
            }
            amsys->max_users = n;
        } else if (!strcmp(k, "max_clones")) {
            int n = yaml_scalar_to_int(path, &val);
            if (n < 0) {
                yaml_die(path, NULL,
                         "server.max_clones must be >= 0 (got %d)", n);
            }
            amsys->max_clones = n;
        } else if (!strcmp(k, "heartbeat")) {
            int n = yaml_scalar_to_int(path, &val);
            if (n < 1) {
                yaml_die(path, NULL,
                         "server.heartbeat must be >= 1 (got %d)", n);
            }
            amsys->heartbeat = n;
        } else {
            yaml_die(path, NULL, "unknown key 'server.%s'", k);
        }
        yaml_event_delete(&val);
    }
}

/* ------------------------------------------------------------------ */
/* timeouts: section                                                   */
/* ------------------------------------------------------------------ */

static void
load_timeouts(const char *path, yaml_parser_t *p)
{
    yaml_event_t ev = yaml_expect(path, p, YAML_MAPPING_START_EVENT,
                                  "timeouts mapping");
    yaml_event_delete(&ev);

    for (;;) {
        yaml_event_t key;
        yaml_next(path, p, &key);
        if (key.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key);
            return;
        }
        if (key.type != YAML_SCALAR_EVENT) {
            yaml_die(path, p, "expected scalar key in timeouts mapping");
        }

        char k[64];
        snprintf(k, sizeof k, "%s", scalar_str(&key));
        yaml_event_delete(&key);

        yaml_event_t val = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                       "timeouts.* value");

        if (!strcmp(k, "login_idle")) {
            int n = yaml_scalar_to_int(path, &val);
            if (n < 10) {
                yaml_die(path, NULL,
                         "timeouts.login_idle must be >= 10 (got %d)", n);
            }
            amsys->login_idle_time = n;
        } else if (!strcmp(k, "user_idle")) {
            int n = yaml_scalar_to_int(path, &val);
            if (n < 10) {
                yaml_die(path, NULL,
                         "timeouts.user_idle must be >= 10 (got %d)", n);
            }
            amsys->user_idle_time = n;
        } else if (!strcmp(k, "timeout_afks")) {
            amsys->time_out_afks = yaml_scalar_to_bool(path, &val) ? 1 : 0;
        } else if (!strcmp(k, "timeout_maxlevel")) {
            amsys->time_out_maxlevel =
                level_required(path, &val, "timeouts.timeout_maxlevel");
        } else {
            yaml_die(path, NULL, "unknown key 'timeouts.%s'", k);
        }
        yaml_event_delete(&val);
    }
}

/* ------------------------------------------------------------------ */
/* defaults: section                                                   */
/* ------------------------------------------------------------------ */

static void
load_defaults(const char *path, yaml_parser_t *p)
{
    yaml_event_t ev = yaml_expect(path, p, YAML_MAPPING_START_EVENT,
                                  "defaults mapping");
    yaml_event_delete(&ev);

    for (;;) {
        yaml_event_t key;
        yaml_next(path, p, &key);
        if (key.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key);
            return;
        }
        if (key.type != YAML_SCALAR_EVENT) {
            yaml_die(path, p, "expected scalar key in defaults mapping");
        }

        char k[64];
        snprintf(k, sizeof k, "%s", scalar_str(&key));
        yaml_event_delete(&key);

        yaml_event_t val = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                       "defaults.* value");

        if (!strcmp(k, "colour")) {
            amsys->colour_def = yaml_scalar_to_bool(path, &val) ? 1 : 0;
        } else if (!strcmp(k, "prompt")) {
            amsys->prompt_def = yaml_scalar_to_bool(path, &val) ? 1 : 0;
        } else if (!strcmp(k, "charecho")) {
            amsys->charecho_def = yaml_scalar_to_bool(path, &val) ? 1 : 0;
        } else if (!strcmp(k, "passwordecho")) {
            amsys->passwordecho_def = yaml_scalar_to_bool(path, &val) ? 1 : 0;
        } else if (!strcmp(k, "warp_room")) {
            const char *v = scalar_str(&val);
            if (strlen(v) > ROOM_NAME_LEN) {
                yaml_die(path, NULL,
                         "defaults.warp_room: room name too long (max %d)",
                         ROOM_NAME_LEN);
            }
            strcpy(amsys->default_warp, v);
        } else if (!strcmp(k, "jail_room")) {
            const char *v = scalar_str(&val);
            if (strlen(v) > ROOM_NAME_LEN) {
                yaml_die(path, NULL,
                         "defaults.jail_room: room name too long (max %d)",
                         ROOM_NAME_LEN);
            }
            strcpy(amsys->default_jail, v);
        } else if (!strcmp(k, "bank_room")) {
            const char *v = scalar_str(&val);
            if (strlen(v) > ROOM_NAME_LEN) {
                yaml_die(path, NULL,
                         "defaults.bank_room: room name too long (max %d)",
                         ROOM_NAME_LEN);
            }
#ifdef GAMES
            strcpy(amsys->default_bank, v);
#endif
        } else if (!strcmp(k, "shoot_room")) {
            const char *v = scalar_str(&val);
            if (strlen(v) > ROOM_NAME_LEN) {
                yaml_die(path, NULL,
                         "defaults.shoot_room: room name too long (max %d)",
                         ROOM_NAME_LEN);
            }
#ifdef GAMES
            strcpy(amsys->default_shoot, v);
#endif
        } else {
            yaml_die(path, NULL, "unknown key 'defaults.%s'", k);
        }
        yaml_event_delete(&val);
    }
}

/* ------------------------------------------------------------------ */
/* moderation: section                                                 */
/* ------------------------------------------------------------------ */

static void
load_moderation(const char *path, yaml_parser_t *p)
{
    yaml_event_t ev = yaml_expect(path, p, YAML_MAPPING_START_EVENT,
                                  "moderation mapping");
    yaml_event_delete(&ev);

    for (;;) {
        yaml_event_t key;
        yaml_next(path, p, &key);
        if (key.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key);
            return;
        }
        if (key.type != YAML_SCALAR_EVENT) {
            yaml_die(path, p, "expected scalar key in moderation mapping");
        }

        char k[64];
        snprintf(k, sizeof k, "%s", scalar_str(&key));
        yaml_event_delete(&key);

        yaml_event_t val = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                       "moderation.* value");

        if (!strcmp(k, "ban_swearing")) {
            amsys->ban_swearing = parse_ban_swearing(path, &val);
        } else if (!strcmp(k, "minlogin_level")) {
            /* NONE is allowed here — sentinel NUM_LEVELS */
            amsys->minlogin_level = yaml_scalar_to_level(path, &val);
        } else if (!strcmp(k, "min_private")) {
            int n = yaml_scalar_to_int(path, &val);
            if (n < 1) {
                yaml_die(path, NULL,
                         "moderation.min_private must be >= 1 (got %d)", n);
            }
            amsys->min_private_users = n;
        } else if (!strcmp(k, "ignore_mp_level")) {
            amsys->ignore_mp_level =
                level_required(path, &val, "moderation.ignore_mp_level");
        } else if (!strcmp(k, "gatecrash_level")) {
            amsys->gatecrash_level =
                level_required(path, &val, "moderation.gatecrash_level");
        } else if (!strcmp(k, "boot_off_min")) {
            amsys->boot_off_min = yaml_scalar_to_bool(path, &val) ? 1 : 0;
        } else if (!strcmp(k, "flood_protect")) {
            amsys->flood_protect = yaml_scalar_to_bool(path, &val) ? 1 : 0;
        } else {
            yaml_die(path, NULL, "unknown key 'moderation.%s'", k);
        }
        yaml_event_delete(&val);
    }
}

/* ------------------------------------------------------------------ */
/* system: section                                                     */
/* ------------------------------------------------------------------ */

static void
load_system(const char *path, yaml_parser_t *p)
{
    yaml_event_t ev = yaml_expect(path, p, YAML_MAPPING_START_EVENT,
                                  "system mapping");
    yaml_event_delete(&ev);

    for (;;) {
        yaml_event_t key;
        yaml_next(path, p, &key);
        if (key.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key);
            return;
        }
        if (key.type != YAML_SCALAR_EVENT) {
            yaml_die(path, p, "expected scalar key in system mapping");
        }

        char k[64];
        snprintf(k, sizeof k, "%s", scalar_str(&key));
        yaml_event_delete(&key);

        yaml_event_t val = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                       "system.* value");

        if (!strcmp(k, "logging")) {
            bool b = yaml_scalar_to_bool(path, &val);
            amsys->logging = b ? (SYSLOG | REQLOG | NETLOG | ERRLOG) : 0;
        } else if (!strcmp(k, "ignore_sigterm")) {
            amsys->ignore_sigterm = yaml_scalar_to_bool(path, &val) ? 1 : 0;
        } else if (!strcmp(k, "auto_connect")) {
            int b = yaml_scalar_to_bool(path, &val) ? 1 : 0;
#ifdef NETLINKS
            amsys->auto_connect = b;
#else
            (void)b;
#endif
        } else if (!strcmp(k, "crash_action")) {
            amsys->crash_action = parse_crash_action(path, &val);
        } else if (!strcmp(k, "resolve_ip")) {
            amsys->resolve_ip = parse_resolve_ip(path, &val);
        } else if (!strcmp(k, "random_motds")) {
            amsys->random_motds = yaml_scalar_to_bool(path, &val) ? 1 : 0;
        } else {
            yaml_die(path, NULL, "unknown key 'system.%s'", k);
        }
        yaml_event_delete(&val);
    }
}

/* ------------------------------------------------------------------ */
/* users: section                                                      */
/* ------------------------------------------------------------------ */

static void
load_users(const char *path, yaml_parser_t *p)
{
    yaml_event_t ev = yaml_expect(path, p, YAML_MAPPING_START_EVENT,
                                  "users mapping");
    yaml_event_delete(&ev);

    for (;;) {
        yaml_event_t key;
        yaml_next(path, p, &key);
        if (key.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key);
            return;
        }
        if (key.type != YAML_SCALAR_EVENT) {
            yaml_die(path, p, "expected scalar key in users mapping");
        }

        char k[64];
        snprintf(k, sizeof k, "%s", scalar_str(&key));
        yaml_event_delete(&key);

        yaml_event_t val = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                       "users.* value");

        if (!strcmp(k, "auto_purge")) {
            bool b = yaml_scalar_to_bool(path, &val);
            if (!b) {
                amsys->auto_purge_date = -1;
            } else {
                amsys->auto_purge_date = time(0) + 86400 - 1;
                amsys->auto_purge_date -= amsys->auto_purge_date % 86400;
            }
        } else if (!strcmp(k, "allow_recaps")) {
            amsys->allow_recaps = yaml_scalar_to_bool(path, &val) ? 1 : 0;
        } else if (!strcmp(k, "auto_promote")) {
            amsys->auto_promote = yaml_scalar_to_bool(path, &val) ? 1 : 0;
        } else if (!strcmp(k, "personal_rooms")) {
            amsys->personal_rooms = yaml_scalar_to_bool(path, &val) ? 1 : 0;
        } else if (!strcmp(k, "startup_room_parse")) {
            amsys->startup_room_parse =
                yaml_scalar_to_bool(path, &val) ? 1 : 0;
        } else if (!strcmp(k, "rem_user_maxlevel")) {
            int lvl = level_required(path, &val, "users.rem_user_maxlevel");
#ifdef NETLINKS
            amsys->rem_user_maxlevel = lvl;
#else
            (void)lvl;
#endif
        } else if (!strcmp(k, "rem_user_deflevel")) {
            int lvl = level_required(path, &val, "users.rem_user_deflevel");
#ifdef NETLINKS
            amsys->rem_user_deflevel = lvl;
#else
            (void)lvl;
#endif
        } else if (!strcmp(k, "wizport_level")) {
            int lvl = level_required(path, &val, "users.wizport_level");
#ifdef WIZPORT
            amsys->wizport_level = lvl;
#else
            (void)lvl;
#endif
        } else {
            yaml_die(path, NULL, "unknown key 'users.%s'", k);
        }
        yaml_event_delete(&val);
    }
}

/* ------------------------------------------------------------------ */
/* messages: section                                                   */
/* ------------------------------------------------------------------ */

static void
load_messages(const char *path, yaml_parser_t *p)
{
    yaml_event_t ev = yaml_expect(path, p, YAML_MAPPING_START_EVENT,
                                  "messages mapping");
    yaml_event_delete(&ev);

    for (;;) {
        yaml_event_t key;
        yaml_next(path, p, &key);
        if (key.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key);
            return;
        }
        if (key.type != YAML_SCALAR_EVENT) {
            yaml_die(path, p, "expected scalar key in messages mapping");
        }

        char k[64];
        snprintf(k, sizeof k, "%s", scalar_str(&key));
        yaml_event_delete(&key);

        yaml_event_t val = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                       "messages.* value");

        if (!strcmp(k, "lifetime_days")) {
            int n = yaml_scalar_to_int(path, &val);
            if (n < 1) {
                yaml_die(path, NULL,
                         "messages.lifetime_days must be >= 1 (got %d)", n);
            }
            amsys->mesg_life = n;
        } else if (!strcmp(k, "check_time")) {
            const char *v = scalar_str(&val);
            int hh = -1, mm = -1;
            /* sscanf %d:%d works whether v is "01:00" or "1:0".
             * We never see YAML-parsed sexagesimal here because libyaml
             * gives us the raw scalar string. */
            if (sscanf(v, "%d:%d", &hh, &mm) != 2 ||
                hh < 0 || hh > 23 ||
                mm < 0 || mm > 59) {
                yaml_die(path, NULL,
                         "messages.check_time must be HH:MM (got '%s')", v);
            }
            amsys->mesg_check_hour = hh;
            amsys->mesg_check_min  = mm;
            amsys->mesg_check_done = time(0) + 86400 - 1;
            amsys->mesg_check_done -= amsys->mesg_check_done % 86400;
            amsys->mesg_check_done +=
                3600 * amsys->mesg_check_hour + 60 * amsys->mesg_check_min;
        } else {
            yaml_die(path, NULL, "unknown key 'messages.%s'", k);
        }
        yaml_event_delete(&val);
    }
}

/* ------------------------------------------------------------------ */
/* sites: section (NETLINKS only)                                      */
/* ------------------------------------------------------------------ */

#ifdef NETLINKS
static void
load_one_site(const char *path, yaml_parser_t *p, const char *service)
{
    yaml_event_t ev = yaml_expect(path, p, YAML_MAPPING_START_EVENT,
                                  "site mapping");
    yaml_event_delete(&ev);

    NL_OBJECT nl = create_netlink();
    if (!nl) {
        yaml_die(path, NULL,
                 "sites.%s: memory allocation failure creating netlink",
                 service);
    }

    if (strlen(service) > SERV_NAME_LEN) {
        yaml_die(path, NULL,
                 "sites.%s: link name too long (max %d)",
                 service, SERV_NAME_LEN);
    }
    strcpy(nl->service, service);

    /* default if 'allow' is omitted */
    nl->allow = ALL;

    bool got_address = false, got_port = false, got_verification = false;

    for (;;) {
        yaml_event_t key;
        yaml_next(path, p, &key);
        if (key.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key);
            break;
        }
        if (key.type != YAML_SCALAR_EVENT) {
            yaml_die(path, p, "expected scalar key in sites.%s", service);
        }

        char k[64];
        snprintf(k, sizeof k, "%s", scalar_str(&key));
        yaml_event_delete(&key);

        yaml_event_t val = yaml_expect(path, p, YAML_SCALAR_EVENT,
                                       "sites.<name>.* value");

        if (!strcmp(k, "address")) {
            const char *v = scalar_str(&val);
            if (strlen(v) >= MAXHOST) {
                yaml_die(path, NULL,
                         "sites.%s.address too long (max %d)",
                         service, MAXHOST - 1);
            }
            strcpy(nl->site, v);
            strtolower(nl->site);
            got_address = true;
        } else if (!strcmp(k, "port")) {
            int n = yaml_scalar_to_int(path, &val);
            if (n < 0 || n > 65535) {
                yaml_die(path, NULL,
                         "sites.%s.port out of range (got %d)", service, n);
            }
            int written = snprintf(nl->port, sizeof nl->port, "%d", n);
            if (written < 0 || (size_t)written >= sizeof nl->port) {
                yaml_die(path, NULL,
                         "sites.%s.port stringification overflow",
                         service);
            }
            got_port = true;
        } else if (!strcmp(k, "verification")) {
            const char *v = scalar_str(&val);
            if (strlen(v) > VERIFY_LEN) {
                yaml_die(path, NULL,
                         "sites.%s.verification too long (max %d)",
                         service, VERIFY_LEN);
            }
            strcpy(nl->verification, v);
            got_verification = true;
        } else if (!strcmp(k, "allow")) {
            nl->allow = parse_nl_allow(path, &val);
        } else {
            yaml_die(path, NULL,
                     "unknown key 'sites.%s.%s'", service, k);
        }
        yaml_event_delete(&val);
    }

    if (!got_address) {
        yaml_die(path, NULL, "sites.%s missing required 'address'", service);
    }
    if (!got_port) {
        yaml_die(path, NULL, "sites.%s missing required 'port'", service);
    }
    if (!got_verification) {
        yaml_die(path, NULL,
                 "sites.%s missing required 'verification'", service);
    }
}

static void
load_sites(const char *path, yaml_parser_t *p)
{
    yaml_event_t ev = yaml_expect(path, p, YAML_MAPPING_START_EVENT,
                                  "sites mapping");
    yaml_event_delete(&ev);

    for (;;) {
        yaml_event_t key;
        yaml_next(path, p, &key);
        if (key.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key);
            return;
        }
        if (key.type != YAML_SCALAR_EVENT) {
            yaml_die(path, p, "expected scalar service name in sites mapping");
        }

        char service[SERV_NAME_LEN + 1];
        snprintf(service, sizeof service, "%s", scalar_str(&key));
        yaml_event_delete(&key);

        load_one_site(path, p, service);
    }
}
#endif /* NETLINKS */

/* ------------------------------------------------------------------ */
/* Top-level entry point                                               */
/* ------------------------------------------------------------------ */

void
yaml_load_config(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Amnuts: Cannot open config file '%s'\n", path);
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

    for (;;) {
        yaml_event_t key;
        yaml_next(path, &parser, &key);
        if (key.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&key);
            break;
        }
        if (key.type != YAML_SCALAR_EVENT) {
            yaml_die(path, &parser,
                     "expected scalar key in top-level mapping");
        }

        char k[64];
        snprintf(k, sizeof k, "%s", scalar_str(&key));
        yaml_event_delete(&key);

        if (!strcmp(k, "server")) {
            load_server(path, &parser);
        } else if (!strcmp(k, "timeouts")) {
            load_timeouts(path, &parser);
        } else if (!strcmp(k, "defaults")) {
            load_defaults(path, &parser);
        } else if (!strcmp(k, "moderation")) {
            load_moderation(path, &parser);
        } else if (!strcmp(k, "system")) {
            load_system(path, &parser);
        } else if (!strcmp(k, "users")) {
            load_users(path, &parser);
        } else if (!strcmp(k, "messages")) {
            load_messages(path, &parser);
#ifdef NETLINKS
        } else if (!strcmp(k, "sites")) {
            load_sites(path, &parser);
#endif
        } else {
            /* Unknown top-level group: be strict. skip_value() exists
             * if we ever want to soften this. */
            (void)skip_value;
            yaml_die(path, &parser, "unknown top-level key '%s'", k);
        }
    }

    ev = yaml_expect(path, &parser, YAML_DOCUMENT_END_EVENT, "document end");
    yaml_event_delete(&ev);
    ev = yaml_expect(path, &parser, YAML_STREAM_END_EVENT, "stream end");
    yaml_event_delete(&ev);

    yaml_parser_delete(&parser);
    fclose(fp);
}
