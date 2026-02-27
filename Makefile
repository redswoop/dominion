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
CXX = c++
# C flags — used only for standalone tools (mkconfig, dosconv, etc.)
CFLAGS = -std=gnu89 \
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
# C++ flags — used for ALL BBS source (.c compiled as C++ via -x c++, plus .cpp)
CXXFLAGS = -std=c++17 \
           -Wall -Wextra \
           -Wno-unused-parameter -Wno-unused-variable -Wno-unused-value \
           -Wno-return-type -Wno-parentheses -Wno-dangling-else \
           -Wno-format -Wno-switch \
           -Wno-comment -Wno-unknown-pragmas \
           -Wno-invalid-source-encoding \
           -Wno-char-subscripts -Wno-four-char-constants -Wno-multichar \
           -Wno-write-strings -Wno-narrowing -Wno-register \
           -Wno-deprecated-declarations \
           -fsigned-char -g -O0

# The BBS core modules (from the original makefile)
BBS_CORE = bbs ansi_attr bbs_output bbs_input bbs_ui conio bbsutl file file1 \
           utility extrn mm1 tcpio jam stream_processor mci mci_bbs \
           bbs_path bbs_file

BBS_MODULES = cmd_registry acs menu_nav msgbase disk user userdb menudb timest shortmsg \
              file2 file3 archive filesys \
              menued uedit diredit subedit stringed \
              sysoplog sysopf wfc xinit \
              personal misccmd automsg bbslist timebank topten config \
              lilo error chat nuv newuser newuser_form

# Platform compatibility
PLATFORM = platform_stubs jam_stubs io_stream session system terminal terminal_bridge screen_form file_lock node_registry vt100 console

# JSON I/O (cJSON library + serialization layer)
JSON_IO = cJSON json_io menu_json

MODULES = version $(BBS_CORE) $(BBS_MODULES) $(PLATFORM) $(JSON_IO)
OBJS    = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(MODULES)))

TARGET  = $(BUILDDIR)/dominion

# Data directories the BBS needs at runtime (relative to build/)
DATA_DIRS = afiles data data/users data/nodes menus msgs scripts batch dls temp

all: $(TARGET) init

# Alias: build just the binary
binary: $(TARGET)

# Link (CXX needed for terminal.o / terminal_bridge.o)
$(TARGET): $(OBJS) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) -lm -lncurses

# Compile C++
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create directories
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

# --- Tool targets ---
# termtest and inputtest excluded — need Phase C migration (vars.h → session/system singletons)
tools: $(BUILDDIR)/mkconfig $(BUILDDIR)/dosconv $(BUILDDIR)/mnudump $(BUILDDIR)/mnuconv $(BUILDDIR)/mnu2json $(BUILDDIR)/datadump $(BUILDDIR)/jamdump $(BUILDDIR)/rawinput $(BUILDDIR)/iotest $(BUILDDIR)/uitest $(BUILDDIR)/formtest $(BUILDDIR)/test_bbs_path

$(BUILDDIR)/mkconfig: $(TOOLDIR)/mkconfig.c $(SRCDIR)/vardec.h $(SRCDIR)/cJSON.cpp $(SRCDIR)/json_io.cpp $(SRCDIR)/file_lock.cpp | $(BUILDDIR)
	$(CXX) -std=c++17 -fsigned-char -I$(SRCDIR) -o $@ -x c++ $< $(SRCDIR)/cJSON.cpp $(SRCDIR)/json_io.cpp $(SRCDIR)/file_lock.cpp

$(BUILDDIR)/dosconv: $(TOOLDIR)/dosconv.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(CC) -std=gnu89 -fsigned-char -I$(SRCDIR) -o $@ $<

$(BUILDDIR)/mnudump: $(TOOLDIR)/mnudump.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(CC) -std=gnu89 -fsigned-char -I$(SRCDIR) -o $@ $<

$(BUILDDIR)/mnuconv: $(TOOLDIR)/mnuconv.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(CC) -std=gnu89 -fsigned-char -I$(SRCDIR) -o $@ $<

$(BUILDDIR)/mnu2json: $(TOOLDIR)/mnu2json.c $(SRCDIR)/vardec.h $(SRCDIR)/menu_json.cpp $(SRCDIR)/menu_json.h $(SRCDIR)/cJSON.cpp $(SRCDIR)/json_io.cpp $(SRCDIR)/file_lock.cpp | $(BUILDDIR)
	$(CXX) -std=c++17 -fsigned-char -I$(SRCDIR) -o $@ -x c++ $< $(SRCDIR)/menu_json.cpp $(SRCDIR)/cJSON.cpp $(SRCDIR)/json_io.cpp $(SRCDIR)/file_lock.cpp

$(BUILDDIR)/datadump: $(TOOLDIR)/datadump.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(CC) -std=gnu89 -fsigned-char -I$(SRCDIR) -o $@ $<

$(BUILDDIR)/jamdump: $(TOOLDIR)/jamdump.c $(SRCDIR)/jam.h $(SRCDIR)/jamsys.h | $(BUILDDIR)
	$(CC) -std=gnu89 -fsigned-char -I$(SRCDIR) -o $@ $<

# Terminal test — links against real BBS .o files to test rendering
TERMTEST_OBJS = $(OBJDIR)/conio.o $(OBJDIR)/platform_stubs.o $(OBJDIR)/io_stream.o $(OBJDIR)/terminal.o $(OBJDIR)/terminal_bridge.o $(OBJDIR)/session.o $(OBJDIR)/system.o
$(BUILDDIR)/termtest: $(TOOLDIR)/termtest.c $(TERMTEST_OBJS) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -o $@ -x c++ $< -x none $(TERMTEST_OBJS) -lncurses

# Raw input byte inspector — standalone, no BBS dependencies
$(BUILDDIR)/rawinput: $(TOOLDIR)/rawinput.c | $(BUILDDIR)
	$(CC) -std=gnu89 -o $@ $<

# Input function test — links against real BBS .o files to test input1/inputdat/getkey
INPUTTEST_OBJS = $(OBJDIR)/bbs_output.o $(OBJDIR)/bbs_input.o $(OBJDIR)/bbs_ui.o $(OBJDIR)/ansi_attr.o $(OBJDIR)/tcpio.o $(OBJDIR)/conio.o $(OBJDIR)/platform_stubs.o $(OBJDIR)/io_stream.o $(OBJDIR)/terminal.o $(OBJDIR)/terminal_bridge.o $(OBJDIR)/stream_processor.o
$(BUILDDIR)/inputtest: $(TOOLDIR)/inputtest.c $(INPUTTEST_OBJS) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -o $@ -x c++ $< -x none $(INPUTTEST_OBJS) -lncurses

# IO test — clean Terminal class, ZERO BBS dependencies
$(BUILDDIR)/iotest: $(TOOLDIR)/iotest.cpp $(OBJDIR)/terminal.o | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -o $@ $< $(OBJDIR)/terminal.o -lncurses

# UI test — declarative UI harness, ZERO BBS dependencies
$(OBJDIR)/ui.o: $(SRCDIR)/ui.cpp $(SRCDIR)/ui.h $(SRCDIR)/terminal.h | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -c -o $@ $<

$(BUILDDIR)/uitest: $(TOOLDIR)/uitest.cpp $(OBJDIR)/ui.o $(OBJDIR)/terminal.o | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -o $@ $< $(OBJDIR)/ui.o $(OBJDIR)/terminal.o -lncurses

# Form test — fullscreen positioned forms, ZERO BBS dependencies
$(BUILDDIR)/formtest: $(TOOLDIR)/formtest.cpp $(OBJDIR)/ui.o $(OBJDIR)/screen_form.o $(OBJDIR)/terminal.o | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -o $@ $< $(OBJDIR)/ui.o $(OBJDIR)/screen_form.o $(OBJDIR)/terminal.o -lncurses

# BbsPath unit tests — standalone, no BBS dependencies
$(BUILDDIR)/test_bbs_path: tests/test_bbs_path.cpp $(SRCDIR)/bbs_path.cpp $(SRCDIR)/bbs_path.h | $(BUILDDIR)
	$(CXX) -std=c++17 -I$(SRCDIR) -o $@ tests/test_bbs_path.cpp $(SRCDIR)/bbs_path.cpp

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
init: $(BUILDDIR)/mkconfig $(BUILDDIR)/mnu2json
	@if [ ! -f $(BUILDDIR)/config.json ]; then \
		echo "Generating config.json and seed data..."; \
		cd $(BUILDDIR) && ./mkconfig .; \
	else \
		echo "config.json already exists (use 'make distclean' first to regenerate)"; \
	fi
	@$(MAKE) data
	@$(MAKE) menus-json

# --- Convert binary .mnu files to .json in build/menus/ ---
menus-json: $(BUILDDIR)/mnu2json
	@if ls $(BUILDDIR)/menus/*.mnu 1>/dev/null 2>&1; then \
		echo "Converting .mnu files to .json..."; \
		for f in $(BUILDDIR)/menus/*.mnu; do \
			base=$$(basename "$$f" .mnu); \
			if [ ! -f "$(BUILDDIR)/menus/$$base.json" ]; then \
				$(BUILDDIR)/mnu2json "$$f"; \
			fi; \
		done; \
	fi

# Remove binary + objects, preserve data dirs (config, users, messages, etc.)
clean:
	rm -rf $(OBJDIR)
	rm -f $(BUILDDIR)/dominion
	rm -f $(BUILDDIR)/mkconfig $(BUILDDIR)/dosconv $(BUILDDIR)/mnudump
	rm -f $(BUILDDIR)/mnuconv $(BUILDDIR)/mnu2json $(BUILDDIR)/datadump $(BUILDDIR)/jamdump
	rm -f $(BUILDDIR)/termtest $(BUILDDIR)/rawinput $(BUILDDIR)/inputtest
	rm -f $(BUILDDIR)/iotest $(BUILDDIR)/uitest $(BUILDDIR)/formtest
	rm -f $(BUILDDIR)/test_bbs_path

# Remove just objects (keep binary + data for quick relink)
clean-obj:
	rm -rf $(OBJDIR)

# Nuclear: remove entire build directory including all data
distclean:
	rm -rf $(BUILDDIR)

.PHONY: all binary clean clean-obj distclean data tools init menus-json
