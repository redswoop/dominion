# Makefile — Dominion BBS for macOS/Linux
#
# Original: Borland C++ 3.1, DOS real-mode
# Target:   clang/gcc, macOS/Linux, 64-bit
#
# Usage:
#   make -j4          Build BBS + sync data into build/
#   make binary       Build BBS binary only (no data sync)
#   make tools        Build standalone utilities (mkconfig, dosconv)
#   make init         Generate config.json + seed data (run once after clean)
#   make clean        Remove everything in build/
#   make clean-obj    Remove objects only (keep binary + data)
#
# To run:  cd build && ./dominion -M

SRCDIR   = src
TOOLDIR  = tools
DISTDIR  = dist
BUILDDIR = build
OBJDIR   = $(BUILDDIR)/obj

CC = cc
CFLAGS = -std=gnu89 -DPD \
         -Wall -Wno-implicit-function-declaration \
         -Wno-return-type -Wno-pointer-sign \
         -Wno-parentheses -Wno-dangling-else \
         -Wno-format -Wno-switch \
         -Wno-unused-variable -Wno-unused-value \
         -Wno-int-conversion -Wno-incompatible-pointer-types \
         -Wno-implicit-int \
         -Wno-deprecated-non-prototype \
         -Wno-comment \
         -Wno-unknown-pragmas \
         -Wno-typedef-redefinition \
         -Wno-main-return-type \
         -Wno-invalid-source-encoding \
         -Wno-char-subscripts \
         -Wno-four-char-constants \
         -Wno-multichar \
         -fsigned-char \
         -g -O0

# The BBS core modules (from the original makefile)
BBS_CORE = bbs com conio bbsutl file file1 mm \
           utility extrn mm1 tcpio jam

BBS_MODULES = mm2 msgbase disk userdb menudb timest utility1 \
              file2 file3 archive filesys \
              menued uedit diredit subedit stringed \
              bbsutl2 sysopf wfc xinit \
              personal misccmd config \
              lilo error chat nuv dv newuser \
              regis multline multmail fido \
              qwk

# Platform compatibility
PLATFORM = platform_stubs jam_stubs io_stream io_ncurses

# JSON I/O (cJSON library + serialization layer)
JSON_IO = cJSON json_io

MODULES = version $(BBS_CORE) $(BBS_MODULES) $(PLATFORM) $(JSON_IO)
OBJS    = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(MODULES)))

TARGET  = $(BUILDDIR)/dominion

# Data directories the BBS needs at runtime (relative to build/)
DATA_DIRS = afiles data data/users menus msgs scripts batch dls temp

all: $(TARGET) init

# Alias: build just the binary
binary: $(TARGET)

# Link
$(TARGET): $(OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm -lncurses

# Compile
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

# --- Tool targets ---
tools: $(BUILDDIR)/mkconfig $(BUILDDIR)/dosconv $(BUILDDIR)/mnudump $(BUILDDIR)/mnuconv $(BUILDDIR)/datadump $(BUILDDIR)/jamdump $(BUILDDIR)/termtest $(BUILDDIR)/rawinput $(BUILDDIR)/inputtest

$(BUILDDIR)/mkconfig: $(TOOLDIR)/mkconfig.c $(SRCDIR)/vardec.h $(SRCDIR)/cJSON.c $(SRCDIR)/json_io.c | $(BUILDDIR)
	$(CC) -std=gnu89 -DPD -fsigned-char -I$(SRCDIR) -o $@ $< $(SRCDIR)/cJSON.c $(SRCDIR)/json_io.c

$(BUILDDIR)/dosconv: $(TOOLDIR)/dosconv.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(CC) -std=gnu89 -DPD -fsigned-char -I$(SRCDIR) -o $@ $<

$(BUILDDIR)/mnudump: $(TOOLDIR)/mnudump.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(CC) -std=gnu89 -DPD -fsigned-char -I$(SRCDIR) -o $@ $<

$(BUILDDIR)/mnuconv: $(TOOLDIR)/mnuconv.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(CC) -std=gnu89 -DPD -fsigned-char -I$(SRCDIR) -o $@ $<

$(BUILDDIR)/datadump: $(TOOLDIR)/datadump.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(CC) -std=gnu89 -DPD -fsigned-char -I$(SRCDIR) -o $@ $<

$(BUILDDIR)/jamdump: $(TOOLDIR)/jamdump.c $(SRCDIR)/jam.h $(SRCDIR)/jamsys.h | $(BUILDDIR)
	$(CC) -std=gnu89 -DPD -fsigned-char -I$(SRCDIR) -o $@ $<

# Terminal test — links against real BBS .o files to test rendering
TERMTEST_OBJS = $(OBJDIR)/conio.o $(OBJDIR)/platform_stubs.o $(OBJDIR)/io_ncurses.o $(OBJDIR)/io_stream.o
$(BUILDDIR)/termtest: $(TOOLDIR)/termtest.c $(TERMTEST_OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) -I$(SRCDIR) -o $@ $< $(TERMTEST_OBJS) -lncurses

# Raw input byte inspector — standalone, no BBS dependencies
$(BUILDDIR)/rawinput: $(TOOLDIR)/rawinput.c | $(BUILDDIR)
	$(CC) -std=gnu89 -o $@ $<

# Input function test — links against real BBS .o files to test input1/inputdat/getkey
INPUTTEST_OBJS = $(OBJDIR)/com.o $(OBJDIR)/tcpio.o $(OBJDIR)/conio.o $(OBJDIR)/platform_stubs.o $(OBJDIR)/io_ncurses.o $(OBJDIR)/io_stream.o
$(BUILDDIR)/inputtest: $(TOOLDIR)/inputtest.c $(INPUTTEST_OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) -I$(SRCDIR) -o $@ $< $(INPUTTEST_OBJS) -lncurses

# --- Data sync from dist/ into build/ ---
data: | $(BUILDDIR)
	@for d in $(DATA_DIRS); do mkdir -p $(BUILDDIR)/$$d; done
	@for d in afiles data menus msgs scripts; do \
		if [ -d $(DISTDIR)/$$d ] && [ "$$(ls -A $(DISTDIR)/$$d 2>/dev/null)" ]; then \
			cp -a $(DISTDIR)/$$d/. $(BUILDDIR)/$$d/; \
		fi; \
	done
	@for f in dom.key; do \
		if [ -f $(DISTDIR)/$$f ]; then cp -a $(DISTDIR)/$$f $(BUILDDIR)/$$f; fi; \
	done

# --- Generate config.json + seed data (first-time setup) ---
# Order matters: mkconfig first (generates JSON), then dist data on top
# (overwrites stubs with real content like mnudata.dat).
init: $(BUILDDIR)/mkconfig
	@if [ ! -f $(BUILDDIR)/config.json ]; then \
		echo "Generating config.json and seed data..."; \
		cd $(BUILDDIR) && ./mkconfig .; \
	else \
		echo "config.json already exists (use 'make clean' first to regenerate)"; \
	fi
	@$(MAKE) data

clean:
	rm -rf $(BUILDDIR)

# Remove just objects (keep data and binary for quick relink)
clean-obj:
	rm -rf $(OBJDIR)

.PHONY: all binary clean clean-obj data tools init
