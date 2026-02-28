/****************************************************************************
             Amnuts - Copyright (C) Andrew Collington, 1996-2024
                        Last update: Sometime in 2024

                   talker@amnuts.net - https://amnuts.net/

                                 based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

#ifndef AMNUTS_TELNET_H
#define AMNUTS_TELNET_H

#include "../vendors/libtelnet/libtelnet.h"

static const telnet_telopt_t telopts[] = {
    { TELNET_TELOPT_BINARY,    TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_SGA,       TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_COMPRESS2, TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_ECHO,      TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_MSSP,      TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_NAWS,      TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_TTYPE,     TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_ZMP,       TELNET_WONT, TELNET_DO   },
    { -1, 0, 0 }
};

#endif
