/****************************************************************************
   Amnuts localisation — tiny libyaml helpers used by catalog.c and any
   future YAML loaders. Centralises error reporting (single
   file:line:col: message format) and event walking so the catalog loader
   stays readable.

   Function declarations live in src/includes/prototypes.h (single source
   of truth, per project convention).
 ***************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "globals.h"
#include "prototypes.h"

#include "../vendors/libyaml/yaml.h"

void
yaml_die(const char *path, void *parser_v, const char *fmt, ...)
{
    va_list ap;
    yaml_parser_t *parser = (yaml_parser_t *) parser_v;

    fprintf(stderr, "%s", path);
    if (parser) {
        fprintf(stderr, ":%lu:%lu",
                (unsigned long) parser->problem_mark.line + 1,
                (unsigned long) parser->problem_mark.column + 1);
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
}

void
yaml_next(const char *path, void *parser_v, void *event_out_v)
{
    yaml_parser_t *parser = (yaml_parser_t *) parser_v;
    yaml_event_t  *event  = (yaml_event_t  *) event_out_v;
    if (!yaml_parser_parse(parser, event)) {
        yaml_die(path, parser, "YAML parse failure");
    }
}

const char *
yaml_event_kind(int event_type)
{
    switch ((yaml_event_type_t) event_type) {
    case YAML_NO_EVENT:             return "none";
    case YAML_STREAM_START_EVENT:   return "stream-start";
    case YAML_STREAM_END_EVENT:     return "stream-end";
    case YAML_DOCUMENT_START_EVENT: return "document-start";
    case YAML_DOCUMENT_END_EVENT:   return "document-end";
    case YAML_ALIAS_EVENT:          return "alias";
    case YAML_SCALAR_EVENT:         return "scalar";
    case YAML_SEQUENCE_START_EVENT: return "sequence-start";
    case YAML_SEQUENCE_END_EVENT:   return "sequence-end";
    case YAML_MAPPING_START_EVENT:  return "mapping-start";
    case YAML_MAPPING_END_EVENT:    return "mapping-end";
    }
    return "unknown";
}
