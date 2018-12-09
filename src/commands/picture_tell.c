
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Displays a picture to a person
 */
void
picture_tell(UR_OBJECT user)
{
    static const char usage[] = "Usage: ptell <user> <picture>\n";
    sds filename;
    const char *name;
    UR_OBJECT u;
    FILE *fp;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot send pictures.\n");
        return;
    }
    if (word_count < 3) {
        write_user(user, usage);
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        write_user(user, "There is an easier way to see pictures...\n");
        return;
    }
    if (check_igusers(u, user) && user->level < GOD) {
        vwrite_user(user, "%s~RS is ignoring pictures from you.\n", u->recap);
        return;
    }
    if (u->ignpics && (user->level < WIZ || u->level > user->level)) {
        vwrite_user(user, "%s~RS is ignoring pictures at the moment.\n",
                u->recap);
        return;
    }
    if (u->afk) {
        if (*u->afk_mesg) {
            vwrite_user(user, "%s~RS is ~FRAFK~RS, message is: %s\n", u->recap,
                    u->afk_mesg);
        } else {
            vwrite_user(user, "%s~RS is ~FRAFK~RS at the moment.\n", u->recap);
        }
        return;
    }
    if (u->malloc_start) {
        vwrite_user(user, "%s~RS is writing a message at the moment.\n",
                u->recap);
        return;
    }
    if (u->ignall && (user->level < WIZ || u->level > user->level)) {
        vwrite_user(user, "%s~RS is not listening at the moment.\n", u->recap);
        return;
    }
#ifdef NETLINKS
    if (!u->room) {
        vwrite_user(user,
                "%s~RS is offsite and would not be able to reply to you.\n",
                u->recap);
        return;
    }
#endif
    if (strpbrk(word[2], "./")) {
        write_user(user, "Sorry, there is no picture with that name.\n");
        return;
    }
    filename = sdscatfmt(sdsempty(), "%s/%s", PICTFILES, word[2]);
    fp = fopen(filename, "r");
    if (!fp) {
        write_user(user, "Sorry, there is no picture with that name.\n");
        return;
    }
    fclose(fp);
    name = user->vis || u->level >= user->level ? user->recap : invisname;
    vwrite_user(u, "%s~RS ~OL~FGshows you the following picture...\n\n", name);
    switch (more(u, u->socket, filename)) {
    case 0:
        break;
    case 1:
        u->misc_op = 2;
        break;
    }
    vwrite_user(user, "You ~OL~FGshow the following picture to~RS %s\n\n",
            u->recap);
    switch (more(user, user->socket, filename)) {
    case 0:
        break;
    case 1:
        user->misc_op = 2;
        break;
    }
    sdsfree(filename);
}
