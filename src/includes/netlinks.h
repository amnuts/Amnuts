#ifndef AMNUTS_NETLINKS_H
#define AMNUTS_NETLINKS_H

#define ML_ENTRY(a) ML_EXPAND a

/* Netlink Commands */
#define NLC_LIST \
  ML_ENTRY((DISCONNECT,   "DISCONNECT"  )) \
  ML_ENTRY((TRANSFER,     "TRANS"       )) \
  ML_ENTRY((RELEASE,      "REL"         )) \
  ML_ENTRY((ACTION,       "ACT"         )) \
  ML_ENTRY((GRANTED,      "GRANTED"     )) \
  ML_ENTRY((DENIED,       "DENIED"      )) \
  ML_ENTRY((MESSAGE,      "MSG"         )) \
  ML_ENTRY((ENDMESSAGE,   "EMSG"        )) \
  ML_ENTRY((PROMPT,       "PRM"         )) \
  ML_ENTRY((VERIFICATION, "VERIFICATION")) \
  ML_ENTRY((VERIFY,       "VERIFY"      )) \
  ML_ENTRY((REMOVED,      "REMVD"       )) \
  ML_ENTRY((ERROR,        "ERROR"       )) \
  ML_ENTRY((EXISTSQUERY,  "EXISTS?"     )) \
  ML_ENTRY((EXISTSNAK,    "EXISTS_NO"   )) \
  ML_ENTRY((EXISTSACK,    "EXISTS_YES"  )) \
  ML_ENTRY((MAIL,         "MAIL"        )) \
  ML_ENTRY((ENDMAIL,      "ENDMAIL"     )) \
  ML_ENTRY((MAILERROR,    "MAILERROR"   )) \
  ML_ENTRY((KEEPALIVE,    "KA"          )) \
  ML_ENTRY((RSTAT,        "RSTAT"       )) \
  ML_ENTRY((COUNT,        NULL          ))

/*
 * The most used commands have been truncated to save bandwidth, ie ACT is
 * short for action, EMSG is short for end message. Commands that do not get
 * used much i.e., VERIFICATION have been left long for readability.
 */

enum nlc_value {
#define ML_EXPAND(value,name) NLC_ ## value,
    NLC_LIST
#undef ML_EXPAND
};

static const char *const netcom[] = {
#define ML_EXPAND(value,name) name,
    NLC_LIST
#undef ML_EXPAND
};

#endif
