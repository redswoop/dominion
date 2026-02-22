"""Session-scoped build fixtures: compile dominion and mkconfig once per test run."""

import subprocess
import pytest

# Paths relative to the dominion source tree
DOMINION_SRC = __file__
# Walk up: fixtures/ -> tests/ -> dominion/
import pathlib
DOMINION_DIR = pathlib.Path(__file__).resolve().parent.parent.parent


@pytest.fixture(scope="session")
def bbs_binary():
    """Build the dominion BBS binary. Returns path to build/dominion."""
    result = subprocess.run(
        ["make", "-j4"],
        cwd=str(DOMINION_DIR),
        capture_output=True,
        text=True,
        timeout=120,
    )
    if result.returncode != 0:
        pytest.fail(
            f"BBS build failed (rc={result.returncode}):\n"
            f"stdout: {result.stdout[-2000:]}\n"
            f"stderr: {result.stderr[-2000:]}"
        )
    binary = DOMINION_DIR / "build" / "dominion"
    assert binary.exists(), f"Binary not found at {binary}"
    return binary


@pytest.fixture(scope="session")
def mkconfig_binary():
    """Compile mkconfig.c. Returns path to build/mkconfig."""
    out = DOMINION_DIR / "build" / "mkconfig"
    result = subprocess.run(
        [
            "cc", "-std=gnu89", "-DPD", "-fsigned-char",
            "-I", str(DOMINION_DIR / "src"),
            "-o", str(out),
            str(DOMINION_DIR / "tools" / "mkconfig.c"),
            str(DOMINION_DIR / "src" / "cJSON.c"),
            str(DOMINION_DIR / "src" / "json_io.c"),
        ],
        capture_output=True,
        text=True,
        timeout=30,
    )
    if result.returncode != 0:
        pytest.fail(
            f"mkconfig build failed:\n"
            f"stdout: {result.stdout}\n"
            f"stderr: {result.stderr}"
        )
    assert out.exists(), f"mkconfig not found at {out}"
    return out


@pytest.fixture(scope="session")
def datadump_binary():
    """Compile datadump.c. Returns path to build/datadump."""
    out = DOMINION_DIR / "build" / "datadump"
    result = subprocess.run(
        [
            "cc", "-std=gnu89", "-DPD", "-fsigned-char",
            "-I", str(DOMINION_DIR / "src"),
            "-o", str(out),
            str(DOMINION_DIR / "tools" / "datadump.c"),
        ],
        capture_output=True,
        text=True,
        timeout=30,
    )
    if result.returncode != 0:
        pytest.fail(
            f"datadump build failed:\n"
            f"stdout: {result.stdout}\n"
            f"stderr: {result.stderr}"
        )
    assert out.exists(), f"datadump not found at {out}"
    return out
