
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Display details of room
 */
void
look(UR_OBJECT user)
{
    char temp[81];
    RM_OBJECT rm;
    UR_OBJECT u;
    int i, exits, users;

    rm = user->room;

    if (is_private_room(rm)) {
        vwrite_user(user, "\n~FCRoom: ~FR%s\n\n",
                is_personal_room(rm) ? rm->show_name : rm->name);
    } else {
        vwrite_user(user, "\n~FCRoom: ~FG%s\n\n",
                is_personal_room(rm) ? rm->show_name : rm->name);
    }
    if (user->show_rdesc) {
        write_user(user, user->room->desc);
    }
    exits = 0;
    strcpy(text, "\n~FCExits are:");
    for (i = 0; i < MAX_LINKS; ++i) {
        if (!rm->link[i]) {
            break;
        }
        if (is_private_room(rm->link[i])) {
            sprintf(temp, "  ~FR%s", rm->link[i]->name);
        } else {
            sprintf(temp, "  ~FG%s", rm->link[i]->name);
        }
        strcat(text, temp);
        ++exits;
    }
#ifdef NETLINKS
    if (rm->netlink && rm->netlink->stage == UP) {
        if (rm->netlink->allow == IN) {
            sprintf(temp, "  ~FR%s*", rm->netlink->service);
        } else {
            sprintf(temp, "  ~FG%s*", rm->netlink->service);
        }
        strcat(text, temp);
    } else
#endif
        if (!exits) {
        strcpy(text, "\n~FCThere are no exits.");
    }
    strcat(text, "\n\n");
    write_user(user, text);
    users = 0;
    for (u = user_first; u; u = u->next) {
        if (u->room != rm || u == user || (!u->vis && u->level > user->level)) {
            continue;
        }
        if (!users++) {
            write_user(user, "~FG~OLYou can see:\n");
        }
        vwrite_user(user, "     %s%s~RS %s~RS  %s\n", u->vis ? " " : "~FR*~RS",
                u->recap, u->desc, u->afk ? "~BR(AFK)" : "");
    }
    if (!users) {
        write_user(user, "~FGYou are all alone here.\n");
    }
    write_user(user, "\n");
    strcpy(text, "Access is ");
    if (is_personal_room(rm)) {
        strcat(text, "personal ");
        if (is_private_room(rm)) {
            strcat(text, "~FR(locked)~RS");
        } else {
            strcat(text, "~FG(unlocked)~RS");
        }
    } else {
        if (is_fixed_room(rm)) {
            strcat(text, "~FRfixed~RS to ");
        } else {
            strcat(text, "set to ");
        }
        if (is_private_room(rm)) {
            strcat(text, "~FRPRIVATE~RS");
        } else {
            strcat(text, "~FGPUBLIC~RS");
        }
    }
    sprintf(temp, " and there %s ~OL~FM%d~RS message%s on the board.\n",
            PLTEXT_IS(rm->mesg_cnt), rm->mesg_cnt, PLTEXT_S(rm->mesg_cnt));
    strcat(text, temp);
    write_user(user, text);
    if (*rm->topic) {
        vwrite_user(user, "~FG~OLCurrent topic:~RS %s\n", rm->topic);
    } else {
        write_user(user, "~FGNo topic has been set yet.\n");
    }
}
