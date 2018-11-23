BINDIR          = $(CURDIR)
INCDIR          = $(CURDIR)/src/includes
PERMS           = 755
CC              = gcc
C_FLAGS         = -g -Wall -W -MMD
CC_FLAGS        = -I$(INCDIR) -DIDENTD -DMANDNS -DNETLINKS -DWIZPORT -DGAMES
LD_FLAGS        =

TALKER_BIN      = amnuts230
TALKER_OBJ_DIR  = $(CURDIR)/src/objects
TALKER_SRC_DIR  = $(CURDIR)/src
TALKER_SRC      = $(wildcard $(TALKER_SRC_DIR)/*.c $(TALKER_SRC_DIR)/commands/*.c)
TALKER_OBJS     = $(addprefix $(TALKER_OBJ_DIR)/,$(notdir $(TALKER_SRC:.c=.o)))

IDENTD_BIN      = amnutsIdent
IDENTD_OBJ_DIR  = $(TALKER_SRC_DIR)/objects
IDENTD_SRC_DIR  = $(TALKER_SRC_DIR)/identd
IDENTD_SRC      = $(wildcard $(IDENTD_SRC_DIR)/*.c)
IDENTD_OBJS     = $(addprefix $(IDENTD_OBJ_DIR)/,$(notdir $(IDENTD_SRC:.c=.o)))

UNAME           = $(shell uname)
ifeq ($(UNAME), Darwin)
	TALKER_LIBS = -lcrypto
endif
ifeq ($(UNAME), SunOS)
	TALKER_LIBS = -lnsl -lsocket
endif
ifeq ($(UNAME), Linux)
	TALKER_LIBS = -lcrypt
endif

IDENTD_LIBS = 

all: build

.PHONY: all compile build install clean distclean

distclean: clean
	@echo "Removing binary and backup files"
	rm -f $(TALKER_SRC_DIR)/*.[ch]~ $(TALKER_SRC_DIR)/*.[ch].bak
	rm -f $(IDENTD_SRC_DIR)/*.[ch]~ $(IDENTD_SRC_DIR)/*.[ch].bak 
	rm -f $(TALKER_BIN) $(BINDIR)/$(TALKER_BIN)
	rm -f $(IDENTD_BIN) $(BINDIR)/$(IDENTD_BIN)
	rm -f $(INCDIR)/*.[ch]~ $(INCDIR)/*.[ch].bak

clean:
	@echo "Removing object and dependancy files"
	rm -f $(TALKER_OBJS) $(TALKER_OBJS:.o=.d)
	rm -f $(IDENTD_OBJS) $(IDENTD_OBJS:.o=.d)

install: $(BINDIR)/$(TALKER_BIN) $(BINDIR)/$(IDENTD_BIN)

build: $(TALKER_BIN) $(IDENTD_BIN)

compile: $(TALKER_OBJS) $(IDENTD_OBJS)

print-%: ; @echo $* = $($*)


vpath %.c $(TALKER_SRC_DIR) $(TALKER_SRC_DIR)/commands $(IDENTD_SRC_DIR)


$(BINDIR)/$(TALKER_BIN) $(BINDIR)/$(IDENTD_BIN): $(BINDIR)/%: %
	@echo "Installing $< ..."
	chmod $(PERMS) $<

$(TALKER_BIN): $(TALKER_OBJS)
	@echo "Linking $@ ..."
	$(CC) $(LD_FLAGS) $^ $(TALKER_LIBS) -o $@

$(IDENTD_BIN): $(IDENTD_OBJS)
	@echo "Linking $@ ..."
	$(CC) $(LD_FLAGS) $^ $(IDENTD_LIBS) -o $@

$(TALKER_OBJS): $(TALKER_OBJ_DIR)/%.o: %.c
	@echo "Compiling talker $< ... ($@)"
	@test -d $(TALKER_OBJ_DIR) || mkdir $(TALKER_OBJ_DIR)
	$(CC) $(C_FLAGS) $(CC_FLAGS) -c -o $@ $<

$(IDENTD_OBJS): $(IDENTD_OBJ_DIR)/%.o: %.c
	@echo "Compiling identd $< ... ($@)"
	@test -d $(IDENTD_OBJ_DIR) || mkdir $(IDENTD_OBJ_DIR)
	$(CC) $(C_FLAGS) $(CC_FLAGS) -c -o $@ $<

-include $(TALKER_OBJS:.o=.d) $(IDENTD_OBJS:.o=.d)

