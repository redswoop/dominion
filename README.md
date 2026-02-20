# Dominion BBS v3.1 Beta

A 1993 DOS Bulletin Board System ported to macOS and Linux. Originally built with Borland C++ 3.1 for 16-bit real-mode DOS; now compiles with clang or gcc on 64-bit POSIX systems.

## Prerequisites

- **C compiler**: clang (macOS) or gcc (Linux)
- **make**: GNU Make
- **Python 3 + pytest**: for the E2E test suite (optional)

## Quick Start

```bash
# Build everything (binary + config + runtime data)
make -j4

# Run in local mode
cd build && ./dominion -M
```

## Running Modes

| Flag | Mode |
|------|------|
| `-M` | Local mode — direct keyboard/screen I/O |
| `-P<port>` | TCP mode — listen for telnet connections on `<port>` |
| `-Q` | Quit after one session (combine with `-P` for testing) |

Example: `cd build && ./dominion -P2023` to serve on port 2023.

## Building

```bash
make -j4          # Build BBS + generate config + copy runtime data
make binary       # Build BBS binary only (no data sync)
make tools        # Build standalone utilities (mkconfig, dosconv)
make init         # Regenerate Config.dat + seed data
make clean        # Remove everything in build/
make clean-obj    # Remove objects only (keep binary + data)
```

## Testing

E2E tests use pytest with a TCP socket client. Tests build the BBS, launch it on an ephemeral port, and drive it via telnet protocol.

```bash
cd tests
source .venv/bin/activate
pytest              # Run all tests
pytest -v -s        # Verbose with stdout
pytest test_login.py::TestLogin::test_sysop_login_succeeds  # Single test
```

## Directory Layout

| Directory | Contents |
|-----------|----------|
| `src/` | BBS source code — C modules and headers compiled into `build/dominion` |
| `tools/` | Standalone utilities — `mkconfig` (config generator), `dosconv` (DOS data converter), etc. |
| `dist/` | Clean-install reference data from v3.0 — ANSI art, menus, seed data, modem scripts |
| `tests/` | E2E test suite (pytest + TCP client) |
| `docs/` | Original documentation — user manual, command reference, release notes |
| `legacy/` | Original DOS build files and orphaned source modules |
| `build/` | Build output (gitignored) — binary, objects, self-contained runtime |

## Data Files

The BBS uses binary data files with fixed-size struct records. There are two kinds:

- **Platform-independent** (text, simple formats): shipped in `dist/` and copied to `build/` by `make`
- **Struct-layout-dependent** (Config.dat, user records, sub/dir definitions): generated at runtime by `mkconfig`, which writes them with the host platform's struct layout

The `dosconv` tool converts DOS-format data files to the macOS/Linux struct layout for migrating existing BBS installations.
