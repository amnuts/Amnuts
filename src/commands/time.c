
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Get current system time
 */
void
get_time(UR_OBJECT user)
{
    char dstr[32], temp[80];
    time_t now;
    int secs, mins, hours, days;

    /* Get some values */
    time(&now);
    secs = (int) (now - amsys->boot_time);
    days = secs / 86400;
    hours = (secs % 86400) / 3600;
    mins = (secs % 3600) / 60;
    secs = secs % 60;
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    write_user(user,
            "| ~OL~FCTalker times~RS                                                               |\n");
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    strftime(dstr, 32, "%a %Y-%m-%d %H:%M:%S", localtime(&now));
    vwrite_user(user, "| The current system time is : ~OL%-45s~RS |\n", dstr);
    strftime(dstr, 32, "%a %Y-%m-%d %H:%M:%S", localtime(&amsys->boot_time));
    vwrite_user(user, "| System booted              : ~OL%-45s~RS |\n", dstr);
    sprintf(temp, "%d day%s, %d hour%s, %d minute%s, %d second%s", days,
            PLTEXT_S(days), hours, PLTEXT_S(hours), mins, PLTEXT_S(mins), secs,
            PLTEXT_S(secs));
    vwrite_user(user, "| Uptime                     : ~OL%-45s~RS |\n", temp);
    write_user(user,
            "+----------------------------------------------------------------------------+\n\n");
}
