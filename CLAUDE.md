# CLAUDE.md

## Design Principles

Follow Lakos design principles documented in the auto-memory file `lakos.md`: method vs utility placement, value types, levelization, minimal interface.

## Refactoring

When refactoring, refer to refactor.md

## Project Overview

Dominion BBS v3.1 Beta — a 1993 DOS BBS migrated to macOS/clang (64-bit). Migration, not rewrite. Compatibility layer (`platform.h` + `platform_stubs.c`) maps DOS/Borland APIs to POSIX.

## Rules

1. **Notes first, code second.** Check MEMORY.md topic files before reading any source. You've documented this codebase — use what you know.
2. **3-5 tool calls for simple tasks. 10-15 for medium. Past budget without a fix? Stop and rethink.**
3. **Parallel reads, not sequential.** Need 3 files? Read all 3 at once.
4. **No Explore agents for documented code.** Use Explore only for genuinely unseen code.
5. **Connect to the BBS and look before fixing behavioral bugs.** Plan keystrokes in advance. Single-keystroke input — no `\r` after menu selections.
6. **dist/ is frozen. vardec.h structs = binary file format. mkconfig is source of truth for generated files.**
7. **Never Read binary art files (.BIN, .ANS).** The bug is in the rendering code, not the file.
8. **Hex dump data files first, code second.** `hexdump -C file | head -40` + compare to struct offsets. Don't guess struct layouts.
9. **Smallest fix wins.** 5-line fix + 10-line test beats 500-line framework. This is a porting project.
10. **Verify end-to-end after every fix.** Build, connect, confirm user-visible behavior.

## Repository Layout

```
src/       — BBS source code
tools/     — Standalone utilities (mkconfig, dosconv, etc.)
dist/      — Clean-install reference data (frozen DOS artifacts)
tests/     — E2E test suite (pytest + TCP client)
docs/      — Original documentation
legacy/    — Original DOS build files and orphaned source modules
build/     — Build output (gitignored)
```

## Build Commands

```bash
make -j4                        # Full build (binary + config + runtime data)
make binary                     # Build only (no data copy)
make tools                      # Build standalone tools
make init                       # Regenerate Config.dat + seed data
make clean                      # Clean everything
make clean-obj                  # Clean objects only (quick relink)
cd build && ./dominion -M       # Run locally
cd build && ./dominion -P2023 -Q  # Run on TCP port, quit after one session
```

`make -j4` does everything: compile, mkconfig, Config.dat, dist/ copy. After clean build, `cd build && ./dominion -M` just works.

## Tests

E2E tests: pytest + TCP socket client. Build BBS, launch on ephemeral port, drive via telnet.

```bash
cd tests && source .venv/bin/activate && pytest
pytest tests/test_login.py
pytest tests/test_login.py::TestLogin::test_sysop_login_succeeds
pytest tests/ -v -s
```

Key infrastructure:
- `tests/conftest.py` — fixtures
- `tests/fixtures/build.py` — session-scoped: compile once
- `tests/fixtures/instance.py` — function-scoped: temp workdir in `/tmp/dom_*`, start/stop BBS
- `tests/bbs_client.py` — `BBSClient`: TCP + telnet IAC + ANSI stripping + `expect()`/`send_line()`
- `tests/ansi.py` — ANSI stripping

Config paths limited to 81 chars — temp dirs must use `/tmp` (not `/var/folders/...`).

## Conventions

- **C++17** (`-std=c++17`, `-x c++` on .c files). Tools stay C.
- **Signed chars:** `-fsigned-char` required (Borland default, opposite of clang).
- **Struct layout = binary files.** `vardec.h` uses explicit-width types + `#pragma pack(1)`. Changing field sizes breaks file compat.
- **Warning suppressions:** 30+ categories — intentional for legacy code.
- **Global state:** System/Session singletons. Single-session-per-process (forked).
- **String lengths:** Config paths 81, user names 31, passwords 21.
