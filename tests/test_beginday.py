"""Test that beginday() doesn't break menu resolution in forked child."""

import json
import pathlib


class TestBeginday:
    """Tests for the beginday() path triggered by stale date in status.json."""

    def test_login_with_stale_date_reaches_menu(self, bbs_instance):
        """When status.json has yesterday's date, beginday runs during logon.
        The child process must still find its menus after beginday completes."""

        # Backdate status.json so beginday() triggers on first login
        status_path = pathlib.Path(bbs_instance.workdir) / "data" / "status.json"
        with open(status_path) as f:
            status = json.load(f)
        # Set date1 to yesterday so strcmp(date(), status.date1) != 0
        status["date1"] = "01/01/25"
        with open(status_path, "w") as f:
            json.dump(status, f)

        client = bbs_instance.create_client()
        try:
            client.expect("Your Handle or User Number", timeout=10)
            client.send_line("SYSOP")
            client.expect("User Password", timeout=5)
            client.send_line("SYSOP")

            # After login, beginday will run (stale date).
            # Collect all output — we should NOT see "Main Menu is missing"
            text = client.recv_until_quiet(quiet_time=3, max_wait=15)
            plain = client.all_text_plain

            assert "Main Menu is missing" not in plain, (
                f"Child process lost menus after beginday():\n{plain[-1000:]}"
            )
            # We should see evidence of successful login — the BBS stayed up
            assert "Illegal Logon" not in plain, (
                f"Login was rejected:\n{plain[-500:]}"
            )
        finally:
            client.close()
