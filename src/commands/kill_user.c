
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Kill a user - bump them off the talker
 */
void
kill_user(UR_OBJECT user)
{

    struct kill_mesgs_struct {
        const char *victim_msg;
        const char *room_msg;
    };
    static const struct kill_mesgs_struct kill_mesgs[] = {
        {"You are killed\n", "%s is killed\n"},
        {"You have been totally splatted\n", "A hammer splats %s\n"},
        {"The Hedgehog Of Doom runs over you with a car.\n",
            "The Hedgehog Of Doom runs over %s with a car.\n"},
        {"The Inquisitor deletes the worthless, prunes away the wastrels... ie, YOU!", "The Inquisitor prunes away %s.\n"},
        {NULL, NULL},
    };
    UR_OBJECT victim;
    const char *name;
    int msg;

    if (word_count < 2) {
        write_user(user, "Usage: kill <user>\n");
        return;
    }
    victim = get_user_name(user, word[1]);
    if (!victim) {
        write_user(user, notloggedon);
        return;
    }
    if (user == victim) {
        write_user(user,
                "Trying to commit suicide this way is the sixth sign of madness.\n");
        return;
    }
    if (victim->level >= user->level) {
        write_user(user,
                "You cannot kill a user of equal or higher level than yourself.\n");
        vwrite_user(victim, "%s~RS tried to kill you!\n", user->recap);
        return;
    }
    write_syslog(SYSLOG, 1, "%s KILLED %s.\n", user->name, victim->name);
    write_user(user, "~FM~OLYou chant an evil incantation...\n");
    name = user->vis ? user->bw_recap : invisname;
    vwrite_room_except(user->room, user,
            "~FM~OL%s chants an evil incantation...\n", name);
    /*
       display random kill message.  if you only want one message to be displayed
       then only have one message listed in kill_mesgs[].
     */
    for (msg = 0; kill_mesgs[msg].victim_msg; ++msg) {
        ;
    }
    if (msg) {
        msg = rand() % msg;
        write_user(victim, kill_mesgs[msg].victim_msg);
        vwrite_room_except(victim->room, victim, kill_mesgs[msg].room_msg,
                victim->bw_recap);
    }
    sprintf(text, "~FRKilled~RS by %s.\n", user->name);
    add_history(victim->name, 1, "%s", text);
    disconnect_user(victim);
    write_monitor(user, NULL, 0);
    write_room(NULL,
            "~FM~OLYou hear insane laughter from beyond the grave...\n");
}
