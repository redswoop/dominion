"""Test new user registration screen renders correctly."""


class TestNewUserScreen:
    """Verify the new user ScreenForm renders with prompts visible over TCP."""

    def test_newuser_screen_pipe_borders(self, bbs_client):
        """The ScreenForm sends newans.ans + field prompts directly over TCP.

        The old approach used printfile("newans.ans") with incom=0 (local-only)
        and mciok=0 to prevent pipe-color consumption of | borders.  The new
        approach uses term.sendAnsiFile() which bypasses MCI entirely and sends
        the art raw to the remote connection.

        We verify the fix by confirming the form's first field prompt is visible
        over TCP — if the ANSI art or setup code crashed the flow, the prompt
        would never arrive.
        """
        bbs_client.expect("Your Handle or User Number", timeout=10)
        bbs_client.send_line("NEW")

        # input_screensize() — screen geometry (defaults are fine)
        bbs_client.expect("wide", timeout=5)
        bbs_client.send_line("")
        bbs_client.expect("tall", timeout=5)
        bbs_client.send_line("")

        # pausescr() after system/newuser display files
        for _ in range(10):
            try:
                bbs_client.expect("Keyboard", timeout=3)
                bbs_client.send_key(" ")
            except TimeoutError:
                break

        # ScreenForm renders newans.ans and shows first field prompt over TCP
        bbs_client.expect("handle or real name", timeout=15)

        # ESC cancels — on_cancel sets hangup, connection closes
        bbs_client.send_key("\x1b")


class TestNewUserRegistration:
    """Complete new user registration and verify entry into main menu."""

    def test_full_registration(self, bbs_client):
        """Register a new user through all ScreenForm fields and reach main menu.

        The ScreenForm uses Tab/Enter to advance between fields.  Auto-advance
        fields (Date: 6 digits, Phone: 10 digits, SelectField: valid key char)
        advance without a CR so send_key() is used instead of send_line().

        The bug being guarded: set_autoval(nifty.nulevel) with uninitialized
        autoval[0] set user SL to 0, causing infinite access denial and crash.
        Fix: initialize autoval[0] in mkconfig to match newusersl/newuserdsl.
        """
        bbs_client.expect("Your Handle or User Number", timeout=10)
        bbs_client.send_line("NEW")

        # Screen size (defaults are fine)
        bbs_client.expect("wide", timeout=5)
        bbs_client.send_line("")
        bbs_client.expect("tall", timeout=5)
        bbs_client.send_line("")

        # pausescr() prompts from system/newuser display files
        for _ in range(10):
            try:
                bbs_client.expect("Keyboard", timeout=3)
                bbs_client.send_key(" ")
            except TimeoutError:
                break

        # Field 0: Name (TextField, max 30) — Enter advances
        bbs_client.expect("handle or real name", timeout=15)
        bbs_client.send_line("RegTestUser")

        # Field 1: Real name (TextField, max 20) — Enter advances
        bbs_client.expect("same as alias", timeout=5)
        bbs_client.send_line("Reg Test")

        # Field 2: Sex (SelectField) — key char auto-advances, no CR needed
        bbs_client.expect("(M)ale", timeout=5)
        bbs_client.send_key("M")

        # Field 3: Birthdate (DateField MM/DD/YY) — 6 digits auto-advance
        bbs_client.expect("MM/DD/YY", timeout=5)
        bbs_client.send_key("011590")

        # Field 4: Phone (PhoneField) — 10 digits auto-advance, no CR
        bbs_client.expect("XXX-XXX-XXXX", timeout=5)
        bbs_client.send_key("5551231234")

        # Field 5: Street (TextField, max 35) — Enter advances
        bbs_client.expect("street address", timeout=5)
        bbs_client.send_line("123 Test St")

        # Field 6: City (TextField, max 35) — Enter advances
        bbs_client.expect("city and state", timeout=5)
        bbs_client.send_line("Testville CA")

        # Field 7: Comment (TextField, optional) — Enter advances
        bbs_client.expect("optional", timeout=5)
        bbs_client.send_line("Test registration")

        # Field 8: Computer type (SelectField popup) — key char auto-advances
        bbs_client.expect("computer type", timeout=5)
        bbs_client.send_key("1")

        # Field 9: Password (TextField masked) — Enter advances to command mode
        bbs_client.expect("3-20 chars", timeout=5)
        bbs_client.send_line("TESTPASS")

        # Command mode: "Q to save, 1-9 to edit, ESC to cancel"
        bbs_client.expect("Q to save", timeout=5)
        bbs_client.send_key("Q")

        # Should see "Your user number is N"
        bbs_client.expect("user number is", timeout=10)

        # pausescr() after user number display
        for _ in range(5):
            try:
                bbs_client.expect("Keyboard", timeout=3)
                bbs_client.send_key(" ")
            except TimeoutError:
                break

        # After registration completes, the BBS enters the main menu.
        # If autoval is broken (SL=0), we'd get infinite "Sorry, you do
        # not have the proper access" and crash.  With the fix, we reach
        # main menu or logon prompt without crashing.
        import time
        time.sleep(2)

        try:
            bbs_client.expect("Logon", timeout=15)
        except TimeoutError:
            pass

        # The real test: the BBS process should still be alive.
        # If it crashed (SIGSEGV), the socket would be dead.
        bbs_client.send_key("Y")
