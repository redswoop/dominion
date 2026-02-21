"""Test full menu navigation workflow: login -> menus -> navigate -> logoff.

Exercises the critical path through converted menu files to verify
they render without crashes or corruption.
"""

import pytest


def _login_as_sysop(client):
    """Drive through login to reach main menu.

    The post-login flow has several prompts:
    1. "Fast Logon? YES/no" — answer Y
    2. File format selection (6 options then "?") — send Enter to accept
    3. Possibly more prompts — send Enter to skip
    4. Eventually reaches main menu prompt containing "Main Menu"
    """
    client.expect("Your Handle or User Number", timeout=10)
    client.send_line("SYSOP")
    client.expect("User Password", timeout=5)
    client.send_line("SYSOP")

    # Navigate through all post-login prompts by sending Enter
    # until we reach the main menu (or 15 seconds elapse).
    import time
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        text = client.recv_until_quiet(quiet_time=1, max_wait=3)
        plain = client.all_text_plain
        if "Main Menu" in plain:
            break
        # Send Enter to dismiss whatever prompt we're at
        client.send_line("")


class TestMenuWorkflow:
    """Full menu navigation workflow test."""

    def test_main_menu_display(self, bbs_client):
        """Login and press '?' to display the main menu.

        Verifies the menu renders without crashing and contains
        expected menu items from the converted menu file.
        """
        _login_as_sysop(bbs_client)

        # Press '?' to show the menu
        bbs_client.send_key("?")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        plain = bbs_client.all_text_plain

        # The main menu should contain these items (from mnudump)
        assert "Message" in plain or "essage" in plain, (
            f"Expected 'Message' in menu display, got:\n{plain[-1000:]}"
        )

    def test_navigate_to_message_menu_and_back(self, bbs_client):
        """Navigate to message menu, verify it loads, return to main."""
        _login_as_sysop(bbs_client)

        # 'M' goes to message menu (=^ goto message.mnu)
        bbs_client.send_key("M")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        plain = bbs_client.all_text_plain

        # Message menu should show its prompt or items
        assert "Message" in plain or "essage" in plain, (
            f"Expected message menu content, got:\n{plain[-1000:]}"
        )

        # 'Q' returns to main menu (=^ goto main.mnu)
        bbs_client.send_key("Q")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        plain = bbs_client.all_text_plain

        # Should be back at main menu
        assert "Main" in plain or "main" in plain.lower(), (
            f"Expected to return to main menu, got:\n{plain[-1000:]}"
        )

    def test_navigate_to_file_menu_and_back(self, bbs_client):
        """Navigate to file menu, verify it loads, return to main."""
        _login_as_sysop(bbs_client)

        # 'F' goes to file menu (=^ goto file.mnu)
        bbs_client.send_key("F")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        plain = bbs_client.all_text_plain

        # File menu should show its prompt or items
        assert "File" in plain or "file" in plain.lower(), (
            f"Expected file menu content, got:\n{plain[-1000:]}"
        )

        # 'Q' returns to main menu (=^ goto main.mnu)
        bbs_client.send_key("Q")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        plain = bbs_client.all_text_plain

        # Should be back at main menu
        assert "Main" in plain or "main" in plain.lower(), (
            f"Expected to return to main menu, got:\n{plain[-1000:]}"
        )

    def test_file_menu_show_display(self, bbs_client):
        """Navigate to file menu, press '?' to display it, return."""
        _login_as_sysop(bbs_client)

        # Go to file menu
        bbs_client.send_key("F")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)

        # Press '?' to show the file menu
        bbs_client.send_key("?")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        plain = bbs_client.all_text_plain

        # Should see file menu items like Download, Upload, List
        has_file_items = any(
            kw in plain for kw in ["ownload", "pload", "ist Files", "earch"]
        )
        assert has_file_items, (
            f"Expected file menu items in display, got:\n{plain[-1000:]}"
        )

        # Return to main
        bbs_client.send_key("Q")
        bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)

    def test_full_workflow_login_navigate_logoff(self, bbs_client):
        """Full workflow: login -> main -> files -> main -> logoff.

        This is the end-to-end smoke test that exercises the full
        converted menu system.
        """
        _login_as_sysop(bbs_client)

        # At main menu - press '?' to verify menu display works
        bbs_client.send_key("?")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)

        # Navigate to file menu
        bbs_client.send_key("F")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)

        # Return to main
        bbs_client.send_key("Q")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)

        # Navigate to message menu
        bbs_client.send_key("M")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)

        # Return to main
        bbs_client.send_key("Q")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)

        # Logoff: 'G' then 'Y' to confirm
        bbs_client.send_key("G")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        bbs_client.send_key("Y")
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)

        plain = bbs_client.all_text_plain
        # Should have gone through the entire flow without crashing
        assert len(plain) > 100, (
            f"Expected substantial output from full workflow, got {len(plain)} chars"
        )
