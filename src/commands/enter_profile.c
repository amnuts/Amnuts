
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Enter user profile
 */
void
enter_profile(UR_OBJECT user, char *inpstr)
{
    char filename[80];
    char *c;
    FILE *fp;

    if (inpstr) {
        if (word_count < 2) {
            write_user(user, "\n~BB*** Writing profile ***\n\n");
            user->misc_op = 5;
            editor(user, NULL);
            return;
        }
        strcat(inpstr, "\n");
    } else {
        inpstr = user->malloc_start;
    }
    sprintf(filename, "%s/%s/%s.P", USERFILES, USERPROFILES, user->name);
    fp = fopen(filename, "w");
    if (!fp) {
        vwrite_user(user, "%s: cannot save your profile.\n", syserror);
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Cannot open file %s to write in enter_profile().\n",
                filename);
        return;
    }
    for (c = inpstr; *c; ++c) {
        putc(*c, fp);
    }
    fclose(fp);
    write_user(user, "Profile stored.\n");
}
