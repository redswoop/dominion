"""Test menu display width after ccount fix."""
import re
import time


def _login(client):
    """Login as sysop and reach main menu."""
    client.expect("Handle or User Number", timeout=10)
    client.send_line("SYSOP")
    client.expect("Password", timeout=5)
    client.send_line("SYSOP")
    client.expect("Fast Logon", timeout=10)
    client.send_line("")
    client.expect("Main Menu", timeout=15)
    client.recv_until_quiet(quiet_time=2, max_wait=5)


def _get_menu_lines(plain):
    """Extract menu item lines (lines containing <X> bracket patterns)."""
    lines = plain.split('\n')
    menu_lines = []
    for line in lines:
        stripped = line.rstrip()
        if re.search(r'<[^>]+>', stripped) and len(stripped) > 10:
            menu_lines.append(stripped)
    return menu_lines


class TestMenuWidth:

    def test_main_menu_fits_80_cols(self, bbs_instance):
        """Main menu 4-column rows should fit in 80 columns."""
        client = bbs_instance.create_client()
        try:
            _login(client)
            client.send_key("?")
            time.sleep(3)
            client.recv_until_quiet(quiet_time=2, max_wait=10)
            menu_lines = _get_menu_lines(client.all_text_plain)

            print("\n=== MAIN MENU LINES ===")
            for line in menu_lines:
                print("  len=%3d '%s'" % (len(line), line[:120]))

            for line in menu_lines:
                assert len(line) <= 80, (
                    "Main menu row exceeds 80 cols (%d): '%s'" % (len(line), line)
                )
        finally:
            client.close()

    def test_file_menu_items_separated(self, bbs_instance):
        """File menu items should have space between them, not run together."""
        client = bbs_instance.create_client()
        try:
            _login(client)

            # Navigate to file menu (single key, no enter)
            client.send_key("F")

            # File status screen with "Slap your Keyboard!" pause
            client.expect("Keyboard", timeout=10)
            client.send_key(" ")

            # Global NewScan prompt - decline
            client.expect("NewScan", timeout=5)
            client.send_line("")

            # Wait for file menu prompt
            client.expect("File Menu", timeout=10)
            client.recv_until_quiet(quiet_time=2, max_wait=5)

            # Press ? to display file menu items
            client.send_key("?")
            time.sleep(3)
            client.recv_until_quiet(quiet_time=2, max_wait=10)

            plain = client.all_text_plain
            print("\n=== FILE MENU ALL TEXT (last 3000) ===")
            print(plain[-3000:])

            menu_lines = _get_menu_lines(plain)

            print("\n=== FILE MENU ITEM LINES ===")
            for line in menu_lines:
                print("  len=%3d '%s'" % (len(line), line[:120]))

            # Filter to only actual menu item rows (multiple <X> patterns)
            multi_item_lines = [l for l in menu_lines
                                if len(re.findall(r'<[^>]+>', l)) >= 2]

            for line in multi_item_lines:
                assert len(line) <= 80, (
                    "File menu row exceeds 80 cols (%d): '%s'" % (len(line), line)
                )

            # Check no items run together: look for >word< pattern where
            # the word has no trailing space before the next <
            for line in multi_item_lines:
                # Match: closing > followed by non-space chars then opening <
                # But exclude patterns inside single items like </G>
                # The key pattern is: description text immediately touching
                # the next item's opening bracket
                items = re.findall(r'<[^>]+>[^<]*', line)
                for item in items:
                    # Each item should end with at least one space (unless last)
                    item_stripped = item.rstrip()
                    if item != items[-1]:  # not the last item on the line
                        assert item[-1] == ' ', (
                            "Item has no trailing space: '%s' in '%s'" %
                            (item_stripped, line)
                        )
        finally:
            client.close()
