
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Set in and out phrases
 */
void
set_iophrase(UR_OBJECT user, char *inpstr)
{
    if (strlen(inpstr) > PHRASE_LEN) {
        write_user(user, "Phrase too long.\n");
        return;
    }
    if (com_num == INPHRASE) {
        if (word_count < 2) {
            vwrite_user(user, "Your current in phrase is: %s\n", user->in_phrase);
            return;
        }
        strcpy(user->in_phrase, inpstr);
        write_user(user, "In phrase set.\n");
        return;
    }
    if (word_count < 2) {
        vwrite_user(user, "Your current out phrase is: %s\n", user->out_phrase);
        return;
    }
    strcpy(user->out_phrase, inpstr);
    write_user(user, "Out phrase set.\n");
}
