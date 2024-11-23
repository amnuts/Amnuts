#include "../includes/defines.h"
#include "../includes/globals.h"
#include "../includes/commands.h"
#include "../includes/prototypes.h"

/*
 * Clear the tell buffer of the user
 */
void
clear_afk(UR_OBJECT user)
{
    destruct_review_buffer_type(user, rbfAFK, 0);
    write_user(user, "Your AFK review buffer has now been cleared.\n");
}
