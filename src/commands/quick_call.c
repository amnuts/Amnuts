
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * set a name for a quick call
 */
void
quick_call(UR_OBJECT user)
{
#if !!0
    static const char usage[] = "Usage: call [<user>]\n";
#endif
    UR_OBJECT u;

    if (word_count < 2) {
        if (!*user->call) {
            write_user(user, "Quick call not set.\n");
            return;
        }
        vwrite_user(user, "Quick call to: %s.\n", user->call);
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        write_user(user, "You cannot set your quick call to yourself.\n");
        return;
    }
    strcpy(user->call, u->name);
    *user->call = toupper(*user->call);
    vwrite_user(user, "You have set a quick call to: %s.\n", user->call);
}
