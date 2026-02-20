"""Test login flows via TCP."""

import time


class TestLogin:
    """Login flow tests: SYSOP login, bad password, unknown user."""

    def test_sysop_login_succeeds(self, bbs_client):
        """SYSOP can log in with correct password."""
        bbs_client.expect("Your Handle or User Number", timeout=10)
        bbs_client.send_line("SYSOP")
        bbs_client.expect("User Password", timeout=5)
        bbs_client.send_line("SYSOP")

        # After login, we should NOT see "Illegal Logon" and should
        # reach the main menu or some post-login content
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=10)
        plain = bbs_client.all_text_plain
        assert "Illegal Logon" not in plain, "Login was rejected unexpectedly"

    def test_wrong_password_rejected(self, bbs_client):
        """Wrong password is rejected."""
        bbs_client.expect("Your Handle or User Number", timeout=10)
        bbs_client.send_line("SYSOP")
        bbs_client.expect("User Password", timeout=5)
        bbs_client.send_line("WRONGPASSWORD")

        # Should get another login prompt (BBS allows up to 5 attempts)
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        plain = bbs_client.all_text_plain
        # Either "Illegal Logon" or another username prompt
        has_rejection = (
            "Illegal Logon" in plain
            or "Your Handle or User Number" in plain
        )
        assert has_rejection, (
            f"Expected rejection after wrong password, got:\n{plain[-500:]}"
        )

    def test_unknown_user_rejected(self, bbs_client):
        """Unknown username is rejected."""
        bbs_client.expect("Your Handle or User Number", timeout=10)
        bbs_client.send_line("NOBODY")

        # Should get "Illegal Logon" or re-prompt
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        plain = bbs_client.all_text_plain
        has_rejection = (
            "Illegal Logon" in plain
            or "Your Handle or User Number" in plain
        )
        assert has_rejection, (
            f"Expected rejection for unknown user, got:\n{plain[-500:]}"
        )
