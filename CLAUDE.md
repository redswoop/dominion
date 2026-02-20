# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Dominion BBS v3.1 Beta — a 1993 DOS Bulletin Board System ported to macOS/clang (64-bit). Originally built with Borland C++ 3.1 for real-mode 16-bit DOS. The port uses a compatibility layer (`src/platform.h` + `src/platform_stubs.c`) to map DOS/Borland APIs to POSIX equivalents.

## Repository Layout

```
src/       — BBS source code (48 .c modules + 18 .h headers)
tools/     — Standalone utilities (mkconfig, dosconv, etc.)
dist/      — Clean-install reference data (from v3.0 snapshot)
tests/     — E2E test suite (pytest + TCP client)
docs/      — Original documentation
legacy/    — Original DOS build files and orphaned source modules
build/     — Build output (gitignored)
```

## Build Commands

```bash
# Full build (binary + config + runtime data — ready to run)
make -j4

# Build only (no data copy)
make binary

# Build standalone tools (mkconfig, dosconv)
make tools

# Regenerate Config.dat + seed data (run after make clean)
make init

# Clean everything
make clean

# Clean objects only (keep binary + data for quick relink)
make clean-obj

# Run locally (must cd into build/)
cd build && ./dominion -M

# Run on specific TCP port, quit after one session
cd build && ./dominion -P2023 -Q
```

**Build output:** `build/dominion` (binary), `build/obj/` (objects), `build/` (self-contained runtime with data dirs)

**Note:** `make` (or `make -j4`) does everything: compiles the BBS, builds mkconfig, generates Config.dat + seed data if missing, and copies dist/ files into build/. After a clean build, `cd build && ./dominion -M` just works.

## Tests

E2E tests using pytest + TCP socket client. Tests build the BBS, launch it on an ephemeral port, and drive it via telnet.

```bash
# Run all tests
cd tests && source .venv/bin/activate && pytest

# Run a single test file
pytest tests/test_login.py

# Run a single test
pytest tests/test_login.py::TestLogin::test_sysop_login_succeeds

# Verbose with stdout
pytest tests/ -v -s
```

The test venv is at `tests/.venv/`. Key test infrastructure:
- `tests/conftest.py` — wires fixtures
- `tests/fixtures/build.py` — session-scoped: compiles `dominion` and `mkconfig` once per run
- `tests/fixtures/instance.py` — function-scoped: creates temp workdir in `/tmp/dom_*`, runs `mkconfig`, starts BBS on free port with `-P{port} -Q`, tears down after each test
- `tests/bbs_client.py` — `BBSClient` class: TCP client with telnet IAC handling, ANSI stripping, `expect()`/`send_line()` pattern
- `tests/ansi.py` — ANSI escape code stripping

**Important:** BBS config paths are limited to 81 chars, so temp dirs must use `/tmp` (not macOS default `/var/folders/...`). The test fixture handles this.

## Architecture

### Platform Compatibility Layer

The core porting strategy — maps all DOS/Borland APIs to POSIX:

- **`src/platform.h`** — `#define` redirects and type stubs: removes `far`/`near`/`huge`, maps `stricmp` → `strcasecmp`, `O_BINARY` → 0, `farmalloc` → `malloc`, DOS conio functions, `findfirst`/`findnext`, etc.
- **`src/platform_stubs.c`** — implementations: `kbhit()`, `getch()`, `gotoxy()`, `textcolor()`, `findfirst()`/`findnext()`, `filelength()`, `itoa()`/`ltoa()`, `spawnvpe()`, `int86()`
- **`src/jam_stubs.c`** — JAM message base file locking stubs (DOS lock/unlock → no-ops)

**Name conflict redirects** in `platform.h`: `yn` → `bbs_yn`, `y0`/`y1` → `bbs_y0`/`bbs_y1` (Bessel functions in math.h), `wait` → `bbs_wait` (POSIX wait), `pipe` → `bbs_pipe`, `strnstr` → `bbs_strnstr`.

### Key Header Files

| Header | Role |
|--------|------|
| `src/vardec.h` | All data structures (`userrec`, `configrec`, `subboardrec`, `directoryrec`, `statusrec`, etc.) with explicit-width types (`INT16`, `INT32`, `UINT16`, `UINT32`, `CHAR8`, `UCHAR8`) for binary file compatibility |
| `src/fcns.h` | Master function prototype file (~700 prototypes) |
| `src/vars.h` | All global variables (uses `#ifdef _DEFINE_GLOBALS_` pattern) |
| `src/platform.h` | DOS→POSIX compatibility layer |
| `src/jam.h` / `src/jamprot.h` / `src/jamsys.h` | JAM message base format types and prototypes |

### Major Subsystems

All source files are in `src/`:

- **Communication** (`com.c`, `x00com.c`, `modem.c`) — ANSI/ASCII terminal output, telnet negotiation (ECHO, SGA, NAWS), color handling, serial/TCP I/O
- **Messages** (`mm.c`, `mm1.c`, `mm2.c`, `msgbase.c`) — message reading, posting, quoting, threading across subboards
- **JAM message base** (`jam.c`, `jamsub.c`) — JAM format (.jhr/.jdt/.jdx/.jlr files) for message storage
- **File transfers** (`file.c`, `file1.c`, `file2.c`, `file3.c`, `filesys.c`, `archive.c`) — upload/download areas, batch operations, external protocols
- **User system** (`disk.c`, `lilo.c`, `newuser.c`, `uedit.c`) — login/logoff, user records, new user registration, user editing
- **Menus & UI** (`menued.c`, `conio.c`, `bbsutl.c`, `bbsutl2.c`) — dynamic menu system, screen drawing, MCI code expansion
- **Config & admin** (`config.c`, `xinit.c`, `sysopf.c`) — system configuration, initialization, sysop functions
- **Networking** (`fido.c`, `multline.c`, `multmail.c`) — FidoNet routing, multi-node support
- **Misc** (`extrn.c` — door/external program support, `chat.c` — sysop chat, `timest.c` — time/event system)

### Standalone Tools

In `tools/`:

- **`mkconfig.c`** — generates Config.dat + seed data files for a fresh install
- **`dosconv.c`** — converts DOS binary data files to macOS/Linux struct layout
- Other utilities: `makekey.c`, `mc.c`, `quicklog.c`, `install.c`, `rststat.c`

### Data Files

All runtime data lives in `build/` when running. Binary formats with struct-aligned records:
- `Config.dat` — `configrec` + `niftyrec` (system configuration)
- `data/Status.dat` — `statusrec` (system status/counters)
- `data/user.lst` — array of `userrec` (user records, 1-indexed)
- `data/user.idx` — array of `smalrec` (name→number index)
- `data/SUBS.DAT` — array of `subboardrec` (message areas, max 200)
- `data/DIRS.DAT` — array of `directoryrec` (file areas, max 200)

Platform-independent data (ANSI art, menus, text seed files) lives in `dist/` and is copied to `build/` by `make`. Struct-layout-dependent files are generated at runtime by `tools/mkconfig.c`.

## Important Conventions

- **C standard:** GNU89 (`-std=gnu89`). Not C99/C11 — no mixed declarations and code, no `//` comments in some contexts.
- **Signed chars:** `-fsigned-char` is required — the code assumes `char` is signed (Borland default, opposite of clang default).
- **Struct layout matters:** Data structures in `src/vardec.h` map directly to binary files on disk. Changing field sizes or order breaks file compatibility.
- **Warning suppressions:** The Makefile suppresses 30+ warning categories — this is intentional for legacy code compatibility, not sloppiness.
- **Global state:** Heavy use of globals declared in `src/vars.h`. The BBS is fundamentally single-session-per-process.
- **String lengths:** Config path fields are 81 chars max. User names 31 chars. Passwords 21 chars. These limits come from the fixed-size struct fields.
