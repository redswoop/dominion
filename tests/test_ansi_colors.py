"""test_ansi_colors.py — Verify CGA-accurate true color output over TCP.

The BBS sends 24-bit true color (ESC[38;2;R;G;Bm) for all foreground colors
to bypass terminal palette dependencies.  These tests verify the correct
escape sequences appear in the TCP stream.
"""

import re


# All 16 CGA RGB values — must match cgaRGB[] in terminal.cpp
# and cga_rgb[] in stream_processor.c
CGA_RGB = {
    0:  (0, 0, 0),        # black
    1:  (0, 0, 170),      # blue
    2:  (0, 170, 0),      # green
    3:  (0, 170, 170),    # cyan
    4:  (170, 0, 0),      # red
    5:  (170, 0, 170),    # magenta
    6:  (170, 85, 0),     # brown
    7:  (170, 170, 170),  # light grey
    8:  (85, 85, 85),     # dark grey
    9:  (85, 85, 255),    # light blue
    10: (85, 255, 85),    # light green
    11: (85, 255, 255),   # light cyan
    12: (255, 85, 85),    # light red
    13: (255, 85, 255),   # light magenta
    14: (255, 255, 85),   # yellow
    15: (255, 255, 255),  # white
}

CGA_RGB_SET = set(CGA_RGB.values())

# Regex: ESC[38;2;R;G;Bm (24-bit true color foreground)
TRUE_COLOR_RE = re.compile(r'\x1b\[38;2;(\d+);(\d+);(\d+)m')

# Regex: ESC[90m through ESC[97m (old palette-dependent bright codes)
OLD_BRIGHT_RE = re.compile(r'\x1b\[9[0-7]m')


def _login_to_main_menu(client):
    """Log in as sysop and wait for main menu to render."""
    client.expect("Your Handle", timeout=10)
    client.send_line("SYSOP")
    client.expect("User Password", timeout=5)
    client.send_line("SYSOP")
    client.recv_until_quiet(quiet_time=2, max_wait=10)


class TestAnsiColors:
    """Verify CGA true color escape sequences in TCP output."""

    def test_true_color_with_valid_cga_values(self, bbs_instance):
        """TCP stream contains true color sequences with CGA RGB values."""
        client = bbs_instance.create_client()
        try:
            _login_to_main_menu(client)
            raw = client.all_text

            matches = TRUE_COLOR_RE.findall(raw)
            assert len(matches) > 0, (
                "No true color sequences (ESC[38;2;R;G;Bm) found in TCP stream"
            )

            non_cga = set()
            for r, g, b in matches:
                rgb = (int(r), int(g), int(b))
                if rgb not in CGA_RGB_SET:
                    non_cga.add(rgb)

            assert len(non_cga) == 0, (
                f"Found non-CGA RGB values in true color sequences: {non_cga}"
            )
        finally:
            client.close()

    def test_no_palette_bright_codes(self, bbs_instance):
        """TCP stream uses true color, not palette-dependent ESC[9Xm."""
        client = bbs_instance.create_client()
        try:
            _login_to_main_menu(client)
            raw = client.all_text

            old_codes = OLD_BRIGHT_RE.findall(raw)
            assert len(old_codes) == 0, (
                f"Found {len(old_codes)} palette-dependent bright codes "
                f"(ESC[9Xm) that should be true color: {old_codes[:5]}"
            )
        finally:
            client.close()
