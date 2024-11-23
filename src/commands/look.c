
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
    sds temp, text = sdsempty();
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
    text = sdscpy(text, "\n~FCExits are:");
    for (i = 0; i < MAX_LINKS; ++i) {
        if (!rm->link[i]) {
            break;
        }
        if (is_private_room(rm->link[i])) {
            temp = sdscatfmt(sdsempty(), "  ~FR%s", rm->link[i]->name);
        } else {
            temp = sdscatfmt(sdsempty(), "  ~FG%s", rm->link[i]->name);
        }
        text = sdscat(text, temp);
        ++exits;
    }
#ifdef NETLINKS
    if (rm->netlink && rm->netlink->stage == UP) {
        if (rm->netlink->allow == IN) {
            temp = sdscatfmt(sdsempty(), "  ~FR%s*", rm->netlink->service);
        } else {
            temp = sdscatfmt(sdsempty(), "  ~FG%s*", rm->netlink->service);
        }
        text = sdscat(text, temp);
    } else
#endif
        if (!exits) {
            text = sdscpy(text, "\n~FCThere are no exits.");
        }
    text = sdscat(text, "\n\n");
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
    text = sdscpy(text, "Access is ");
    if (is_personal_room(rm)) {
        strcat(text, "personal ");
        if (is_private_room(rm)) {
            text = sdscat(text, "~FR(locked)~RS");
        } else {
            text = sdscat(text, "~FG(unlocked)~RS");
        }
    } else {
        if (is_fixed_room(rm)) {
            text = sdscat(text, "~FRfixed~RS to ");
        } else {
            text = sdscat(text, "set to ");
        }
        if (is_private_room(rm)) {
            text = sdscat(text, "~FRPRIVATE~RS");
        } else {
            text = sdscat(text, "~FGPUBLIC~RS");
        }
    }
    temp = sdscatprintf(sdsempty(), " and there %s ~OL~FM%d~RS message%s on the board.\n",
            PLTEXT_IS(rm->mesg_cnt), rm->mesg_cnt, PLTEXT_S(rm->mesg_cnt));
    text = sdscat(text, temp);
    write_user(user, text);
    if (*rm->topic) {
        vwrite_user(user, "~FG~OLCurrent topic:~RS %s\n", rm->topic);
    } else {
        write_user(user, "~FGNo topic has been set yet.\n");
    }
    sdsfree(text);
    sdsfree(temp);
}
