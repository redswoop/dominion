"""Function-scoped BBS instance fixture: temp workdir, start/stop BBS process."""

import os
import shutil
import signal
import socket
import subprocess
import tempfile
import time
import pathlib

import pytest

from ..bbs_client import BBSClient

DOMINION_DIR = pathlib.Path(__file__).resolve().parent.parent.parent

# Source directories containing static assets needed by the BBS
STATIC_DIRS = ["afiles", "menus"]
# Individual static data files that mkconfig does NOT generate
STATIC_DATA_FILES = ["STRINGS.DAT", "SYSSTR.DAT"]


def _free_port():
    """Get a free TCP port by briefly binding to port 0."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(('127.0.0.1', 0))
        return s.getsockname()[1]


class _FileCapture:
    """Poll a log file for output content."""

    def __init__(self, path):
        self._path = pathlib.Path(path)

    @property
    def data(self):
        try:
            return self._path.read_bytes()
        except FileNotFoundError:
            return b''

    @property
    def text(self):
        return self.data.decode('latin-1', errors='replace')

    def wait_for(self, pattern, timeout=10):
        """Wait until pattern appears in file content."""
        if isinstance(pattern, str):
            pattern = pattern.encode('latin-1')
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if pattern in self.data:
                return True
            time.sleep(0.1)
        return False


class BBSInstance:
    """A running BBS instance for testing."""

    def __init__(self, process, port, workdir, stdout_cap, stderr_cap):
        self.process = process
        self.port = port
        self.workdir = workdir
        self._stdout = stdout_cap
        self._stderr = stderr_cap

    def create_client(self, timeout=10):
        """Create and connect a BBSClient to this instance."""
        client = BBSClient('127.0.0.1', self.port, timeout=timeout)
        client.connect()
        return client

    def stop(self, timeout=5):
        """Stop the BBS process. SIGTERM first, then SIGKILL."""
        if self.process.poll() is not None:
            return self.process.returncode

        self.process.send_signal(signal.SIGTERM)
        try:
            self.process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            self.process.kill()
            self.process.wait(timeout=2)
        return self.process.returncode

    @property
    def running(self):
        return self.process.poll() is None

    @property
    def stdout_text(self):
        return self._stdout.text

    @property
    def stderr_text(self):
        return self._stderr.text


@pytest.fixture
def bbs_instance(bbs_binary, mkconfig_binary):
    """Per-test clean BBS instance.

    1. Run mkconfig to generate data files in a short temp path
    2. Copy static assets from source tree
    3. Start BBS on a free port with -P{port} -Q
    4. Wait for "TCP listening" in stdout (avoids consuming a connection)
    5. Yield BBSInstance
    6. Teardown: stop process, remove temp dir
    """
    # Use /tmp with short prefix — BBS config paths are limited to 81 chars,
    # so pytest's deep tmp_path won't work.
    # Force /tmp — Python's tempfile uses /var/folders/... on macOS which is
    # too long for the BBS's 81-char path fields in Config.dat.
    workdir = pathlib.Path(tempfile.mkdtemp(prefix="dom_", dir="/tmp"))

    # --- Generate data files via mkconfig ---
    result = subprocess.run(
        [str(mkconfig_binary), str(workdir) + "/"],
        capture_output=True,
        timeout=10,
    )
    if result.returncode != 0:
        pytest.fail(
            f"mkconfig failed:\n"
            f"{result.stdout.decode('latin-1', errors='replace')}\n"
            f"{result.stderr.decode('latin-1', errors='replace')}"
        )

    # --- Copy static directories (afiles, menus) ---
    for dirname in STATIC_DIRS:
        src = DOMINION_DIR / "dist" / dirname
        dst = workdir / dirname
        if src.is_dir():
            # mkconfig already created the dir; copy contents into it
            if dst.exists():
                shutil.rmtree(dst)
            shutil.copytree(str(src), str(dst))

    # --- Copy static data files that mkconfig doesn't generate ---
    data_dir = workdir / "data"
    src_data = DOMINION_DIR / "dist" / "data"
    for fname in STATIC_DATA_FILES:
        src_file = src_data / fname
        if src_file.exists():
            shutil.copy2(str(src_file), str(data_dir / fname))

    # --- Copy DOM.KEY ---
    dom_key = DOMINION_DIR / "dist" / "DOM.KEY"
    if dom_key.exists():
        shutil.copy2(str(dom_key), str(workdir / "DOM.KEY"))

    # --- Start BBS ---
    port = _free_port()
    # IMPORTANT: Do NOT use /dev/null for stdin.  The BBS uses select() on
    # STDIN to detect local keyboard input (kbhitb()).  /dev/null is always
    # readable (returns EOF), which makes kbhitb() return TRUE forever,
    # causing the BBS to read garbage from stdin instead of checking the TCP
    # socket.  A pipe with no writer blocks correctly in select().
    stdin_read, stdin_write = os.pipe()
    # Redirect stdout/stderr to files instead of pipes — the BBS does
    # terminal ioctls that behave differently with pipes vs files/terminals.
    stdout_file = open(str(workdir / "bbs_stdout.log"), "wb")
    stderr_file = open(str(workdir / "bbs_stderr.log"), "wb")
    proc = subprocess.Popen(
        [str(bbs_binary), f"-P{port}", "-Q"],
        cwd=str(workdir),
        stdin=stdin_read,
        stdout=stdout_file,
        stderr=stderr_file,
    )
    # Keep stdin_write open — if we close it, the pipe returns EOF which
    # makes select() report it as readable (same problem as /dev/null).
    # With the write end open but no data, select() correctly blocks.
    stdout_file.close()
    stderr_file.close()

    # Poll stdout file for the "TCP listening" message
    stdout_cap = _FileCapture(workdir / "bbs_stdout.log")
    stderr_cap = _FileCapture(workdir / "bbs_stderr.log")

    # Wait for BBS to report it's listening (don't probe the port — that
    # would consume the one connection allowed by -Q mode)
    listen_msg = f"TCP listening on port {port}"
    if not stdout_cap.wait_for(listen_msg, timeout=15):
        proc.kill()
        proc.wait()
        pytest.fail(
            f"BBS did not print '{listen_msg}' within 15s.\n"
            f"Process rc: {proc.returncode}\n"
            f"stdout: {stdout_cap.text[-2000:]}\n"
            f"stderr: {stderr_cap.text[-2000:]}"
        )

    instance = BBSInstance(proc, port, workdir, stdout_cap, stderr_cap)

    yield instance

    # --- Teardown ---
    instance.stop()
    os.close(stdin_read)
    os.close(stdin_write)
    shutil.rmtree(str(workdir), ignore_errors=True)


@pytest.fixture
def bbs_client(bbs_instance):
    """Convenience fixture: connected BBSClient, auto-closed on teardown."""
    client = bbs_instance.create_client()
    yield client
    client.close()
