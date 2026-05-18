#include "defines.h"
#include "globals.h"
#include "prototypes.h"
#include "yaml.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char *
event_type_name(yaml_event_type_t t)
{
    switch (t) {
    case YAML_NO_EVENT:             return "NO_EVENT";
    case YAML_STREAM_START_EVENT:   return "STREAM_START";
    case YAML_STREAM_END_EVENT:     return "STREAM_END";
    case YAML_DOCUMENT_START_EVENT: return "DOCUMENT_START";
    case YAML_DOCUMENT_END_EVENT:   return "DOCUMENT_END";
    case YAML_ALIAS_EVENT:          return "ALIAS";
    case YAML_SCALAR_EVENT:         return "SCALAR";
    case YAML_SEQUENCE_START_EVENT: return "SEQUENCE_START";
    case YAML_SEQUENCE_END_EVENT:   return "SEQUENCE_END";
    case YAML_MAPPING_START_EVENT:  return "MAPPING_START";
    case YAML_MAPPING_END_EVENT:    return "MAPPING_END";
    }
    return "UNKNOWN";
}

void
yaml_die(const char *path, yaml_parser_t *parser, const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "%s", path);
    if (parser) {
        fprintf(stderr, ":%lu:%lu",
                (unsigned long)parser->problem_mark.line + 1,
                (unsigned long)parser->problem_mark.column + 1);
    }
    fprintf(stderr, ": ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    if (parser && parser->problem) {
        fprintf(stderr, "  libyaml: %s\n", parser->problem);
    }
    boot_exit(1);
    /* unreachable, but silences noreturn analysis */
    abort();
}

void
yaml_next(const char *path, yaml_parser_t *parser, yaml_event_t *event)
{
    if (!yaml_parser_parse(parser, event)) {
        yaml_die(path, parser, "parse failure");
    }
}

yaml_event_t
yaml_expect(const char *path, yaml_parser_t *parser,
            yaml_event_type_t type, const char *context)
{
    yaml_event_t ev;
    yaml_next(path, parser, &ev);
    if (ev.type != type) {
        yaml_die(path, parser,
                 "expected %s while parsing %s, got %s",
                 event_type_name(type), context, event_type_name(ev.type));
    }
    return ev;
}

bool
yaml_scalar_to_bool(const char *path, yaml_event_t *ev)
{
    const char *v = (const char *)ev->data.scalar.value;
    if (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || !strcmp(v, "1")) {
        return true;
    }
    if (!strcasecmp(v, "false") || !strcasecmp(v, "no") || !strcmp(v, "0")) {
        return false;
    }
    yaml_die(path, NULL, "expected boolean, got '%s'", v);
}

int
yaml_scalar_to_int(const char *path, yaml_event_t *ev)
{
    char *end;
    const char *v = (const char *)ev->data.scalar.value;
    long n = strtol(v, &end, 10);
    if (*end || end == v) {
        yaml_die(path, NULL, "expected integer, got '%s'", v);
    }
    return (int)n;
}

int
yaml_scalar_to_level(const char *path, yaml_event_t *ev)
{
    const char *v = (const char *)ev->data.scalar.value;
    for (int i = 0; i < NUM_LEVELS; ++i) {
        if (!strcasecmp(v, user_level[i].name)) {
            return i;
        }
    }
    if (!strcasecmp(v, "NONE")) {
        return NUM_LEVELS;
    }
    yaml_die(path, NULL, "unknown level '%s'", v);
}
