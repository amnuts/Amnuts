
#include "../includes/defines.h"
#include "../includes/globals.h"
#include "../includes/commands.h"
#include "../includes/prototypes.h"

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
