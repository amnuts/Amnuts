/****************************************************************************
   Amnuts localisation — file-path resolver and locale discovery.
   See docs/superpowers/specs/2026-05-10-localisation-design.md
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "defines.h"
#include "globals.h"
#include "locale.h"

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
