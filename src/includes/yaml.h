#ifndef AMNUTS_YAML_H
#define AMNUTS_YAML_H

#include "../vendors/libyaml/yaml.h"
#include <stdbool.h>

/* Print a parse error in "file:line:column: message" form to stderr,
 * then call boot_exit(1). Used by every loader to fail fast. */
void yaml_die(const char *path, yaml_parser_t *parser, const char *fmt, ...)
    __attribute__((format(printf, 3, 4), noreturn));

/* Read the next event into `event`. On parse failure, calls yaml_die. */
void yaml_next(const char *path, yaml_parser_t *parser, yaml_event_t *event);

/* Expect the next event to be of `type`; fail with yaml_die otherwise.
 * Returns the event so the caller can read its data. The caller owns
 * the event and must yaml_event_delete() it. */
yaml_event_t yaml_expect(const char *path, yaml_parser_t *parser,
                         yaml_event_type_t type, const char *context);

/* Convert a scalar event's value to a bool. Accepts: true/false, yes/no,
 * 1/0 (case insensitive). Calls yaml_die on anything else. */
bool yaml_scalar_to_bool(const char *path, yaml_event_t *ev);

/* Convert a scalar event's value to int. Calls yaml_die on parse failure
 * or out-of-range. */
int yaml_scalar_to_int(const char *path, yaml_event_t *ev);

/* Map a level name ("USER", "WIZ", etc.) to enum lvl_value. Returns
 * NUM_LEVELS as a sentinel for "NONE". Calls yaml_die if unknown. */
int yaml_scalar_to_level(const char *path, yaml_event_t *ev);

#endif /* AMNUTS_YAML_H */
