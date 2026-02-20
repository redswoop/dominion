"""ANSI escape sequence stripping utilities."""

import re

# Matches CSI sequences: ESC [ <params> <intermediate> <final>
_CSI_RE = re.compile(r'\x1b\[[0-9;]*[A-Za-z]')

# Matches OSC sequences: ESC ] ... BEL/ST
_OSC_RE = re.compile(r'\x1b\].*?(?:\x07|\x1b\\)')

# Matches any ESC followed by one character (e.g., ESC[, ESC(, etc.)
_ESC_RE = re.compile(r'\x1b[^[\]]')


def strip_ansi(text):
    """Remove ANSI CSI/OSC/ESC sequences, preserving printable text."""
    text = _CSI_RE.sub('', text)
    text = _OSC_RE.sub('', text)
    text = _ESC_RE.sub('', text)
    return text


def strip_to_plain(text):
    """Remove ANSI sequences and all non-printable control characters."""
    text = strip_ansi(text)
    # Remove control chars except newline, carriage return, tab
    text = re.sub(r'[\x00-\x08\x0b\x0c\x0e-\x1f\x7f]', '', text)
    return text
