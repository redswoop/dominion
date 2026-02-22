# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Refactoring

When refactoring, refer to refactor.md

## Project Overview

Dominion BBS v3.1 Beta — a 1993 DOS Bulletin Board System being migrated to macOS/clang (64-bit). Originally built with Borland C++ 3.1 for real-mode 16-bit DOS.

**This is a migration, not a rewrite.** The goal is to make the original C code compile and run on modern macOS with minimal changes. The code is ugly, full of globals, and uses 1993 patterns — that's fine. We're not cleaning it up, we're making it work. The compatibility layer (`src/platform.h` + `src/platform_stubs.c`) maps DOS/Borland APIs to POSIX equivalents so the original source can compile largely unchanged.

## How to Work — READ THIS FIRST

You have been working on this codebase across many sessions. You have extensive notes in your memory files: architecture, structs, I/O layer, menu system, crash patterns. **Use them.**

### Before touching any tool, think:

1. **What do I already know?** Check MEMORY.md — source file map, function cheat sheets, struct layouts, globals. Check the topic files (architecture.md, structs.md, menu-system.md, io-layer.md). You probably already know which 2-3 files matter.
2. **What's my hypothesis?** State it in one sentence. "The color codes are being emitted without ESC bytes." "The struct is 4 bytes too long." If you can't form a hypothesis, you need ONE piece of information — go get that specific thing.
3. **What's the minimum action to test it?** Usually: read 2-3 files (in parallel), or connect to the BBS once, or hexdump one file. Not 20 sequential reads.
4. **Do it, fix it, verify it.**

### Context is expensive. Be surgical.

- **Parallel reads, not sequential.** If you need mm1.c, com.c, and bbsutl.c, read all three at once. Don't read one, follow a call chain, read the next. You already know the call chains — they're in your notes.
- **No Explore subagents for known code.** You've mapped this codebase. Launching an Explore agent to breadth-search files you've already documented is procrastination. Use Explore only for code you've genuinely never seen.
- **Budget yourself.** If you've made 10 tool calls and haven't formed a hypothesis yet, stop. Re-read your notes. Think. The answer is probably in something you already know.
- **One BBS connection, planned in advance.** Before connecting, decide: what login steps, what keystrokes, what output am I looking for? The BBS uses single-keystroke input (onek/getkey) — don't send \r after menu selections.

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

### Always Look Before You Fix

The BBS is a text stream over TCP. You can connect to it, type commands, and read the output — just like a user would. **This is your most powerful debugging tool.**

When something is reported broken (e.g., "menus aren't working", "login fails"):

0. **Think first.** Check your memory files. What subsystem is this? Which files? What's your hypothesis? Don't open a single file until you have a direction.
1. **Read the minimum source** to confirm or refine your hypothesis. 2-3 files, in parallel. Not 10 sequential reads following call chains you already know.
2. **Build and connect once.** `make -j4`, launch, connect with a plan (know your keystrokes in advance). Look at what the BBS actually shows.
3. **Fix it.** Smallest change that addresses the root cause.
4. **Connect again and verify.** Confirm your fix works by driving the BBS through the same path.

The existing test infrastructure makes this trivial:
- `tests/bbs_client.py` — `BBSClient` with `expect()`/`send_line()` handles telnet negotiation and ANSI stripping
- `tests/fixtures/instance.py` — spins up a fresh BBS instance on an ephemeral port and tears it down

**Write E2E tests, not just unit tests.** A pytest that launches the BBS, logs in, navigates to the broken thing, and asserts on the output is worth more than any amount of code inspection. The BBS produces text — you can read text. Use that.

**After every fix, verify it end-to-end.** Don't just check that the code compiles or that a struct size is right. Connect to the running BBS and confirm the user-visible behavior is correct. It takes 30 seconds.

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

Platform-independent data (ANSI art, text files) lives in `dist/` and is copied to `build/` by `make`. Struct-layout-dependent binary files are generated fresh by `tools/mkconfig.c`.

### Data File Pipeline — READ THIS

There are two worlds of binary data, and confusing them is the #1 source of spinning:

```
DOS world (16-bit, Borland)          macOS world (64-bit, clang)
─────────────────────────            ────────────────────────────
dist/ files                          build/ files
  Original binary data                 What the running BBS reads
  Frozen artifacts — NEVER modify      Generated by mkconfig or dosconv
  Struct layout: Borland 16-bit        Struct layout: matches vardec.h
  May have different padding,          sizeof() == file record size
  field widths, or alignment
```

**The pipeline:**
```
dist/*.dat  ──→  dosconv/mkconfig  ──→  build/*.dat  ──→  BBS reads via struct
(DOS binary)     (conversion)           (macOS binary)     (vardec.h layout)
```

**Some dist/ files are plain text** (ANSI art, .MSG files, .FMT files) — these copy straight to build/ with no conversion needed.

**Some dist/ files are binary** (Config.dat seeds, Status.dat, menus) — these have DOS struct layouts that may differ from the macOS structs. They need conversion or regeneration.

**Some files are generated fresh** — mkconfig creates Config.dat, Status.dat, user.lst, user.idx, SUBS.DAT, DIRS.DAT from scratch. These never come from dist/ at all.

**Menus are a special case:** dist/menus/ are OLD v2.x format (312-byte header, 189-byte commands). The BBS code expects NEW v3.x format (471-byte header, 217-byte commands). These need format conversion via mnuconv, not just struct re-padding.

### Debugging Data File Issues

When something crashes or misbehaves at runtime and you suspect a data file:

**Step 1: Hex dump the file.** `hexdump -C build/path/to/file | head -40`. What bytes are actually there?

**Step 2: Check the struct.** Read the struct definition in `vardec.h`. What fields, what widths, what offsets does the code expect? Use `sizeof()` or count manually.

**Step 3: Compare.** Does the file size match? Is it `sizeof(struct) * N` for an array file? Do the bytes at known offsets contain sensible values (e.g., does the sysop name appear at the right offset)?

**Step 4: Fix the right layer.** Is the conversion wrong? Fix dosconv/mkconfig. Is the struct wrong? Fix vardec.h (carefully — it affects all files). Is the dist/ file just not what we thought? Document it and adjust the conversion.

**DO NOT:**
- Build elaborate validation tools — a 10-line `hexdump` + `sizeof()` check catches 90% of issues
- Modify dist/ files — they are frozen DOS artifacts
- Guess at struct layouts — count the bytes in vardec.h, or write a 5-line C program that prints `offsetof()` for each field
- Spin for more than 10 minutes without hex-dumping the actual file

## Important Conventions

- **C standard:** GNU89 (`-std=gnu89`). Not C99/C11 — no mixed declarations and code, no `//` comments in some contexts.
- **Signed chars:** `-fsigned-char` is required — the code assumes `char` is signed (Borland default, opposite of clang default).
- **Struct layout matters:** Data structures in `src/vardec.h` map directly to binary files on disk. Changing field sizes or order breaks file compatibility.
- **Warning suppressions:** The Makefile suppresses 30+ warning categories — this is intentional for legacy code compatibility, not sloppiness.
- **Global state:** Heavy use of globals declared in `src/vars.h`. The BBS is fundamentally single-session-per-process.
- **String lengths:** Config path fields are 81 chars max. User names 31 chars. Passwords 21 chars. These limits come from the fixed-size struct fields.

## Invariants — Do Not Violate

1. **dist/ is frozen.** Never modify files in dist/. They are the original DOS artifacts. If a dist/ file doesn't match what you expect, the problem is your expectation or the conversion layer, not the dist/ file.

2. **vardec.h structs = binary file format.** The structs use explicit-width types (INT16, UINT16, INT32, CHAR8, etc.) and `#pragma pack(1)` so that `sizeof(struct)` exactly matches the binary record size on disk. If you change a struct, every file that uses it breaks.

3. **mkconfig is the source of truth for generated files.** Config.dat, Status.dat, user.lst, user.idx, SUBS.DAT, DIRS.DAT are all created by mkconfig. If these files are wrong, fix mkconfig — don't hand-edit the binary.

4. **Hex dump first, code second.** When a data file issue appears, always hex-dump the actual file before writing any code. Most problems are visible in 5 minutes of `hexdump -C` + comparing to struct field offsets.

5. **Smallest fix wins — for investigation AND code.** This is a porting project. A 5-line fix + a 10-line test beats a 500-line validation framework. But also: 3 parallel file reads + 1 BBS connection beats 20 sequential reads + 3 failed connections. The investigation should be as minimal as the fix.

6. **Look at it running.** Before fixing a reported problem, connect to the BBS and see the problem with your own eyes. After fixing it, connect again and confirm it's fixed. The BBS is a text stream — there is no excuse for not verifying end-to-end.

7. **Your notes are the first source, not the code.** MEMORY.md and its topic files document the architecture, structs, I/O path, menu system, and known crash patterns. Consult them before reading source files. If you're re-reading a file you've already documented, you're wasting time.

8. **ANSI art files are opaque binary assets.** TheDraw `.BIN`/`.ANS` art files must NEVER be hex-dumped or parsed. The rendering bug is always in the code that processes them (`printfile` → `outchr` pipeline in `com.c`), not in the files themselves. Treat art files like you'd treat a JPEG — you debug the image viewer, not the pixel data.
