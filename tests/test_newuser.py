"""Test new user registration flow."""

import pytest


class TestNewUser:
    """New user registration via 'NEW' at login prompt."""

    def test_new_triggers_registration(self, bbs_client):
        """Typing 'NEW' at login prompt enters registration flow."""
        bbs_client.expect("Your Handle or User Number", timeout=10)
        bbs_client.send_line("NEW")

        # Should enter new user flow — expect some kind of prompt
        # (not "Illegal Logon")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        plain = bbs_client.all_text_plain
        assert "Illegal Logon" not in plain, "NEW should not trigger illegal logon"

    @pytest.mark.skip(reason="Needs exact prompt sequence mapping from BBS output")
    def test_full_registration(self, bbs_client):
        """Complete a full new user registration."""
        pass
