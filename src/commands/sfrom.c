#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Show list of people suggestions are from without seeing the whole lot
 */
void
suggestions_from(UR_OBJECT user)
{
    char id[ARR_SIZE], line[ARR_SIZE], filename[80], *s, *str;
    FILE *fp;
    int valid;
    int cnt;

    if (!amsys->suggestion_count) {
        write_user(user, "There are currently no suggestions.\n");
        return;
    }
    sprintf(filename, "%s/%s", MISCFILES, SUGBOARD);
    fp = fopen(filename, "r");
    if (!fp) {
        write_user(user,
                "There was an error trying to read the suggestion board.\n");
        write_syslog(SYSLOG, 0,
                "Unable to open suggestion board in suggestions_from().\n");
        return;
    }
    vwrite_user(user, "\n~BB*** Suggestions on the %s board from ***\n\n",
            SUGBOARD);
    valid = 1;
    cnt = 0;
    for (s = fgets(line, ARR_SIZE - 1, fp); s;
            s = fgets(line, ARR_SIZE - 1, fp)) {
        if (*s == '\n') {
            valid = 1;
        }
        sscanf(s, "%s", id);
        str = colour_com_strip(id);
        if (valid && !strcmp(str, "From:")) {
            vwrite_user(user, "~FC%2d)~RS %s", ++cnt, remove_first(s));
            valid = 0;
        }
    }
    fclose(fp);
    vwrite_user(user, "\nTotal of ~OL%d~RS suggestions.\n\n",
            amsys->suggestion_count);
}
