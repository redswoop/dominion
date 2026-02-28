# Makefile — Dominion BBS for macOS/Linux
#
# Lakos-style packages:
#   src/terminal/  → libterminal.a  (Terminal I/O, ANSI, VT100, CP437)
#   src/tui/       → libtui.a       (Declarative UI: ScreenForm, Navigator)
#   src/jam/       → libjam.a       (JAM message base: format types, POSIX I/O)
#   src/files/     → libfiles.a     (File transfer: listings, upload/download, archive)
#   src/user/      → libuser.a      (User: class, DB, registration, editor, voting)
#   src/           → BBS application (everything else)
#
# Usage:
#   make -j4            Build BBS + sync data (release build)
#   make DEBUG=1        Build with debug flags (-O0 -g)
#   make V=1            Verbose build output
#   make binary         Build BBS binary only (no data sync)
#   make tools          Build standalone utilities (mkconfig, dosconv, etc.)
#   make test-terminal  Build terminal library tests
#   make test-tui       Build TUI library tests
#   make test-jam       Build JAM library test
#   make init           Generate config.json + seed data (run once after clean)
#   make install        Install to PREFIX (default: /usr/local)
#   make clean          Remove build artifacts
#   make clean-obj      Remove objects only (keep binary + data)
#   make help           Show all targets
#
# To run:  cd build && ./dominion -M

# =================================================================
# Platform detection
# =================================================================

PLATFORM := $(shell uname -s)

ifeq ($(PLATFORM),Darwin)
  PLATFORM_CPPFLAGS := -DPLATFORM_DARWIN
  PLATFORM_LDLIBS   :=
else ifeq ($(PLATFORM),Linux)
  PLATFORM_CPPFLAGS := -DPLATFORM_LINUX
  PLATFORM_LDLIBS   := -lutil
else
  $(warning Untested platform: $(PLATFORM) — assuming Linux-like)
  PLATFORM_CPPFLAGS := -DPLATFORM_LINUX
  PLATFORM_LDLIBS   := -lutil
endif

# =================================================================
# Build configuration (release by default, DEBUG=1 for debug)
# =================================================================

ifdef DEBUG
  OPTFLAGS := -O0 -g -DDEBUG
else
  OPTFLAGS := -O2 -g -DNDEBUG
endif

# =================================================================
# Quiet builds (V=1 for verbose)
# =================================================================

ifneq ($(V),1)
  Q   := @
  MSG := @printf '  %-7s %s\n'
else
  Q   :=
  MSG := @true
endif

# =================================================================
# Directories
# =================================================================

SRCDIR   = src
TOOLDIR  = tools
DISTDIR  = dist
BUILDDIR = build
OBJDIR   = $(BUILDDIR)/obj

# =================================================================
# Toolchain
# =================================================================

CC  = cc
CXX = c++
AR  = ar

# =================================================================
# Compiler / linker flags
# =================================================================

# C flags — standalone C tools only (mkconfig, dosconv, jamdump, etc.)
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

# C++ flags — all BBS + library source
CXXFLAGS = -std=c++17 -Isrc -MMD -MP \
           -Wall -Wextra \
           -Wno-unused-parameter -Wno-unused-variable -Wno-unused-value \
           -Wno-return-type -Wno-parentheses -Wno-dangling-else \
           -Wno-format -Wno-switch \
           -Wno-comment -Wno-unknown-pragmas \
           -Wno-invalid-source-encoding \
           -Wno-char-subscripts -Wno-four-char-constants -Wno-multichar \
           -Wno-write-strings -Wno-narrowing -Wno-register \
           -Wno-deprecated-declarations \
           -fsigned-char \
           $(PLATFORM_CPPFLAGS) $(OPTFLAGS)

# Linker flags (add custom search paths here, e.g. -L/usr/local/lib)
LDFLAGS  =

# Libraries to link
LDLIBS   = -lm -lncurses $(PLATFORM_LDLIBS)

# =================================================================
# Library sources (wildcard discovery)
# =================================================================

TERMINAL_SRC := $(wildcard $(SRCDIR)/terminal/*.cpp)
TUI_SRC      := $(wildcard $(SRCDIR)/tui/*.cpp)
JAM_SRC      := $(wildcard $(SRCDIR)/jam/*.cpp)
FILES_SRC    := $(wildcard $(SRCDIR)/files/*.cpp)
USER_SRC     := $(wildcard $(SRCDIR)/user/*.cpp)

# Library objects — prefixed to avoid name collisions in flat obj/
TERMINAL_OBJS := $(patsubst $(SRCDIR)/terminal/%.cpp,$(OBJDIR)/terminal_%.o,$(TERMINAL_SRC))
TUI_OBJS      := $(patsubst $(SRCDIR)/tui/%.cpp,$(OBJDIR)/tui_%.o,$(TUI_SRC))
JAM_OBJS      := $(patsubst $(SRCDIR)/jam/%.cpp,$(OBJDIR)/jam_%.o,$(JAM_SRC))
FILES_OBJS    := $(patsubst $(SRCDIR)/files/%.cpp,$(OBJDIR)/files_%.o,$(FILES_SRC))
USER_OBJS     := $(patsubst $(SRCDIR)/user/%.cpp,$(OBJDIR)/user_%.o,$(USER_SRC))

# Library archives
LIBTERMINAL := $(BUILDDIR)/libterminal.a
LIBTUI      := $(BUILDDIR)/libtui.a
LIBJAM      := $(BUILDDIR)/libjam.a
LIBFILES    := $(BUILDDIR)/libfiles.a
LIBUSER     := $(BUILDDIR)/libuser.a
LIBS        := $(LIBTERMINAL) $(LIBTUI) $(LIBJAM) $(LIBFILES) $(LIBUSER)

# =================================================================
# BBS application modules (flat in src/)
# =================================================================

BBS_CORE = bbs bbs_output bbs_input bbs_ui conio bbsutl \
           utility extrn mm1 tcpio jam stream_processor mci mci_bbs \
           bbs_path bbs_file

BBS_MODULES = cmd_registry acs menu_nav msgbase disk menudb timest shortmsg \
              menued subedit stringed \
              sysoplog sysopf wfc xinit \
              personal misccmd automsg bbslist timebank topten config \
              lilo error chat

PLAT_MODULES = platform_stubs io_stream session system terminal_bridge \
               file_lock node_registry console

JSON_IO = cJSON json_io menu_json

MODULES  = version $(BBS_CORE) $(BBS_MODULES) $(PLAT_MODULES) $(JSON_IO)
BBS_OBJS = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(MODULES)))

TARGET = $(BUILDDIR)/dominion

# Data directories the BBS needs at runtime (relative to build/)
DATA_DIRS = afiles data data/users data/nodes menus msgs scripts batch dls temp

# Install paths
PREFIX  ?= /usr/local
DESTDIR ?=

# =================================================================
# Top-level targets
# =================================================================

all: $(TARGET) init

binary: $(TARGET)

# =================================================================
# Libraries
# =================================================================

$(LIBTERMINAL): $(TERMINAL_OBJS) | $(BUILDDIR)
	$(MSG) AR $@
	$(Q)$(AR) rcs $@ $^

$(LIBTUI): $(TUI_OBJS) | $(BUILDDIR)
	$(MSG) AR $@
	$(Q)$(AR) rcs $@ $^

$(LIBJAM): $(JAM_OBJS) | $(BUILDDIR)
	$(MSG) AR $@
	$(Q)$(AR) rcs $@ $^

$(LIBFILES): $(FILES_OBJS) | $(BUILDDIR)
	$(MSG) AR $@
	$(Q)$(AR) rcs $@ $^

$(LIBUSER): $(USER_OBJS) | $(BUILDDIR)
	$(MSG) AR $@
	$(Q)$(AR) rcs $@ $^

# =================================================================
# BBS binary
# =================================================================

$(TARGET): $(BBS_OBJS) $(LIBS) | $(BUILDDIR)
	$(MSG) LD $@
	$(Q)$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(BBS_OBJS) $(LIBS) $(LDLIBS)

# =================================================================
# Compile rules — per-directory patterns
# =================================================================

# Library: src/terminal/*.cpp → build/obj/terminal_*.o
$(OBJDIR)/terminal_%.o: $(SRCDIR)/terminal/%.cpp | $(OBJDIR)
	$(MSG) CXX $<
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

# Library: src/tui/*.cpp → build/obj/tui_*.o
$(OBJDIR)/tui_%.o: $(SRCDIR)/tui/%.cpp | $(OBJDIR)
	$(MSG) CXX $<
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

# Library: src/jam/*.cpp → build/obj/jam_*.o
$(OBJDIR)/jam_%.o: $(SRCDIR)/jam/%.cpp | $(OBJDIR)
	$(MSG) CXX $<
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

# Library: src/files/*.cpp → build/obj/files_*.o
$(OBJDIR)/files_%.o: $(SRCDIR)/files/%.cpp | $(OBJDIR)
	$(MSG) CXX $<
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

# Library: src/user/*.cpp → build/obj/user_*.o
$(OBJDIR)/user_%.o: $(SRCDIR)/user/%.cpp | $(OBJDIR)
	$(MSG) CXX $<
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

# BBS: src/*.cpp → build/obj/*.o
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(MSG) CXX $<
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

# =================================================================
# Directories
# =================================================================

$(BUILDDIR):
	$(Q)mkdir -p $(BUILDDIR)

$(OBJDIR):
	$(Q)mkdir -p $(OBJDIR)

# =================================================================
# Library test targets
# =================================================================

test-terminal: $(BUILDDIR)/iotest

test-tui: $(BUILDDIR)/uitest $(BUILDDIR)/formtest

test-jam: $(BUILDDIR)/jamdump

# IO test — clean Terminal class, ZERO BBS dependencies
$(BUILDDIR)/iotest: $(SRCDIR)/terminal/tests/iotest.cpp $(LIBTERMINAL) | $(BUILDDIR)
	$(MSG) LD $@
	$(Q)$(CXX) $(CXXFLAGS) -o $@ $< $(LIBTERMINAL) -lncurses

# UI test — declarative UI harness, ZERO BBS dependencies
$(BUILDDIR)/uitest: $(SRCDIR)/tui/tests/uitest.cpp $(LIBTUI) $(LIBTERMINAL) | $(BUILDDIR)
	$(MSG) LD $@
	$(Q)$(CXX) $(CXXFLAGS) -o $@ $< $(LIBTUI) $(LIBTERMINAL) -lncurses

# Form test — fullscreen positioned forms, ZERO BBS dependencies
$(BUILDDIR)/formtest: $(SRCDIR)/tui/tests/formtest.cpp $(LIBTUI) $(LIBTERMINAL) | $(BUILDDIR)
	$(MSG) LD $@
	$(Q)$(CXX) $(CXXFLAGS) -o $@ $< $(LIBTUI) $(LIBTERMINAL) -lncurses

# JAM dump — standalone C, just JAM headers
$(BUILDDIR)/jamdump: $(SRCDIR)/jam/tests/jamdump.c | $(BUILDDIR)
	$(MSG) CC $@
	$(Q)$(CC) $(CFLAGS) -Isrc -o $@ $<

# =================================================================
# Standalone tools
# =================================================================

# Suppress warnings in tools: mkconfig.c has char* string literals,
# cJSON.cpp uses sprintf (deprecated on macOS 15+)
TOOL_NOWARN = -Wno-writable-strings -Wno-deprecated-declarations

tools: $(BUILDDIR)/mkconfig $(BUILDDIR)/dosconv $(BUILDDIR)/mnudump \
       $(BUILDDIR)/mnuconv $(BUILDDIR)/mnu2json $(BUILDDIR)/datadump \
       $(BUILDDIR)/rawinput $(BUILDDIR)/test_bbs_path

$(BUILDDIR)/mkconfig: $(TOOLDIR)/mkconfig.c $(SRCDIR)/vardec.h $(SRCDIR)/cJSON.cpp $(SRCDIR)/json_io.cpp $(SRCDIR)/file_lock.cpp | $(BUILDDIR)
	$(MSG) CXX $@
	$(Q)$(CXX) -std=c++17 -fsigned-char -Isrc $(TOOL_NOWARN) -o $@ -x c++ $< $(SRCDIR)/cJSON.cpp $(SRCDIR)/json_io.cpp $(SRCDIR)/file_lock.cpp

$(BUILDDIR)/dosconv: $(TOOLDIR)/dosconv.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(MSG) CC $@
	$(Q)$(CC) -std=gnu89 -fsigned-char -Isrc -o $@ $<

$(BUILDDIR)/mnudump: $(TOOLDIR)/mnudump.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(MSG) CC $@
	$(Q)$(CC) -std=gnu89 -fsigned-char -Isrc -o $@ $<

$(BUILDDIR)/mnuconv: $(TOOLDIR)/mnuconv.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(MSG) CC $@
	$(Q)$(CC) -std=gnu89 -fsigned-char -Isrc -o $@ $<

$(BUILDDIR)/mnu2json: $(TOOLDIR)/mnu2json.c $(SRCDIR)/vardec.h $(SRCDIR)/menu_json.cpp $(SRCDIR)/menu_json.h $(SRCDIR)/cJSON.cpp $(SRCDIR)/json_io.cpp $(SRCDIR)/file_lock.cpp | $(BUILDDIR)
	$(MSG) CXX $@
	$(Q)$(CXX) -std=c++17 -fsigned-char -Isrc $(TOOL_NOWARN) -o $@ -x c++ $< $(SRCDIR)/menu_json.cpp $(SRCDIR)/cJSON.cpp $(SRCDIR)/json_io.cpp $(SRCDIR)/file_lock.cpp

$(BUILDDIR)/datadump: $(TOOLDIR)/datadump.c $(SRCDIR)/vardec.h | $(BUILDDIR)
	$(MSG) CC $@
	$(Q)$(CC) -std=gnu89 -fsigned-char -Isrc -o $@ $<

# Raw input byte inspector — standalone, no BBS dependencies
$(BUILDDIR)/rawinput: $(TOOLDIR)/rawinput.c | $(BUILDDIR)
	$(MSG) CC $@
	$(Q)$(CC) -std=gnu89 -o $@ $<

# BbsPath unit tests — standalone, no BBS dependencies
$(BUILDDIR)/test_bbs_path: tests/test_bbs_path.cpp $(SRCDIR)/bbs_path.cpp $(SRCDIR)/bbs_path.h | $(BUILDDIR)
	$(MSG) CXX $@
	$(Q)$(CXX) -std=c++17 -Isrc -o $@ tests/test_bbs_path.cpp $(SRCDIR)/bbs_path.cpp

# =================================================================
# BBS-entangled test tools (need BBS .o files, not library tests)
# =================================================================

# Terminal rendering test — links BBS .o files + libterminal
TERMTEST_OBJS = $(OBJDIR)/conio.o $(OBJDIR)/platform_stubs.o $(OBJDIR)/io_stream.o \
                $(OBJDIR)/terminal_bridge.o $(OBJDIR)/session.o $(OBJDIR)/system.o
$(BUILDDIR)/termtest: $(SRCDIR)/terminal/tests/termtest.c $(TERMTEST_OBJS) $(LIBTERMINAL) | $(BUILDDIR)
	$(MSG) LD $@
	$(Q)$(CXX) $(CXXFLAGS) -o $@ -x c++ $< -x none $(TERMTEST_OBJS) $(LIBTERMINAL) -lncurses

# Input function test — links BBS .o files + libterminal
INPUTTEST_OBJS = $(OBJDIR)/bbs_output.o $(OBJDIR)/bbs_input.o $(OBJDIR)/bbs_ui.o \
                 $(OBJDIR)/tcpio.o $(OBJDIR)/conio.o $(OBJDIR)/platform_stubs.o \
                 $(OBJDIR)/io_stream.o $(OBJDIR)/terminal_bridge.o $(OBJDIR)/stream_processor.o
$(BUILDDIR)/inputtest: $(TOOLDIR)/inputtest.c $(INPUTTEST_OBJS) $(LIBTERMINAL) | $(BUILDDIR)
	$(MSG) LD $@
	$(Q)$(CXX) $(CXXFLAGS) -o $@ -x c++ $< -x none $(INPUTTEST_OBJS) $(LIBTERMINAL) -lncurses

# =================================================================
# Data sync from dist/ into build/
# =================================================================

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

# =================================================================
# Generate config.json + seed data (first-time setup)
# =================================================================

init: $(BUILDDIR)/mkconfig $(BUILDDIR)/mnu2json
	@if [ ! -f $(BUILDDIR)/config.json ]; then \
		echo "Generating config.json and seed data..."; \
		cd $(BUILDDIR) && ./mkconfig .; \
	else \
		echo "config.json already exists (use 'make distclean' first to regenerate)"; \
	fi
	@$(MAKE) data
	@$(MAKE) menus-json

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

# =================================================================
# Install
# =================================================================

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/dominion
	install -d $(DESTDIR)$(PREFIX)/share/dominion
	@for d in afiles data menus msgs scripts; do \
		if [ -d $(BUILDDIR)/$$d ]; then \
			cp -R $(BUILDDIR)/$$d $(DESTDIR)$(PREFIX)/share/dominion/; \
		fi; \
	done

# =================================================================
# Clean targets
# =================================================================

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)
	rm -f $(LIBTERMINAL) $(LIBTUI) $(LIBJAM) $(LIBFILES) $(LIBUSER)
	rm -f $(BUILDDIR)/mkconfig $(BUILDDIR)/dosconv $(BUILDDIR)/mnudump
	rm -f $(BUILDDIR)/mnuconv $(BUILDDIR)/mnu2json $(BUILDDIR)/datadump $(BUILDDIR)/jamdump
	rm -f $(BUILDDIR)/termtest $(BUILDDIR)/rawinput $(BUILDDIR)/inputtest
	rm -f $(BUILDDIR)/iotest $(BUILDDIR)/uitest $(BUILDDIR)/formtest
	rm -f $(BUILDDIR)/test_bbs_path

clean-obj:
	rm -rf $(OBJDIR)

distclean:
	rm -rf $(BUILDDIR)

# =================================================================
# Help
# =================================================================

help:
	@echo 'Dominion BBS — Build System'
	@echo ''
	@echo 'Build targets:'
	@echo '  make              Build BBS + sync runtime data (release)'
	@echo '  make binary       Build BBS binary only (no data sync)'
	@echo '  make tools        Build standalone utilities'
	@echo '  make init         Generate config.json + seed data'
	@echo ''
	@echo 'Test targets:'
	@echo '  make test-terminal  Terminal library tests'
	@echo '  make test-tui       TUI library tests'
	@echo '  make test-jam       JAM message base test'
	@echo ''
	@echo 'Install:'
	@echo '  make install             Install to /usr/local'
	@echo '  make install PREFIX=DIR  Install to custom prefix'
	@echo ''
	@echo 'Options:'
	@echo '  DEBUG=1    Debug build (-O0 -g, no optimization)'
	@echo '  V=1        Verbose build output (show full commands)'
	@echo ''
	@echo 'Maintenance:'
	@echo '  make clean       Remove build artifacts (keep data)'
	@echo '  make clean-obj   Remove objects only (quick relink)'
	@echo '  make distclean   Remove entire build directory'
	@echo ''
	@echo 'Detected platform: $(PLATFORM)'

.PHONY: all binary clean clean-obj distclean data tools init menus-json \
        test-terminal test-tui test-jam install help

# =================================================================
# Auto-dependency tracking (generated by -MMD -MP)
# =================================================================

DEPS := $(wildcard $(OBJDIR)/*.d)
-include $(DEPS)
