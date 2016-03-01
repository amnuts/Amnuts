
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * displays what the user is currently listening to/ignoring
 */
void
show_ignlist(UR_OBJECT user)
{
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    write_user(user,
            "| ~OL~FCYour ignore states are as follows~RS                                          |\n");
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    if (user->ignall) {
        write_user(user,
                "| You are currently ignoring ~OL~FReverything~RS                                      |\n");
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        return;
    }
    vwrite_user(user,
            "| Ignoring shouts   : ~OL%-3s~RS    Ignoring tells  : ~OL%-3s~RS    Ignoring logons : ~OL%-3s~RS  |\n",
            noyes[user->ignshouts], noyes[user->igntells],
            noyes[user->ignlogons]);
    vwrite_user(user,
            "| Ignoring pictures : ~OL%-3s~RS    Ignoring greets : ~OL%-3s~RS    Ignoring beeps  : ~OL%-3s~RS  |\n",
            noyes[user->ignpics], noyes[user->igngreets],
            noyes[user->ignbeeps]);
    if (user->level >= (enum lvl_value) command_table[IGNWIZ].level) {
        vwrite_user(user,
                "| Ignoring wiztells : ~OL%-3s~RS                                                    |\n",
                noyes[user->ignwiz]);
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n\n");
}
