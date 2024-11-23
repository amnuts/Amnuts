#include "../includes/defines.h"
#include "../includes/globals.h"
#include "../includes/commands.h"
#include "../includes/prototypes.h"

/*
 * Clear the tell buffer of the user
 */
void
clear_edit(UR_OBJECT user)
{
    destruct_review_buffer_type(user, rbfEDIT, 0);
}