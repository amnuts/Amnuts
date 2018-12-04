
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * sets up a channel for the user to ignore
 */
void
set_ignore(UR_OBJECT user)
{
    switch (com_num) {
    case IGNTELLS:
        switch (user->igntells) {
        case 0:
            user->igntells = 1;
            write_user(user, "You will now ignore tells.\n");
            break;
        case 1:
            user->igntells = 0;
            write_user(user, "You will now hear tells.\n");
            break;
        }
        break;
    case IGNSHOUTS:
        switch (user->ignshouts) {
        case 0:
            user->ignshouts = 1;
            write_user(user, "You will now ignore shouts.\n");
            break;
        case 1:
            user->ignshouts = 0;
            write_user(user, "You will now hear shouts.\n");
            break;
        }
        break;
    case IGNPICS:
        switch (user->ignpics) {
        case 0:
            user->ignpics = 1;
            write_user(user, "You will now ignore pictures.\n");
            break;
        case 1:
            user->ignpics = 0;
            write_user(user, "You will now see pictures.\n");
            break;
        }
        break;
    case IGNWIZ:
        switch (user->ignwiz) {
        case 0:
            user->ignwiz = 1;
            write_user(user, "You will now ignore all wiztells and wizemotes.\n");
            break;
        case 1:
            user->ignwiz = 0;
            write_user(user,
                    "You will now listen to all wiztells and wizemotes.\n");
            break;
        }
        break;
    case IGNLOGONS:
        switch (user->ignlogons) {
        case 0:
            user->ignlogons = 1;
            write_user(user, "You will now ignore all logons and logoffs.\n");
            break;
        case 1:
            user->ignlogons = 0;
            write_user(user, "You will now see all logons and logoffs.\n");
            break;
        }
        break;
    case IGNGREETS:
        switch (user->igngreets) {
        case 0:
            user->igngreets = 1;
            write_user(user, "You will now ignore all greets.\n");
            break;
        case 1:
            user->igngreets = 0;
            write_user(user, "You will now see all greets.\n");
            break;
        }
        break;
    case IGNBEEPS:
        switch (user->ignbeeps) {
        case 0:
            user->ignbeeps = 1;
            write_user(user, "You will now ignore all beeps from users.\n");
            break;
        case 1:
            user->ignbeeps = 0;
            write_user(user, "You will now hear all beeps from users.\n");
            break;
        }
        break;
    default:
        break;
    }
}