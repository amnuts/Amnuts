/****************************************************************************
         Amnuts version 2.3.0 - Copyright (C) Andrew Collington, 2003
                      Last update: 2003-08-04

                              amnuts@talker.com
                          http://amnuts.talker.com/

                                   based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/***************************************************************************/

/*
 * review from the user review buffer
 */
int
review_buffer(UR_OBJECT user, unsigned flags)
{
    int count = 0;
    RB_OBJECT rb, next;

    if (user->reverse_buffer) {
        for (rb = user->rb_last; rb; rb = next) {
            next = rb->prev;
            if (rb->flags & flags) {
                write_user(user, rb->buffer);
                ++count;
            }
        }
    } else {
        for (rb = user->rb_first; rb; rb = next) {
            next = rb->next;
            if (rb->flags & flags) {
                write_user(user, rb->buffer);
                ++count;
            }
        }
    }
    return count;
}

/*
 * Record speech and emotes in the room.
 */
void
record(RM_OBJECT rm, char *str)
{
    *rm->revbuff[rm->revline] = '\0';
    strncat(rm->revbuff[rm->revline], str, REVIEW_LEN);
    rm->revbuff[rm->revline][REVIEW_LEN] = '\n';
    rm->revbuff[rm->revline][REVIEW_LEN + 1] = '\0';
    rm->revline = (rm->revline + 1) % REVIEW_LINES;
}

/*
 * Records shouts and shemotes sent over the talker.
 */
void
record_shout(const char *str)
{
    *amsys->shoutbuff[amsys->sbuffline] = '\0';
    strncat(amsys->shoutbuff[amsys->sbuffline], str, REVIEW_LEN);
    amsys->shoutbuff[amsys->sbuffline][REVIEW_LEN] = '\n';
    amsys->shoutbuff[amsys->sbuffline][REVIEW_LEN + 1] = '\0';
    amsys->sbuffline = (amsys->sbuffline + 1) % REVIEW_LINES;
}

/*
 * Records tells and pemotes sent to the user.
 */
void
record_tell(UR_OBJECT from, UR_OBJECT to, const char *str)
{
    int count;

    if (!create_review_buffer_entry(to, !from ? "?" : from->name, str, rbfTELL)) {
        write_syslog(ERRLOG, 1,
                "Could not create tell review buffer entry for %s.\n",
                to->name);
        return;
    }
    /* check to see if we need to prune */
    count = has_review(to, rbfTELL);
    if (count > REVTELL_LINES) {
        destruct_review_buffer_type(to, rbfTELL, 1);
    }
}

/*
 * Records tells and pemotes sent to the user when afk.
 */
void
record_afk(UR_OBJECT from, UR_OBJECT to, const char *str)
{
    int count;

    if (!create_review_buffer_entry(to, !from ? "?" : from->name, str, rbfAFK)) {
        write_syslog(ERRLOG, 1,
                "Could not create afk review buffer entry for %s.\n",
                to->name);
        return;
    }
    /* check to see if we need to prune */
    count = has_review(to, rbfAFK);
    if (count > REVTELL_LINES) {
        destruct_review_buffer_type(to, rbfAFK, 1);
    }
}

/*
 * Records tells and pemotes sent to the user when in the line editor.
 */
void
record_edit(UR_OBJECT from, UR_OBJECT to, const char *str)
{
    int count;

    if (!create_review_buffer_entry(to, !from ? "?" : from->name, str, rbfEDIT)) {
        write_syslog(ERRLOG, 1,
                "Could not create edit review buffer entry for %s.\n",
                to->name);
        return;
    }
    /* check to see if we need to prune */
    count = has_review(to, rbfEDIT);
    if (count > REVTELL_LINES) {
        destruct_review_buffer_type(to, rbfEDIT, 1);
    }
}

/*
 * Clear the review buffer in the room
 */
void
clear_revbuff(RM_OBJECT rm)
{
    int i;

    for (i = 0; i < REVIEW_LINES; ++i) {
        *rm->revbuff[i] = '\0';
    }
    rm->revline = 0;
}

/*
 * count up how many review buffers of a certain type
 */
int
has_review(UR_OBJECT user, unsigned flags)
{
    int count = 0;
    RB_OBJECT rb;

    for (rb = user->rb_first; rb; rb = rb->next) {
        if (rb->flags & flags) {
            ++count;
        }
    }
    return count;
}
