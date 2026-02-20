# Makefile — Dominion BBS for macOS/Linux
#
# Original: Borland C++ 3.1, DOS real-mode
# Target:   clang/gcc, macOS/Linux, 64-bit
#
# Usage:
#   make -j4          Build BBS + sync data into build/
#   make binary       Build BBS binary only (no data sync)
#   make tools        Build standalone utilities (mkconfig, dosconv)
#   make init         Generate Config.dat + seed data (run once after clean)
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
           utility extrn mm1 x00com jam

BBS_MODULES = mm2 msgbase disk timest utility1 \
              file2 file3 archive filesys \
              menued uedit diredit subedit stringed \
              bbsutl2 sysopf modem xinit \
              personal misccmd config \
              lilo error chat nuv dv newuser \
              regis multline multmail fido \
              qwk

# Platform compatibility
PLATFORM = platform_stubs jam_stubs

MODULES = version $(BBS_CORE) $(BBS_MODULES) $(PLATFORM)
OBJS    = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(MODULES)))

TARGET  = $(BUILDDIR)/dominion

# Data directories the BBS needs at runtime (relative to build/)
DATA_DIRS = afiles data menus msgs scripts batch dls temp

all: $(TARGET) init

# Alias: build just the binary
binary: $(TARGET)

# Link
$(TARGET): $(OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm

# Compile
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

# --- Tool targets ---
tools: $(BUILDDIR)/mkconfig $(BUILDDIR)/dosconv

$(BUILDDIR)/mkconfig: $(TOOLDIR)/mkconfig.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(CC) -std=gnu89 -fsigned-char -I$(SRCDIR) -o $@ $<

$(BUILDDIR)/dosconv: $(TOOLDIR)/dosconv.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(CC) -std=gnu89 -fsigned-char -I$(SRCDIR) -o $@ $<

# --- Data sync from dist/ into build/ ---
data: | $(BUILDDIR)
	@for d in $(DATA_DIRS); do mkdir -p $(BUILDDIR)/$$d; done
	@for d in afiles data menus msgs scripts; do \
		if [ -d $(DISTDIR)/$$d ] && [ "$$(ls -A $(DISTDIR)/$$d 2>/dev/null)" ]; then \
			cp -a $(DISTDIR)/$$d/. $(BUILDDIR)/$$d/; \
		fi; \
	done
	@for f in DOM.KEY; do \
		if [ -f $(DISTDIR)/$$f ]; then cp -a $(DISTDIR)/$$f $(BUILDDIR)/$$f; fi; \
	done

# --- Generate Config.dat + seed data (first-time setup) ---
# Order matters: mkconfig first (generates stubs), then dist data on top
# (overwrites stubs with real content like mnudata.dat).
init: $(BUILDDIR)/mkconfig
	@if [ ! -f $(BUILDDIR)/Config.dat ]; then \
		echo "Generating Config.dat and seed data..."; \
		cd $(BUILDDIR) && ./mkconfig ./; \
	else \
		echo "Config.dat already exists (use 'make clean' first to regenerate)"; \
	fi
	@$(MAKE) data

clean:
	rm -rf $(BUILDDIR)

# Remove just objects (keep data and binary for quick relink)
clean-obj:
	rm -rf $(OBJDIR)

.PHONY: all binary clean clean-obj data tools init
