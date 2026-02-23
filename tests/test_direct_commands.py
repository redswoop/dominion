"""Test commands dispatched through the direct registry (cmd_direct_t)."""

import time
import pytest


def _login_to_main_menu(client):
    """Drive through login to reach main menu."""
    client.expect("Your Handle or User Number", timeout=10)
    client.send_line("SYSOP")
    client.expect("User Password", timeout=5)
    client.send_line("SYSOP")

    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        text = client.recv_until_quiet(quiet_time=1, max_wait=3)
        plain = client.all_text_plain
        if "Main Menu" in plain:
            break
        client.send_line("")


class TestDirectCommands:
    """Commands extracted from othercmd() into the direct registry."""

    def test_sysinfo_shows_version(self, bbs_client):
        """'I' at main menu shows version string (OI -> cmd_sysinfo)."""
        _login_to_main_menu(bbs_client)
        bbs_client.send_key("I")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        plain = bbs_client.all_text_plain
        assert "Dominion Bulletin Board System" in plain
        assert "v3.1" in plain
