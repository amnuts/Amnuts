
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Free a hung socket
 */
void
clearline(UR_OBJECT user)
{
    UR_OBJECT u;
    int sock;

    if (word_count < 2 || !is_number(word[1])) {
        write_user(user, "Usage: clearline <line>\n");
        return;
    }
    sock = atoi(word[1]);
    /* Find line amongst users */
    for (u = user_first; u; u = u->next) {
        if (u->type != CLONE_TYPE && u->socket == sock) {
            break;
        }
    }
    if (!u) {
        write_user(user, "That line is not currently active.\n");
        return;
    }
    if (!u->login) {
        write_user(user, "You cannot clear the line of a logged in user.\n");
        return;
    }
    write_user(u, "\n\nThis line is being cleared.\n\n");
    disconnect_user(u);
    write_syslog(SYSLOG, 1, "%s cleared line %d.\n", user->name, sock);
    vwrite_user(user, "Line %d cleared.\n", sock);
    destructed = 0;
    no_prompt = 0;
}