#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

#ifdef IDENTD

/*
 * For Ident, by Ardant
 */
void
resite(UR_OBJECT user)
{
    char buffer[255];
    UR_OBJECT u;

    if (word_count < 2) {
        write_user(user, "Usage: resite <user>|-a\n");
        return;
    }
    if (!amsys->ident_state) {
        write_user(user, "The ident daemon is not active.\n");
        return;
    }
    if (!strcmp(word[1], "-a")) {
        for (u = user_first; u; u = u->next) {
            sprintf(buffer, "SITE: %s\n", u->ipsite);
            write_sock(amsys->ident_socket, buffer);
#ifdef WIZPORT
            sprintf(buffer, "AUTH: %d %s %s %s\n", u->socket, u->site_port,
                    !user->wizport ? amsys->mport_port : amsys->wport_port,
                    u->ipsite);
#else
            sprintf(buffer, "AUTH: %d %s %s %s\n", u->socket, u->site_port,
                    amsys->mport_port, u->ipsite);
#endif
            write_sock(amsys->ident_socket, buffer);
        }
        write_user(user, "Refreshed site lookup of all users.\n");
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    sprintf(buffer, "SITE: %s\n", u->ipsite);
    write_sock(amsys->ident_socket, buffer);
#ifdef WIZPORT
    sprintf(buffer, "AUTH: %d %s %s %s\n", u->socket, u->site_port,
            !user->wizport ? amsys->mport_port : amsys->wport_port, u->ipsite);
#else
    sprintf(buffer, "AUTH: %d %s %s %s\n", u->socket, u->site_port,
            amsys->mport_port, u->ipsite);
#endif
    write_sock(amsys->ident_socket, buffer);
    sprintf(text, "Refreshed site lookup for \"%s\".\n", u->name);
    write_user(user, text);
}

#endif