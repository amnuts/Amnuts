/****************************************************************************
             Amnuts - Copyright (C) Andrew Collington, 1996-2026

                   talker@amnuts.net - https://amnuts.net/

                                 based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Reload all language catalogs from disk. Atomic swap; in-flight callers
 * read from the previous generation, held one cycle.
 */
void
langreload(UR_OBJECT user)
{
    if (catalog_reload_all() != 0) {
        write_user(user, "~FRLanguage reload failed.~RS Check the syslog.\n");
        write_syslog(SYSLOG | ERRLOG, 1,
                     "[locale] langreload requested by %s but failed.\n",
                     user->name);
        return;
    }
    vwrite_user(user,
                "~FGLanguage reload complete:~RS %d locale(s) loaded; default = ~OL%s~RS.\n",
                amsys->locales.count, amsys->default_locale);
    write_syslog(SYSLOG, 1, "%s ran langreload.\n", user->name);
}
