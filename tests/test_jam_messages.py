"""Test JAM message base operations: post, read, jamdump verification."""

import os
import pathlib
import subprocess

DOMINION_DIR = pathlib.Path(__file__).resolve().parent.parent


def _login_as_sysop(client):
    """Drive through login to reach main menu."""
    client.expect("Your Handle or User Number", timeout=10)
    client.send_line("SYSOP")
    client.expect("User Password", timeout=5)
    client.send_line("SYSOP")
    # Answer "Fast Logon?" prompt (default yes)
    client.expect("Fast Logon", timeout=10)
    client.send_line("")
    # Wait for main menu to appear
    client.recv_until_quiet(quiet_time=2, max_wait=10)


class TestJamMessageBase:
    """JAM message base creation and file integrity tests."""

    def test_jam_files_created_on_login(self, bbs_instance):
        """Login triggers JAM message base creation for email sub."""
        client = bbs_instance.create_client()
        try:
            _login_as_sysop(client)

            # After login, the BBS scans for new mail via findwaiting()
            # which calls JAMOpen() -> JAMmbCreate() for new message bases.
            # Check that the JAM files were created in the msgs/ directory.
            msgs_dir = bbs_instance.workdir / "msgs"
            assert msgs_dir.exists(), "msgs/ directory should exist"

            # The email sub has filename "email", so we expect email.j* files
            jhr = msgs_dir / "email.jhr"
            jdt = msgs_dir / "email.jdt"
            jdx = msgs_dir / "email.jdx"
            jlr = msgs_dir / "email.jlr"

            assert jhr.exists(), f"email.jhr should exist in {msgs_dir}"
            assert jdt.exists(), f"email.jdt should exist in {msgs_dir}"
            assert jdx.exists(), f"email.jdx should exist in {msgs_dir}"
            assert jlr.exists(), f"email.jlr should exist in {msgs_dir}"

            # The .jhr should start with "JAM\0" signature
            with open(str(jhr), "rb") as f:
                sig = f.read(4)
            assert sig[:3] == b"JAM", f"Expected JAM signature, got {sig!r}"

            # The .jdx should be empty (no messages yet)
            assert jdx.stat().st_size == 0, "Index file should be empty (no messages)"

        finally:
            client.close()

    def test_jamdump_on_empty_base(self, bbs_instance):
        """jamdump can read a freshly-created (empty) message base."""
        client = bbs_instance.create_client()
        try:
            _login_as_sysop(client)
        finally:
            client.close()

        jamdump = DOMINION_DIR / "build" / "jamdump"
        assert jamdump.exists(), f"jamdump not found at {jamdump}"

        email_base = bbs_instance.workdir / "msgs" / "email"
        result = subprocess.run(
            [str(jamdump), str(email_base)],
            capture_output=True,
            text=True,
            timeout=5,
        )
        assert result.returncode == 0, (
            f"jamdump failed (rc={result.returncode}):\n"
            f"stdout: {result.stdout}\n"
            f"stderr: {result.stderr}"
        )

        # Should show header info with JAM signature and 0 messages
        assert "JAM" in result.stdout
        assert "Active Msgs" in result.stdout
        assert "Index entries: 0" in result.stdout

    def test_post_message_on_general_sub(self, bbs_instance):
        """Post a message on the General sub and verify with jamdump."""
        client = bbs_instance.create_client()
        try:
            _login_as_sysop(client)

            # Wait for main menu prompt, then go to message menu
            client.expect("Main Menu", timeout=5)
            client.send_key("M")

            # Message menu shows a status screen first (pause prompt)
            client.expect("Keyboard", timeout=5)
            client.send_key(" ")

            # Then forced newscan prompt — decline it
            client.expect("NewScan", timeout=5)
            client.send_line("")  # default NO

            # Wait for message menu, then switch to General Discussion (sub 1)
            client.expect("Message Menu", timeout=10)
            client.send_key("]")  # next area
            client.expect("General", timeout=5)

            # Now post
            client.send_key("P")

            # Decline upload prepared file
            client.expect("Upload Prepared File", timeout=5)
            client.send_line("")  # default NO

            # Enter subject
            client.expect("Subject", timeout=5)
            client.send_line("Test Message Subject")

            # Enter receiver (blank = "All")
            client.expect("Receiver", timeout=5)
            client.send_line("")

            # Message editor instructions appear, type body and save
            client.expect("/S", timeout=5)
            client.send_line("This is a test message body.")
            client.send_line("/S")

            # Wait for save confirmation
            text = client.recv_until_quiet(quiet_time=2, max_wait=10)
            plain = client.all_text_plain

            # Should see "Posted on" confirmation
            assert "Posted on" in plain, (
                f"Expected 'Posted on' confirmation, got:\n{plain[-1000:]}"
            )

        finally:
            client.close()

        # Verify with jamdump
        jamdump = DOMINION_DIR / "build" / "jamdump"
        general_base = bbs_instance.workdir / "msgs" / "general"
        result = subprocess.run(
            [str(jamdump), str(general_base)],
            capture_output=True,
            text=True,
            timeout=5,
        )
        assert result.returncode == 0, (
            f"jamdump failed:\nstdout: {result.stdout}\nstderr: {result.stderr}"
        )

        # Verify message content in jamdump output
        assert "Test Message Subject" in result.stdout, (
            f"Subject not found in jamdump output:\n{result.stdout}"
        )
        assert "test message body" in result.stdout.lower(), (
            f"Message body not found in jamdump output:\n{result.stdout}"
        )
        assert "Index entries: 1" in result.stdout, (
            f"Expected 1 message, jamdump output:\n{result.stdout}"
        )
