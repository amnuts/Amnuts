
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"
#ifndef __SDS_H
#include "../vendors/sds/sds.h"
#endif

/*
 * Show a picture to the whole room that the user is in
 */
void
picture_all(UR_OBJECT user)
{
    static const char usage[] = "Usage: picture <picture>\n";
    sds filename;
    const char *name;
    UR_OBJECT u;
    FILE *fp;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot send pictures.\n");
        return;
    }
    if (word_count < 2) {
        write_user(user, usage);
        return;
    }
    if (strpbrk(word[1], "./")) {
        write_user(user, "Sorry, there is no picture with that name.\n");
        return;
    }
    filename = sdscatfmt(sdsempty(), "%s/%s", PICTFILES, word[1]);
    fp = fopen(filename, "r");
    if (!fp) {
        write_user(user, "Sorry, there is no picture with that name.\n");
        sdsfree(filename);
        return;
    }
    fclose(fp);
    for (u = user_first; u; u = u->next) {
        if (u->login || !u->room || (u->room != user->room && user->room)
                || (u->ignall && !force_listen)
                || u->ignpics || u == user) {
            continue;
        }
        if (check_igusers(u, user) && user->level < GOD) {
            continue;
        }
        name = user->vis || u->level > user->level ? user->recap : invisname;
        if (u->type == CLONE_TYPE) {
            if (u->clone_hear == CLONE_HEAR_NOTHING || u->owner->ignall
                    || u->clone_hear == CLONE_HEAR_SWEARS) {
                continue;
            }
            /*
             * Ignore anything not in clones room, eg shouts, system messages
             * and shemotes since the clones owner will hear them anyway.
             */
            if (user->room != u->room) {
                continue;
            }
            vwrite_user(u->owner,
                    "~FC[ %s ]:~RS %s~RS ~OL~FGshows the following picture...\n\n",
                    u->room->name, name);
            switch (more(u, u->socket, filename)) {
            case 0:
                break;
            case 1:
                u->misc_op = 2;
                break;
            }
        } else {
            vwrite_user(u, "%s~RS ~OL~FGshows the following picture...\n\n", name);
            switch (more(u, u->socket, filename)) {
            case 0:
                break;
            case 1:
                u->misc_op = 2;
                break;
            }
        }
    }
    write_user(user, "You ~OL~FGshow the following picture to the room...\n\n");
    switch (more(user, user->socket, filename)) {
    case 0:
        break;
    case 1:
        user->misc_op = 2;
        break;
    }
    sdsfree(filename);
}
