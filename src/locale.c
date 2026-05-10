/****************************************************************************
   Amnuts localisation — file-path resolver and locale discovery.
   See docs/superpowers/specs/2026-05-10-localisation-design.md

   Function declarations live in src/includes/prototypes.h (single source
   of truth, per project convention).
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "defines.h"
#include "globals.h"
#include "prototypes.h"

/* Return the configured default-locale name (NUL-terminated, owned). */
const char *
locale_default(void)
{
    return amsys->default_locale;
}

static int
file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

/* Same as locale_path, but always resolves against the server default
 * locale. Use for boot-time loaders, login banner display, and any
 * "describes the world, not a person" lookup. */
int
locale_default_path(char *out, size_t outlen,
                    const char *category, const char *name)
{
    int n;
    n = snprintf(out, outlen, "%s/%s/%s/%s",
                 LANGS_ROOT, amsys->default_locale, category, name);
    if (n < 0 || (size_t) n >= outlen) {
        /* truncation — caller still gets a NUL-terminated string */
        return 0;
    }
    return file_exists(out) ? 1 : 0;
}

/* Look up a category file in user's locale, falling back to the server
 * default. Writes the resolved path to `out`. Returns:
 *   2  found in user's locale
 *   1  found in default locale (fallback used)
 *   0  not found in either; `out` populated with the default-locale path
 *
 * `category` is a bare category name (e.g. "helpfiles"). `name` is the
 * filename within that category. The composed path is
 * "<LANGS_ROOT>/<locale>/<category>/<name>". */
int
locale_path(UR_OBJECT user, char *out, size_t outlen,
            const char *category, const char *name)
{
    int n;
    /* Try user's locale if they have one set. */
    if (user && user->locale[0]) {
        n = snprintf(out, outlen, "%s/%s/%s/%s",
                     LANGS_ROOT, user->locale, category, name);
        if (n > 0 && (size_t) n < outlen && file_exists(out)) {
            return 2;
        }
    }
    /* Fall back to default locale. */
    n = snprintf(out, outlen, "%s/%s/%s/%s",
                 LANGS_ROOT, amsys->default_locale, category, name);
    if (n < 0 || (size_t) n >= outlen) return 0;
    return file_exists(out) ? 1 : 0;
}

static int
is_valid_locale_dirname(const char *name)
{
    size_t len = strlen(name);
    if (!len || len >= LOCALE_NAME_LEN) return 0;
    if (*name == '.') return 0;
    for (const char *p = name; *p; ++p) {
        if (*p == '/' || *p == '\\') return 0;
    }
    return 1;
}

/* Discover locales under files/langs/ and validate the default. Called
 * once at boot, after the config file has been parsed (so default_locale
 * is known). Returns 0 on success; nonzero values are reserved for future
 * use (in Phase 1, any failure is fatal and reported via boot_exit). */
int
locale_load_all(void)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat st;
    char path[1024];
    int default_seen = 0;

    amsys->locales.count = 0;

    dirp = opendir(LANGS_ROOT);
    if (!dirp) {
        fprintf(stderr, "Amnuts: cannot open %s — install is missing the localisation tree.\n",
                LANGS_ROOT);
        boot_exit(1);
    }
    for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
        if (!is_valid_locale_dirname(dp->d_name)) continue;
        snprintf(path, sizeof path, "%s/%s", LANGS_ROOT, dp->d_name);
        if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) continue;

        /* Track default-seen even if the cap is hit (otherwise the 65th-listed
         * default would be falsely reported as missing). */
        if (strcmp(dp->d_name, amsys->default_locale) == 0) {
            default_seen = 1;
        }

        if (amsys->locales.count >= MAX_LOCALES) {
            fprintf(stderr, "Amnuts: more than %d locales discovered — ignoring %s.\n",
                    MAX_LOCALES, dp->d_name);
            continue;
        }
        strncpy(amsys->locales.names[amsys->locales.count],
                dp->d_name, LOCALE_NAME_LEN - 1);
        amsys->locales.names[amsys->locales.count][LOCALE_NAME_LEN - 1] = '\0';
        amsys->locales.count++;
    }
    closedir(dirp);

    if (!default_seen) {
        fprintf(stderr,
                "Amnuts: default_language '%s' has no directory under %s.\n",
                amsys->default_locale, LANGS_ROOT);
        boot_exit(1);
    }
    printf("Localisation: discovered %d locale(s); default = %s.\n",
           amsys->locales.count, amsys->default_locale);
    return 0;
}
