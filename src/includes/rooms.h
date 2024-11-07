/****************************************************************************
             Amnuts - Copyright (C) Andrew Collington, 1996-2023
                        Last update: Sometime in 2023

                   talker@amnuts.net - https://amnuts.net/

                                 based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996

 ***************************************************************************/

#ifndef AMNUTS_ROOMS_H
#define AMNUTS_ROOMS_H

struct priv_room_struct {
    const char *name;
    enum lvl_value level;
};

/*
 * The rooms listed here are just examples of what can be added
 * You may add more or remove as many as you like, but you MUST
 * keep the stopping clause in
 */
static const struct priv_room_struct priv_room[] = {
    {"wizroom", WIZ}, /* a room for wizzes+ only */
    {"andys_computer", GOD}, /* only top people can get in this place! */
    {NULL, NUM_LEVELS} /* stopping clause */
};

#endif
