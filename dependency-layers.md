# Dependency Layers — Dominion BBS

Consult this before any structural change. If your change alters the
layering, update this file as part of the proposal.

```
Layer 0   Platform / System
          ─────────────────────────────────────────────────
          POSIX headers, <ncurses.h>, <termios.h>, etc.

Layer 1   Compatibility & Base Types
          ─────────────────────────────────────────────────
          platform.h          DOS→POSIX mapping
          vardec_types.h      INT32/UINT32/INT16/UINT16/CHAR8/UCHAR8
          jamsys.h            JAM platform detection
          cp437.h             CP437→UTF-8 table (static data, no deps)

Layer 2   Type Hierarchies (pure struct definitions)
          ─────────────────────────────────────────────────
          vardec_user.h       userrec, slrec, valrec, flags
          vardec_config.h     configrec, statusrec, niftyrec
          vardec_msgfile.h    subboardrec, directoryrec, postrec, ...
          vardec_ui.h         mmrec, menurec, screentype, ...
          fido.h              FidoNet address structs
          jam.h → jamsys.h    JAM format definitions
          jammb.h → jam.h     JAM message base C API

Layer 3   Leaf I/O & Data Modules (no BBS globals)
          ─────────────────────────────────────────────────
      ┌─► terminal.h/.cpp     Clean terminal I/O (C++ leaf)
      │                       Deps: <termios.h>, <ncurses.h>, cp437.h
      │                       Owns: dual streams, screen buffer, ANSI,
      │                             telnet IAC, keyboard, cursor
      │                       Test: tools/iotest.cpp (zero stubs)
      │
      ├── io_stream.h/.c      io_session_t struct + compat macros
      │                       Deps: <termios.h>
      │
      ├── io_ncurses.h/.c     ncurses init/shutdown, color mapping
      │                       Deps: <ncurses.h>, io_stream.h, cp437.h
      │
      ├── cJSON.h/.c          Third-party JSON parser (pure C library)
      │
      ├── json_io.h/.c        JSON ↔ binary struct conversion
      │                       Deps: cJSON.h, vardec.h
      │
      ├── menudb.h/.c         Menu file I/O (struct ↔ .mnu files)
      │                       Deps: vardec_ui.h, platform.h
      │
      ├── userdb.h/.c         User record I/O (struct ↔ files)
      │                       Deps: vardec_user.h, json_io.h
      │
      ├── jam_stubs.c         JAM file locking (POSIX flock)
      │                       Deps: platform.h, jammb.h
      │
      └── platform_stubs.c    DOS conio stubs (textcolor, gotoxy, ...)
                              Deps: io_ncurses.h, platform.h, io_stream.h

Layer 4   Umbrella Headers (prototypes + global state)
          ─────────────────────────────────────────────────
          fcns.h              Master function prototypes
                              Includes: platform.h, vardec.h, jammb.h,
                                        fcns_io.h, fcns_user.h,
                                        fcns_msg.h, fcns_file.h,
                                        fcns_sys.h

          vars.h              ALL global variables + compat macros
                              Includes: platform.h, fcns.h, io_stream.h
                              41 .c files include this

Layer 5   BBS Application Modules
          ─────────────────────────────────────────────────
          All 41 files that #include vars.h:

          Session:    bbs.c lilo.c xinit.c wfc.c error.c
          I/O:        com.c tcpio.c conio.c
          Menus:      mm.c mm1.c mm2.c menued.c
          UI:         bbsutl.c bbsutl2.c
          Messages:   msgbase.c jam.c
          Files:      file.c file1.c file2.c file3.c filesys.c archive.c
          Users:      disk.c newuser.c uedit.c
          Config:     config.c sysopf.c
          Network:    fido.c multline.c multmail.c
          Misc:       extrn.c chat.c timest.c utility.c utility1.c
                      personal.c misccmd.c stringed.c nuv.c dv.c
                      regis.c qwk.c subedit.c diredit.c
```

## Dependency Rules

1. **Downward only.** A component at layer N depends only on layers 0..(N-1).
2. **No cycles.** If adding a dependency would create one, extract a new
   lower-level component.
3. **terminal.h is a leaf.** It must NEVER include vars.h, vardec.h, fcns.h,
   or any BBS header. Its only non-system dependency is cp437.h.
4. **Layer 3 modules are isolated.** They don't include vars.h. They can be
   tested and linked independently of the BBS.
5. **vars.h is Layer 4.** Anything that includes vars.h is Layer 5 (BBS app).
   Goal: shrink Layer 5 by pushing logic down into Layer 3 modules.

## Include Ordering

Files that use ncurses must include `io_ncurses.h` BEFORE `vars.h`:
- com.c, conio.c, lilo.c, platform_stubs.c, xinit.c

This is because ncurses.h defines `echo()` and `getch()` as macros, and
io_ncurses.h undefs them before io_stream.h redefines `echo` as `io.echo`.

## Migration Direction

```
Phase 1 (done):  terminal.h/.cpp — clean leaf, proves IO independence
                  iotest.cpp links terminal.o only (zero BBS objects)

Phase 2 (next):  com.c/tcpio.c/conio.c use Terminal internally
                  Their vars.h dependency shrinks to BBS-specific code
                  (MCI, macros, palette lookup, pause, skey)

Phase 3:         Split com.c into bbs_io.cpp (BBS text processing)
                  + Terminal (pure I/O). com.c dies.

Phase 4:         conio.c splits: topscreen/skey → sysop_ui.cpp,
                  screen ops already in Terminal
```
