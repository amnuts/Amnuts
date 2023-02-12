#
# General setup of GCC and compiler flags, base directories, etc.
#
BINDIR          = $(CURDIR)/build
INCDIR          = $(CURDIR)/src/includes
PERMS           = 755
CC              = gcc
C_FLAGS         = -g -Wall -W -MMD
CC_FLAGS        = -I$(INCDIR)
LD_FLAGS        =

#
# TALKER_FLAGS allows you to turn off certain aspects within the talker
# as if they were never there.  Don't want Netlinks function?  Then just
# remove the '-DNETLINKS' - same for the others.
# Possible flags are (each starting with '-D'):
#     GAMES - include games
#     WIZPORT - Allow a separate port just for WIZ levels to use
#     IDENTD - Ident Daemon
#     MANDNS - Manual DNS lookups
#     NETLINKS - The infamous Netlinks
#
TALKER_FLAGS    = -DGAMES -DWIZPORT -DIDENTD -DMANDNS -DNETLINKS

#
# Locations and binary name for talker build
#
TALKER_BIN      = amnutsTalker
TALKER_SRC_DIR  = $(CURDIR)/src
TALKER_OBJ_DIR  = $(TALKER_SRC_DIR)/objects
TALKER_SRC      = $(wildcard $(TALKER_SRC_DIR)/*.c $(TALKER_SRC_DIR)/commands/*.c)
TALKER_OBJS     = $(addprefix $(TALKER_OBJ_DIR)/,$(notdir $(TALKER_SRC:.c=.o)))

#
# Locations and binary name for ident server build
#
IDENTD_BIN      = amnutsIdent
IDENTD_SRC_DIR  = $(TALKER_SRC_DIR)/identd
IDENTD_OBJ_DIR  = $(TALKER_OBJ_DIR)
IDENTD_SRC      = $(wildcard $(IDENTD_SRC_DIR)/*.c)
IDENTD_OBJS     = $(addprefix $(IDENTD_OBJ_DIR)/,$(notdir $(IDENTD_SRC:.c=.o)))

#
# Locations of vendors libraries
#
VENDOR_SDS_SRC_DIR  = $(TALKER_SRC_DIR)/vendors/sds
VENDOR_SDS_OBJ_DIR  = $(TALKER_OBJ_DIR)
VENDOR_SDS_SRC      = $(wildcard $(VENDOR_SDS_SRC_DIR)/*.c)
VENDOR_SDS_OBJS     = $(addprefix $(VENDOR_SDS_OBJ_DIR)/,$(notdir $(VENDOR_SDS_SRC:.c=.o)))

#
# Platform-specific libraries that need to be included
#
UNAME = $(shell uname)
ifeq ($(UNAME), Darwin)
	LD_FLAGS    += -L/usr/local/opt/openssl/lib
	CC_FLAGS    += -I/usr/local/opt/openssl/include
	TALKER_LIBS  = -lcrypto
	IDENTD_LIBS  =
endif
ifeq ($(UNAME), SunOS)
	TALKER_LIBS = -lnsl -lsocket
	IDENTD_LIBS =
endif
ifeq ($(UNAME), Linux)
	TALKER_LIBS = -lcrypt
	IDENTD_LIBS =
endif

#
# Build rules
#
all: build

.PHONY: all compile build install clean distclean

distclean: clean
	@echo "Removing binary and backup files"
	rm -f $(TALKER_SRC_DIR)/*.[ch]~ $(TALKER_SRC_DIR)/*.[ch].bak
	rm -f $(IDENTD_SRC_DIR)/*.[ch]~ $(IDENTD_SRC_DIR)/*.[ch].bak 
	rm -f $(VENDOR_SDS_SRC_DIR)/*.[ch]~ $(VENDOR_SDS_SRC_DIR)/*.[ch].bak
	rm -f $(TALKER_BIN) $(BINDIR)/$(TALKER_BIN)
	rm -f $(IDENTD_BIN) $(BINDIR)/$(IDENTD_BIN)
	rm -f $(INCDIR)/*.[ch]~ $(INCDIR)/*.[ch].bak

clean:
	@echo "Removing object and dependency files"
	rm -f $(TALKER_OBJS) $(TALKER_OBJS:.o=.d)
	rm -f $(IDENTD_OBJS) $(IDENTD_OBJS:.o=.d)
	rm -f $(VENDOR_SDS_OBJS) $(VENDOR_SDS_OBJS:.o=.d)

install: $(BINDIR)/$(TALKER_BIN) $(BINDIR)/$(IDENTD_BIN)

build: $(TALKER_BIN) $(IDENTD_BIN)

compile: $(TALKER_OBJS) $(IDENTD_OBJS) $(VENDOR_SDS_OBJS)

print-%: ; @echo $* = $($*)

vpath %.c $(TALKER_SRC_DIR) $(TALKER_SRC_DIR)/commands $(IDENTD_SRC_DIR) $(VENDOR_SDS_SRC_DIR)

$(BINDIR)/$(TALKER_BIN) $(BINDIR)/$(IDENTD_BIN): $(BINDIR)/%: %
	@echo "Installing $< ..."
	chmod $(PERMS) $<
	mv $< $(BINDIR)

$(TALKER_BIN): $(TALKER_OBJS) $(VENDOR_SDS_OBJS)
	@echo "Linking $@ ..."
	$(CC) $(LD_FLAGS) $^ $(TALKER_LIBS) -o $@

$(IDENTD_BIN): $(IDENTD_OBJS) $(VENDOR_SDS_OBJS)
	@echo "Linking $@ ..."
	$(CC) $(LD_FLAGS) $^ $(IDENTD_LIBS) -o $@

$(TALKER_OBJS): $(TALKER_OBJ_DIR)/%.o: %.c
	@echo "Compiling talker $< ... ($@)"
	@test -d $(TALKER_OBJ_DIR) || mkdir $(TALKER_OBJ_DIR)
	$(CC) $(C_FLAGS) $(CC_FLAGS) $(TALKER_FLAGS) -c -o $@ $<

$(IDENTD_OBJS): $(IDENTD_OBJ_DIR)/%.o: %.c
	@echo "Compiling identd $< ... ($@)"
	@test -d $(IDENTD_OBJ_DIR) || mkdir $(IDENTD_OBJ_DIR)
	$(CC) $(C_FLAGS) $(CC_FLAGS) $(TALKER_FLAGS) -c -o $@ $<

$(VENDOR_SDS_OBJS): $(VENDOR_SDS_OBJ_DIR)/%.o: %.c
	@echo "Compiling SDS library $< ... ($@)"
	@test -d $(VENDOR_SDS_OBJ_DIR) || mkdir $(VENDOR_SDS_OBJ_DIR)
	$(CC) $(C_FLAGS) $(CC_FLAGS) $(TALKER_FLAGS) -c -o $@ $<

-include $(TALKER_OBJS:.o=.d) $(IDENTD_OBJS:.o=.d) $(VENDOR_SDS_OBJS:.o=.d)
