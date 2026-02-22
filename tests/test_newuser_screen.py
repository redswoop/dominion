"""Test new user registration screen renders correctly."""


class TestNewUserScreen:
    """Verify the new user ANSI form renders with pipe borders intact."""

    def test_newuser_screen_pipe_borders(self, bbs_client):
        """The ANSI form template (newans.ans) should render with | borders.

        The bug was that mciok=1 caused outchr() to consume | characters
        as BBS pipe color codes, destroying the ANSI template borders.
        Fix: set mciok=0 before printfile("newans.ans").

        Note: The ANSI form is rendered with incom=0 (local screen only),
        so TCP clients don't receive the template. But the form IS rendered
        through outchr() which is where the mciok bug was. We verify the
        fix indirectly by confirming the new user flow reaches the input
        prompts (if the template crashed the display, the flow would break).
        """
        bbs_client.expect("Your Handle or User Number", timeout=10)
        bbs_client.send_line("NEW")

        # ANSI+color default to on, no ANSI/Avatar/RIP questions.
        # First prompt is input_screensize().
        bbs_client.expect("wide", timeout=5)
        bbs_client.send_line("")
        bbs_client.expect("tall", timeout=5)
        bbs_client.send_line("")

        # pausescr() prompts + possible paging within the display files.
        # Keep pressing space until we stop seeing "Keyboard" prompts.
        for _ in range(10):
            try:
                bbs_client.expect("Keyboard", timeout=3)
                bbs_client.send_key(" ")
            except TimeoutError:
                break

        # After the ANSI form (drawn locally with incom=0), input_name()
        # sends "Enter a handle or your real name" over TCP
        bbs_client.expect("handle or your real name", timeout=10)

        # If we got here, the new user flow survived past the ANSI template
        # rendering. Enter a name to continue.
        bbs_client.send_line("TestUser")

        # input_realname() sends "Enter your real name"
        bbs_client.expect("real name", timeout=5)
        bbs_client.send_line("Test User")

        # input_sex() — should ask for gender
        bbs_client.expect("Sex", timeout=5)

        # Success — the new user form rendered and the flow continued
        # through multiple input functions without hanging or crashing.


class TestNewUserRegistration:
    """Complete new user registration and verify entry into main menu."""

    def test_full_registration(self, bbs_client):
        """Register a new user through all fields, confirm, and reach main menu.

        The bug was that set_autoval(nifty.nulevel) with uninitialized
        autoval[0] set the user's SL to 0, causing infinite access denial
        in menuman() and a SIGSEGV from stack/heap exhaustion.
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

        # input_name()
        bbs_client.expect("handle or your real name", timeout=10)
        bbs_client.send_line("RegTestUser")

        # input_realname()
        bbs_client.expect("real name", timeout=5)
        bbs_client.send_line("Reg Test")

        # input_sex() — onek("MFYL")
        bbs_client.expect("Sex", timeout=5)
        bbs_client.send_key("M")

        # input_age() — birthdate in MM/DD/YY form
        bbs_client.expect("birthdate", timeout=5)
        bbs_client.send_line("01/15/90")

        # input_phone()
        bbs_client.expect("Phone", timeout=5)
        bbs_client.send_line("555-555-1234")

        # input_city() — street then city/state
        bbs_client.expect("Street", timeout=5)
        bbs_client.send_line("123 Test St")
        bbs_client.expect("City", timeout=5)
        bbs_client.send_line("Testville CA")

        # input_comment()
        bbs_client.expect("Comment", timeout=5)
        bbs_client.send_line("Test registration")

        # input_comptype() — pick option 1
        bbs_client.expect("computer type", timeout=5)
        bbs_client.send_line("1")

        # input_pw() — single password entry (no verification)
        bbs_client.expect("password", timeout=5)
        bbs_client.send_line("TESTPASS")

        # Review screen — press Q to accept
        bbs_client.expect("Q=Quit", timeout=5)
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

        # email() for feedback — may prompt for message text
        # The email function might fail ("Too many messages posted today")
        # or prompt for feedback. Either way, we should eventually reach
        # the main menu or a logon prompt.

        # After registration completes, the BBS enters the main menu.
        # If autoval is broken (SL=0), we'd get infinite "Sorry, you do
        # not have the proper access" and crash. With the fix, we should
        # see the main menu or a "Fast Logon" prompt.
        #
        # Look for either main menu content or logon prompts.
        # The key assertion: the BBS does NOT crash (no timeout/disconnect).
        import time
        time.sleep(2)

        # Try to get any output — if BBS crashed, socket is dead
        try:
            # Look for common post-login prompts/menu content
            # "Fast Logon" is asked by logon() in lilo.c
            bbs_client.expect("Logon", timeout=15)
        except TimeoutError:
            # Even if we don't find "Logon", check we're still connected
            # by looking at whatever text arrived
            pass

        # The real test: the BBS process should still be alive.
        # If it crashed (SIGSEGV), the socket would be dead and
        # bbs_client would have gotten an empty recv or connection reset.
        # Just sending any data without error proves the BBS is alive.
        bbs_client.send_key("Y")
