#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Clear the tell buffer of the user
 */
void
clear_edit(UR_OBJECT user)
{
    destruct_review_buffer_type(user, rbfEDIT, 0);
}