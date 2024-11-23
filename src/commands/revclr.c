
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Clear the review buffer
 */
void
revclr(UR_OBJECT user)
{
#if !!0
    static const char usage[] = "Usage: cbuff\n";
#endif
    const char *name;

    clear_revbuff(user->room);
    name = user->vis ? user->recap : invisname;
    vwrite_room_except(user->room, user, "%s~RS clears the review buffer.\n",
            name);
    write_user(user, "You clear the review buffer.\n");
}
