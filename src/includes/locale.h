/****************************************************************************
   Amnuts localisation — file-path resolver and locale discovery.
   See docs/superpowers/specs/2026-05-10-localisation-design.md
 ***************************************************************************/

#ifndef AMNUTS_LOCALE_H
#define AMNUTS_LOCALE_H

#include <stddef.h>

/* Opaque to callers; defined in locale.c */
struct locale_catalog;
struct locale_state;

/* Forward decl — UR_OBJECT is a pointer typedef declared in globals.h */
struct user_struct;

/* Discover locales under files/langs/ and validate the default. Called
 * once at boot, after the config file has been parsed (so default_locale
 * is known). Returns 0 on success; nonzero values are reserved for future
 * use (in Phase 1, any failure is fatal and reported via boot_exit). */
int locale_load_all(void);

/* Look up a category file in user's locale, falling back to the server
 * default. Writes the resolved path to `out`. Returns:
 *   2  found in user's locale
 *   1  found in default locale (fallback used)
 *   0  not found in either; `out` populated with the default-locale path
 *
 * `category` is a bare category name (e.g. "helpfiles"). `name` is the
 * filename within that category. The composed path is
 * "<LANGS_ROOT>/<locale>/<category>/<name>". */
int locale_path(struct user_struct *user, char *out, size_t outlen,
                const char *category, const char *name);

/* Same, but always resolves against the server default locale. Use for
 * boot-time loaders, login banner display, and any "describes the world,
 * not a person" lookup. */
int locale_default_path(char *out, size_t outlen,
                        const char *category, const char *name);

/* Return the configured default-locale name (NUL-terminated, owned). */
const char *locale_default(void);

#endif /* AMNUTS_LOCALE_H */
