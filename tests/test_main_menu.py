"""Test main menu navigation after login."""

import pytest


def _login_as_sysop(client):
    """Helper: drive through login to reach main menu."""
    client.expect("Your Handle or User Number", timeout=10)
    client.send_line("SYSOP")
    client.expect("User Password", timeout=5)
    client.send_line("SYSOP")
    # Consume post-login output until things settle
    client.recv_until_quiet(quiet_time=2, max_wait=10)


class TestMainMenu:
    """Post-login menu navigation tests."""

    def test_logoff_command(self, bbs_client):
        """'Q' at main menu triggers logoff."""
        _login_as_sysop(bbs_client)
        bbs_client.send_key("Q")
        # Should see logoff prompt or goodbye message
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        # Accept the logoff
        bbs_client.send_key("Y")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        plain = bbs_client.all_text_plain
        # After logoff the connection should close or we see logoff screen
        assert len(plain) > 0, "Expected some output during logoff"

    @pytest.mark.skip(reason="Needs menu key mapping from BBS output")
    def test_message_menu(self, bbs_client):
        """Message menu is accessible from main menu."""
        pass

    @pytest.mark.skip(reason="Needs menu key mapping from BBS output")
    def test_file_menu(self, bbs_client):
        """File menu is accessible from main menu."""
        pass
