#!/usr/bin/env python3
"""bbsview.py — BBS screen viewer via virtual terminal emulator.

Connects to BBS over TCP, renders ANSI output through pyte (VT terminal
emulator), dumps the screen buffer as numbered plain text lines.

Usage:
  bbsview.py -p PORT                           # connect, dump login screen
  bbsview.py -p PORT -k "SYSOP\\r" -k "SYSOP\\r" -k "\\r"  # multi-step login
  bbsview.py -p PORT -w 3                      # wait 3s for output to settle
  bbsview.py -p PORT -r 25 -c 80              # set virtual screen size

Each -k sends keystrokes then waits for output to settle before the next -k.
"""

import argparse
import socket
import sys
import time

try:
    import pyte
except ImportError:
    print("Error: pyte not installed. Run: pip install pyte", file=sys.stderr)
    sys.exit(1)

# Telnet IAC constants
IAC  = 0xFF
WILL = 0xFB
WONT = 0xFC
DO   = 0xFD
DONT = 0xFE
SB   = 0xFA
SE   = 0xF0

# ANSI cursor position request/response
CPR_QUERY = b'\x1b[6n'


def filter_telnet(data, sock, leftover):
    """Strip telnet IAC sequences from raw bytes. Returns (clean_bytes, leftover).

    Responds WONT to any DO request. Handles incomplete sequences at buffer
    boundaries via leftover accumulator.
    """
    data = leftover + data
    out = bytearray()
    i = 0
    while i < len(data):
        b = data[i]
        if b == IAC:
            if i + 1 >= len(data):
                return bytes(out), bytes(data[i:])
            cmd = data[i + 1]
            if cmd in (WILL, WONT, DO, DONT):
                if i + 2 >= len(data):
                    return bytes(out), bytes(data[i:])
                opt = data[i + 2]
                if cmd == DO:
                    sock.sendall(bytes([IAC, WONT, opt]))
                i += 3
            elif cmd == SB:
                j = i + 2
                while j < len(data) - 1:
                    if data[j] == IAC and data[j + 1] == SE:
                        break
                    j += 1
                if j >= len(data) - 1:
                    return bytes(out), bytes(data[i:])
                i = j + 2
            else:
                out.append(0xFF)
                i += 1
        else:
            out.append(b)
            i += 1
    return bytes(out), b''


def respond_cpr(data, sock, screen):
    """Detect ESC[6n cursor position requests and auto-respond."""
    if CPR_QUERY not in data:
        return data
    row = screen.cursor.y + 1
    col = screen.cursor.x + 1
    sock.sendall(f'\x1b[{row};{col}R'.encode('latin-1'))
    return data.replace(CPR_QUERY, b'')


def parse_keystrokes(raw):
    """Parse keystroke string: \\r→CR, \\n→LF, \\e→ESC, \\t→TAB, rest literal."""
    out = bytearray()
    i = 0
    while i < len(raw):
        if raw[i] == '\\' and i + 1 < len(raw):
            c = raw[i + 1]
            if c == 'r':
                out.append(0x0D)
            elif c == 'n':
                out.append(0x0A)
            elif c == 'e':
                out.append(0x1B)
            elif c == 't':
                out.append(0x09)
            elif c == '\\':
                out.append(0x5C)
            else:
                out.append(ord(raw[i]))
                out.append(ord(c))
            i += 2
        else:
            out.extend(raw[i].encode('latin-1'))
            i += 1
    return bytes(out)


def dump_screen(screen):
    """Print screen buffer as numbered plain-text lines."""
    rows = screen.lines
    cols = screen.columns
    print(f'--- screen {cols}x{rows} cursor=({screen.cursor.x},{screen.cursor.y}) ---')
    for row in range(rows):
        line_chars = []
        for col in range(cols):
            ch = screen.buffer[row][col]
            line_chars.append(ch.data if ch.data else ' ')
        line = ''.join(line_chars).rstrip()
        print(f'{row:02d}: {line}')
    print('---')


def recv_until_quiet(sock, stream, screen, quiet_time, max_wait):
    """Read from socket until no data arrives for quiet_time seconds.

    Uses a short poll interval (0.1s) to avoid missing slow character-by-
    character echoing (like password asterisks).  The quiet timer resets on
    every received byte.
    """
    POLL = 0.1          # socket recv timeout per iteration
    leftover = b''
    last_data = time.monotonic()
    deadline = time.monotonic() + max_wait
    while time.monotonic() < deadline:
        remaining = deadline - time.monotonic()
        sock.settimeout(min(POLL, remaining))
        try:
            raw = sock.recv(4096)
            if not raw:
                break
        except socket.timeout:
            if time.monotonic() - last_data >= quiet_time:
                break
            continue
        except (ConnectionResetError, BrokenPipeError, OSError):
            break
        last_data = time.monotonic()
        clean, leftover = filter_telnet(raw, sock, leftover)
        clean = respond_cpr(clean, sock, screen)
        if clean:
            stream.feed(clean)


def main():
    parser = argparse.ArgumentParser(description='BBS screen viewer')
    parser.add_argument('-p', '--port', type=int, required=True, help='TCP port')
    parser.add_argument('-H', '--host', default='127.0.0.1', help='Host (default: 127.0.0.1)')
    parser.add_argument('-k', '--keys', action='append', default=None,
                        help='Keystrokes to send (\\r=CR, \\e=ESC). Repeatable — each -k waits for output before next.')
    parser.add_argument('-w', '--wait', type=float, default=1.0, help='Quiet time before dump (default: 1.0s)')
    parser.add_argument('-W', '--max-wait', type=float, default=15.0, help='Max total wait (default: 15s)')
    parser.add_argument('-r', '--rows', type=int, default=25, help='Screen rows (default: 25)')
    parser.add_argument('-c', '--cols', type=int, default=80, help='Screen columns (default: 80)')
    args = parser.parse_args()

    screen = pyte.Screen(args.cols, args.rows)
    stream = pyte.ByteStream(screen)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(10)
    try:
        sock.connect((args.host, args.port))
    except (ConnectionRefusedError, OSError) as e:
        print(f'Connection failed: {e}', file=sys.stderr)
        sys.exit(1)

    # Read initial output (login screen, ANSI check, etc.)
    recv_until_quiet(sock, stream, screen, args.wait, args.max_wait)

    # Send keystrokes step by step — each -k waits for output to settle
    if args.keys:
        for step in args.keys:
            keys = parse_keystrokes(step)
            try:
                sock.sendall(keys)
            except (BrokenPipeError, OSError):
                break
            recv_until_quiet(sock, stream, screen, args.wait, args.max_wait)

    dump_screen(screen)

    try:
        sock.shutdown(socket.SHUT_RDWR)
    except OSError:
        pass
    sock.close()


if __name__ == '__main__':
    main()
