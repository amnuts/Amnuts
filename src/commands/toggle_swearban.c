
#include "../includes/defines.h"
#include "../includes/globals.h"
#include "../includes/commands.h"
#include "../includes/prototypes.h"

/*
 * Switch swearing ban on and off
 */
void
toggle_swearban(UR_OBJECT user)
{
    switch (amsys->ban_swearing) {
    case SBOFF:
        write_user(user, "Swearing ban now set to ~FGminimum ban~RS.\n");
        amsys->ban_swearing = SBMIN;
        write_syslog(SYSLOG, 1, "%s set swearing ban to MIN.\n", user->name);
        break;
    case SBMIN:
        write_user(user, "Swearing ban now set to ~FRmaximum ban~RS.\n");
        amsys->ban_swearing = SBMAX;
        write_syslog(SYSLOG, 1, "%s set swearing ban to MAX.\n", user->name);
        break;
    case SBMAX:
        write_user(user, "Swearing ban now set to ~FYoff~RS.\n");
        amsys->ban_swearing = SBOFF;
        write_syslog(SYSLOG, 1, "%s set swearing ban to OFF.\n", user->name);
        break;
    }
}