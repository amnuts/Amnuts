#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Clear the tell buffer of the user
 */
void
clear_afk(UR_OBJECT user)
{
    destruct_review_buffer_type(user, rbfAFK, 0);
    write_user(user, "Your AFK review buffer has now been cleared.\n");
}
