"""Test menu editor (menued) CRUD operations via menudb API."""

import time
import pytest


def _login_as_sysop(client):
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


def _enter_menu_editor(client):
    """From main menu, navigate to the sysop menu editor.

    Global menu has \\ = sysop menu (=/  submenu).
    Sysop menu has # = menu editor (S# command type).
    checkpw() prompts for sysop password before entering editor.
    """
    # \ = sysop menu (from global.mnu)
    client.send_key("\\")
    client.recv_until_quiet(quiet_time=2, max_wait=5)
    # # = menu editor
    client.send_key("#")
    # checkpw() will prompt for sysop password
    client.recv_until_quiet(quiet_time=1, max_wait=3)
    plain = client.all_text_plain
    if "Password" in plain or "password" in plain:
        client.send_line("SYSOP")
        client.recv_until_quiet(quiet_time=2, max_wait=5)


class TestMenuEditor:
    """Test menu editor CRUD via the BBS UI."""

    @pytest.fixture(autouse=True)
    def _increase_timeout(self):
        """Menu editor tests need more time."""
        pass

    def test_menu_editor_list(self, bbs_instance):
        """Enter menu editor and see menu listing without crash."""
        client = bbs_instance.create_client()
        try:
            _login_as_sysop(client)
            try:
                _enter_menu_editor(client)
            except (ConnectionResetError, BrokenPipeError, OSError):
                import time
                time.sleep(0.5)
                assert False, (
                    f"BBS crashed!\n"
                    f"stdout:\n{bbs_instance.stdout_text[-2000:]}\n"
                    f"stderr:\n{bbs_instance.stderr_text[-2000:]}"
                )
            plain = client.all_text_plain
            assert bbs_instance.running, (
                f"BBS crashed! stderr:\n{bbs_instance.stderr_text[-2000:]}"
            )
            assert "main.mnu" in plain.lower() or "Menu Files" in plain, (
                f"Expected menu listing, got:\n{plain[-2000:]}"
            )
        finally:
            client.close()

    def test_menu_editor_modify(self, bbs_instance):
        """Select M to modify a menu, type a name, enter the editor."""
        client = bbs_instance.create_client()
        try:
            _login_as_sysop(client)
            _enter_menu_editor(client)
            plain = client.all_text_plain
            assert "Menu Files" in plain or "main.mnu" in plain.lower()

            # M = modify
            client.send_key("M")
            client.recv_until_quiet(quiet_time=1, max_wait=3)

            # Check BBS is still alive before sending name
            assert bbs_instance.running, (
                f"BBS crashed after M! stderr:\n{bbs_instance.stderr_text[-2000:]}"
            )

            # Type menu name
            client.send_line("main")
            text = client.recv_until_quiet(quiet_time=2, max_wait=5)
            plain = client.all_text_plain

            assert bbs_instance.running, (
                f"BBS crashed after entering menu name! stderr:\n{bbs_instance.stderr_text[-2000:]}"
            )

            # Should see the menu editor (top view or info view)
            assert "Menu Editor" in plain or "Current Menu" in plain or "Menu File Name" in plain, (
                f"Expected menu editor screen, got:\n{plain[-2000:]}"
            )

            # Quit the editor
            client.send_key("Q")
            client.recv_until_quiet(quiet_time=1, max_wait=3)
        finally:
            client.close()

    def test_menu_editor_create_and_delete(self, bbs_instance):
        """Create a new menu, verify it appears, then delete it."""
        client = bbs_instance.create_client()
        try:
            _login_as_sysop(client)
            _enter_menu_editor(client)

            # I = insert/create new menu
            client.send_key("I")
            client.recv_until_quiet(quiet_time=1, max_wait=3)
            client.send_line("tstmnu")
            text = client.recv_until_quiet(quiet_time=2, max_wait=5)
            plain = client.all_text_plain

            assert bbs_instance.running, (
                f"BBS crashed during create! stderr:\n{bbs_instance.stderr_text[-2000:]}"
            )

            # Should enter the editor for the new menu
            assert "Menu Editor" in plain or "Current Menu" in plain or "Menu File Name" in plain, (
                f"Expected menu editor for new menu, got:\n{plain[-2000:]}"
            )

            # Quit editor back to menu list
            client.send_key("Q")
            client.recv_until_quiet(quiet_time=1, max_wait=3)

            # Refresh listing (Enter key)
            client.send_key("\r")
            text = client.recv_until_quiet(quiet_time=2, max_wait=5)
            plain = client.all_text_plain
            assert "tstmnu.mnu" in plain.lower(), (
                f"Expected tstmnu.mnu in listing after create, got:\n{plain[-2000:]}"
            )

            # D = delete
            client.send_key("D")
            client.recv_until_quiet(quiet_time=1, max_wait=3)
            client.send_line("tstmnu")
            client.recv_until_quiet(quiet_time=1, max_wait=3)

            # Refresh listing
            client.send_key("\r")
            text = client.recv_until_quiet(quiet_time=2, max_wait=5)
            plain = client.all_text_plain
            assert "tstmnu.mnu" not in plain.lower(), (
                f"tstmnu.mnu should be gone after delete, got:\n{plain[-2000:]}"
            )

            # Q to quit menu editor
            client.send_key("Q")
        finally:
            client.close()
