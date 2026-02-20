"""BBSClient — raw TCP client with telnet IAC handling for E2E tests."""

import socket
import time

from .ansi import strip_ansi, strip_to_plain

# Telnet IAC constants
IAC = 0xFF
WILL = 0xFB
WONT = 0xFC
DO = 0xFD
DONT = 0xFE
SB = 0xFA
SE = 0xF0

# Telnet option codes
TELOPT_ECHO = 1
TELOPT_SGA = 3
TELOPT_NAWS = 31

# ANSI cursor position query: ESC[6n
CPR_QUERY = b'\x1b[6n'
# Our response: row 1, col 1
CPR_RESPONSE = b'\x1b[1;1R'


class BBSClient:
    """TCP client that handles telnet IAC negotiation and ANSI detection.

    Decodes as latin-1 to preserve CP437 byte values without errors.
    Provides expect/send pattern for driving BBS interactions.
    """

    def __init__(self, host, port, timeout=10):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.sock = None
        self.buffer = b''        # raw bytes not yet consumed
        self.text_buffer = ''    # decoded text after IAC/ANSI filtering
        self._all_text = ''      # cumulative log of all received text

    def connect(self, retries=20, delay=0.25):
        """Connect with retry loop to handle BBS startup delay."""
        for i in range(retries):
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.settimeout(self.timeout)
                s.connect((self.host, self.port))
                self.sock = s
                return
            except (ConnectionRefusedError, OSError):
                if i == retries - 1:
                    raise
                time.sleep(delay)

    def close(self):
        """Close the socket."""
        if self.sock:
            try:
                self.sock.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            self.sock.close()
            self.sock = None

    def _recv_raw(self, timeout=None):
        """Read available bytes from socket, return them."""
        t = timeout if timeout is not None else self.timeout
        self.sock.settimeout(t)
        try:
            data = self.sock.recv(4096)
            if not data:
                return b''
            return data
        except socket.timeout:
            return b''

    def _filter_telnet(self, data):
        """Strip telnet IAC sequences from raw bytes, return clean bytes.

        The BBS does NOT implement IAC escaping (no IAC doubling for literal
        0xFF bytes), so we only strip recognized telnet commands
        (WILL/WONT/DO/DONT/SB..SE). Bare 0xFF bytes are passed through.
        """
        out = bytearray()
        i = 0
        while i < len(data):
            b = data[i]
            if b == IAC:
                if i + 1 >= len(data):
                    # Incomplete IAC at end — save for next read
                    self.buffer = bytes(data[i:])
                    return bytes(out)
                cmd = data[i + 1]
                if cmd in (WILL, WONT, DO, DONT):
                    if i + 2 >= len(data):
                        self.buffer = bytes(data[i:])
                        return bytes(out)
                    opt = data[i + 2]
                    # Auto-respond: decline anything we're asked to DO
                    if cmd == DO:
                        self._send_raw(bytes([IAC, WONT, opt]))
                    i += 3
                elif cmd == SB:
                    # Skip until IAC SE
                    j = i + 2
                    while j < len(data) - 1:
                        if data[j] == IAC and data[j + 1] == SE:
                            break
                        j += 1
                    i = j + 2
                else:
                    # Unknown command or IAC IAC — pass the 0xFF through
                    out.append(0xFF)
                    i += 1
            else:
                out.append(b)
                i += 1
        return bytes(out)

    def _send_raw(self, data):
        """Send raw bytes to socket."""
        self.sock.sendall(data)

    def _process_incoming(self, raw):
        """Filter telnet IAC, detect ANSI CPR queries, decode to text."""
        # Prepend any leftover bytes from incomplete IAC sequences
        if self.buffer:
            raw = self.buffer + raw
            self.buffer = b''
        clean = self._filter_telnet(raw)

        # Auto-respond to cursor position query ESC[6n
        if CPR_QUERY in clean:
            self._send_raw(CPR_RESPONSE)
            clean = clean.replace(CPR_QUERY, b'')

        text = clean.decode('latin-1')
        self.text_buffer += text
        self._all_text += text
        return text

    def recv_until_quiet(self, quiet_time=0.5, max_wait=10):
        """Read until BBS stops sending for quiet_time seconds."""
        collected = ''
        deadline = time.monotonic() + max_wait
        while time.monotonic() < deadline:
            raw = self._recv_raw(timeout=quiet_time)
            if not raw:
                break
            text = self._process_incoming(raw)
            collected += text
        return collected

    def expect(self, pattern, timeout=None, strip=True):
        """Wait for a substring pattern in received text.

        Args:
            pattern: substring to search for (case-insensitive in stripped text)
            timeout: max seconds to wait
            strip: if True, search in ANSI-stripped text

        Returns:
            All text received up to and including the match.

        Raises:
            TimeoutError: if pattern not found within timeout.
        """
        t = timeout if timeout is not None else self.timeout
        deadline = time.monotonic() + t

        while time.monotonic() < deadline:
            check = strip_to_plain(self.text_buffer) if strip else self.text_buffer
            if pattern.lower() in check.lower():
                result = self.text_buffer
                self.text_buffer = ''
                return result

            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            raw = self._recv_raw(timeout=min(remaining, 0.5))
            if raw:
                self._process_incoming(raw)

        plain = strip_to_plain(self.text_buffer)
        raise TimeoutError(
            f"Timed out waiting for {pattern!r} after {t}s.\n"
            f"Buffer ({len(self.text_buffer)} chars, plain={len(plain)}):\n"
            f"{plain[-500:]!r}"
        )

    def send_line(self, text):
        """Send text followed by CR (BBS expects CR, not LF)."""
        self._send_raw(text.encode('latin-1') + b'\r')

    def send_key(self, char):
        """Send a single character (for onek() menus)."""
        self._send_raw(char.encode('latin-1'))

    @property
    def all_text(self):
        """All text received so far (raw, not stripped)."""
        return self._all_text

    @property
    def all_text_plain(self):
        """All text received so far, ANSI-stripped."""
        return strip_to_plain(self._all_text)
