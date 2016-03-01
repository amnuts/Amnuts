
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Display user macros
 */
void
macros(UR_OBJECT user)
{
    int i;

#ifdef NETLINKS
    if (user->type == REMOTE_TYPE) {
        write_user(user,
                "Due to software limitations, remote users cannot have macros.\n");
        return;
    }
#endif
    write_user(user, "Your current macros:\n");
    for (i = 0; i < 10; ++i) {
        vwrite_user(user, "  ~OL%d)~RS %s\n", i, user->macros[i]);
    }
}