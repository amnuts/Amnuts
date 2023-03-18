
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Set user visible or invisible
 */
void
visibility(UR_OBJECT user, int vis)
{
    if (vis) {
        if (user->vis) {
            write_user(user, "You are already visible.\n");
            return;
        }
        write_user(user,
                "~FB~OLYou recite a melodic incantation and reappear.\n");
        vwrite_room_except(user->room, user,
                "~FB~OLYou hear a melodic incantation chanted and %s materialises!\n",
                user->bw_recap);
        user->vis = 1;
        return;
    }
    if (!user->vis) {
        write_user(user, "You are already invisible.\n");
        return;
    }
    write_user(user, "~FB~OLYou recite a melodic incantation and fade out.\n");
    vwrite_room_except(user->room, user,
            "~FB~OL%s recites a melodic incantation and disappears!\n",
            user->bw_recap);
    user->vis = 0;
}