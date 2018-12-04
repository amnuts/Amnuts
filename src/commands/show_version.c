
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Show version number and some small stats of the talker
 */
void
show_version(UR_OBJECT user)
{
    int rms;
    RM_OBJECT rm;

    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    write_user(user, align_string(1, 78, 1, "|", "~OL%s~RS", TALKER_NAME));
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    vwrite_user(user,
            "| Logons this current boot : ~OL%4d~RS new users, ~OL%4d~RS old users~RS                  |\n",
            amsys->logons_new, amsys->logons_old);
    vwrite_user(user,
            "| Total number of users    : ~OL%-4d~RS  Maximum online users     : ~OL%-3d~RS            |\n",
            amsys->user_count, amsys->max_users);
    rms = 0;
    for (rm = room_first; rm; rm = rm->next) {
        ++rms;
    }
    vwrite_user(user,
            "| Total number of rooms    : ~OL%-3d~RS   Swear ban currently on   : ~OL%s~RS            |\n",
            rms, minmax[amsys->ban_swearing]);
    vwrite_user(user,
            "| Smail auto-forwarding on : ~OL%-3s~RS   Auto purge on            : ~OL%-3s~RS            |\n",
            noyes[amsys->forwarding], noyes[amsys->auto_purge_date != -1]);
    vwrite_user(user,
            "| Maximum smail copies     : ~OL%-3d~RS   Names can be recapped    : ~OL%-3s~RS            |\n",
            MAX_COPIES, noyes[amsys->allow_recaps]);
    vwrite_user(user,
            "| Personal rooms active    : ~OL%-3s~RS   Maximum user idle time   : ~OL%-3d~RS mins~RS       |\n",
            noyes[amsys->personal_rooms], amsys->user_idle_time / 60);
    if (user->level >= WIZ) {
#ifdef NETLINKS
        write_user(user,
                "| Compiled netlinks        : ~OLYES~RS                                             |\n");
#else
        write_user(user,
                "| Compiled netlinks        : ~OLNO~RS                                              |\n");
#endif
    }
    /* YOU MUST *NOT* ALTER THE REMAINING LINES */
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    vwrite_user(user,
            "| ~FC~OLAmnuts version %-21.21s (C) Andrew Collington, September 2001~RS |\n",
            AMNUTSVER);
    vwrite_user(user,
            "| ~FC~OLBuilt %s %s~RS                                                 |\n",
            __DATE__, __TIME__);
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    write_user(user,
            "| Running Ardant's Ident Daemon (ArIdent) code, version 2.0.2                |\n");
    write_user(user,
            "| Running Ardant's Universal Pager code                                      |\n");
    write_user(user,
            "| Running Arny's Seamless reboot code (based on phypor's EWTOO sreboot)      |\n");
    write_user(user,
            "| Running Silver's spodlist code (converted from PG+ spodlist)               |\n");
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
}
