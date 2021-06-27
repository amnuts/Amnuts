#ifndef AMNUTS_TELNET_H
#define AMNUTS_TELNET_H

#include "../vendors/libtelnet/libtelnet.h"

static const telnet_telopt_t telopts[] = {
        { TELNET_TELOPT_ECHO,      TELNET_WILL, TELNET_DONT },
        { TELNET_TELOPT_TTYPE,     TELNET_WILL, TELNET_DONT },
        { TELNET_TELOPT_COMPRESS2, TELNET_WONT, TELNET_DO   },
        { TELNET_TELOPT_ZMP,       TELNET_WONT, TELNET_DO   },
        { TELNET_TELOPT_MSSP,      TELNET_WONT, TELNET_DO   },
        { TELNET_TELOPT_BINARY,    TELNET_WILL, TELNET_DO   },
        { TELNET_TELOPT_NAWS,      TELNET_WILL, TELNET_DONT },
        { -1, 0, 0 }
};

#endif
