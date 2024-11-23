
#include "../includes/defines.h"
#include "../includes/globals.h"
#include "../includes/commands.h"
#include "../includes/prototypes.h"

/*
 * Clear the tell buffer of the user
 */
void
clear_tells(UR_OBJECT user)
{
    destruct_review_buffer_type(user, rbfTELL, 0);
}
