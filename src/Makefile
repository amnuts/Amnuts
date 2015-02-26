#
# Amnuts version 2.3.0
# Copyright (c) Andrew Collington 1996-2003
# http://amnuts.talker.com/
# mailto:amnuts@talker.com
#
# gmake (GNU make) is presently REQUIRED!

TALKER          = amnuts230
IDENTD          = amnutsIdent
BINDIR          = ..
OBJDIR          = objects
INCDIR          = includes
SRCDIR          = $(CURDIR)
PERMS           = 755
CC              = cc
#CC             = cc -V 3.0.4 -std=c99 -D_XOPEN_SOURCE=600
# This should take cpp args and cause dependancy info to go to stdout
# It would be nice to use gcc -MMD to make this at the same time as compile
# It would be nice to use gcc 3+ (2.96+) -MP to fix dependancy info for gmake
MAKEDEPEND      = $(CC) -MM
#MAKEDEPEND     = makedepend -f -

# GAMES => include games (i.e., hangman, gun duelling, money)
# WIZPORT => include support for wizport
# NETLINKS => include support for netlinks
# MANDNS => include support for "host" program for name resolution
# IDENTD => include support for "ident" daemon for name resolution
# ISOWEEKS => use ISO 8601 week calendars (Mon-Sun; first week has 4+ days).
# DOUBLEFORK => use old double_fork() code (actually quad fork() with system())
# DUMPCORE => cause crash actions to dump core and exit for debugging
#TALKERCPPFLAGS = -I$(INCDIR)
#TALKERCPPFLAGS = -I$(INCDIR) -DGAMES
#TALKERCPPFLAGS = -I$(INCDIR) -DMANDNS -DWIZPORT -DGAMES
#TALKERCPPFLAGS = -I$(INCDIR) -DMANDNS -DNETLINKS -DWIZPORT -DGAMES
#TALKERCPPFLAGS = -I$(INCDIR) -DIDENTD -DWIZPORT -DGAMES
#TALKERCPPFLAGS = -I$(INCDIR) -DIDENTD -DNETLINKS -DWIZPORT -DGAMES
TALKERCPPFLAGS = -I$(INCDIR) -DIDENTD -DMANDNS -DNETLINKS -DWIZPORT -DGAMES

# MANDNS => use for "host" program for name resolution
# DEBUG => include copious debugging output to stdout
IDENTDCPPFLAGS = -I$(INCDIR)
#IDENTDCPPFLAGS = -I$(INCDIR) -DMANDNS

# These are sort of gcc dependant; roll your own if you use another compiler
CFLAGS         = -g -Wall -W
#CFLAGS         = -g -O2 -Wall -W
#CFLAGS         = -g -O0 -fno-inline
#CFLAGS         = -g -O2 -Wall -W -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wpacked -Wnested-externs -Winline -Wlong-long
#CFLAGS         = -g -O2 -Wall -W -pedantic -Wfloat-equal -Wtraditional -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn -Wpacked -Wpadded -Wredundant-decls -Wnested-externs -Wunreachable-code -Winline -Wlong-long
#CFLAGS         = -Q -fprofile-arcs -ftest-coverage -ggdb3 -O2 -Wall -W

LDFLAGS         =

# if you add a new file you must add a line here!
TALKEROBJS      = \
        $(OBJDIR)/amnuts230.o \
        $(OBJDIR)/pager.o \
        $(OBJDIR)/reboot.o \
        $(OBJDIR)/netlinks.o \
        $(OBJDIR)/rooms.o \
        $(OBJDIR)/messages.o \
        $(OBJDIR)/games.o \
        $(OBJDIR)/objects.o \
        $(OBJDIR)/speech.o \
        $(OBJDIR)/strings.o \
        $(OBJDIR)/admin.o \
        $(OBJDIR)/spodlist.o

IDENTDOBJS      = \
        $(OBJDIR)/ident.o

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
        TALKERLIBS     = -lcrypto
endif

ifeq ($(UNAME), SunOS)
        TALKERLIBS     = -lnsl -lsocket
endif

ifeq ($(UNAME), Linux)
        TALKERLIBS     = -lcrypt
endif

#if you're using Solaris, comment the above and uncomment the below
#TALKERLIBS     = -lnsl -lsocket

IDENTDLIBS     =
#if you're using Solaris, comment the above and uncomment the below
#IDENTDLIBS     = -lnsl -lsocket

#
# you should not need to alter anything below this point
#

#VPATH = $(SRCDIR)
#vpath %.c $(SRCDIR)
#vpath %.h $(SRCDIR)/$(INCDIR)
#vpath %.o $(OBJDIR)

all: build

.PHONY: all compile build install clean distclean

distclean: clean
	@echo "Removing binary and backup files"
	rm -f $(TALKER) $(BINDIR)/$(TALKER) $(IDENTD) $(BINDIR)/$(IDENTD)
	rm -f *.[ch]~ *.[ch].bak $(INCDIR)/*.[ch]~ $(INCDIR)/*.[ch].bak

clean:
	@echo "Removing object and dependancy files"
	rm -f $(TALKEROBJS) $(IDENTDOBJS)
	rm -f $(TALKEROBJS:.o=.d) $(IDENTDOBJS:.o=.d)

install: $(BINDIR)/$(TALKER) $(BINDIR)/$(IDENTD)

build: $(TALKER) $(IDENTD)

compile: $(TALKEROBJS) $(IDENTDOBJS)

$(BINDIR)/$(TALKER) $(BINDIR)/$(IDENTD): $(BINDIR)/%: %
	@echo "Installing $< ..."
	chmod $(PERMS) $<
	ln -f $< $@

$(TALKER): $(TALKEROBJS)
	@echo "Linking $@ ..."
	$(CC) $(LDFLAGS) $^ $(TALKERLIBS) -o $@

$(IDENTD): $(IDENTDOBJS)
	@echo "Linking $@ ..."
	$(CC) $(LDLAGS) $^ $(IDENTDLIBS) -o $@

$(TALKEROBJS): $(OBJDIR)/%.o: %.c
	@echo "Compiling $< ..."
	@test -d $(OBJDIR) || mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $(TALKERCPPFLAGS) -c $< -o $@
	@$(MAKEDEPEND) $(TALKERCPPFLAGS) $< | sed -e 's/#.*//' -e '/\\$$/N' -e 's/\\\n//g' -e 's/  */ /g' -e 's/^ //' -e 's/ $$//' -e '/^$$/d' -e 's!^[^:]*$*\.o *: *!$@: !' -e 'p' -e 's!^[^:]*$*\.o *: *$*\.c *!!' -e '/^$$/d' -e 's/$$/:/' > $(@:.o=.d)

$(IDENTDOBJS): $(OBJDIR)/%.o: %.c
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) $(IDENTDCPPFLAGS) -c $< -o $@
	@$(MAKEDEPEND) $(IDENTDCPPFLAGS) $< | sed -e 's/#.*//' -e '/\\$$/N' -e 's/\\\n//g' -e 's/  */ /g' -e 's/^ //' -e 's/ $$//' -e '/^$$/d' -e 's!^[^:]*$*\.o *: *!$@: !' -e 'p' -e 's!^[^:]*$*\.o *: *$*\.c *!!' -e '/^$$/d' -e 's/$$/:/' > $(@:.o=.d)

-include $(TALKEROBJS:.o=.d) $(IDENTDOBJS:.o=.d)

