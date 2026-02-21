"""Debug the title line rendering in the file menu."""
import time

def test_title_rendering(bbs_instance):
    client = bbs_instance.create_client()
    try:
        client.expect("Handle or User Number", timeout=10)
        client.send_line("SYSOP")
        client.expect("Password", timeout=5)
        client.send_line("SYSOP")
        client.expect("Fast Logon", timeout=10)
        client.send_line("")
        client.expect("Main Menu", timeout=15)
        client.recv_until_quiet(quiet_time=2, max_wait=5)

        # Go to file menu
        client.send_key("F")
        client.expect("Keyboard", timeout=10)
        client.send_key(" ")
        client.expect("NewScan", timeout=5)
        client.send_line("")
        client.expect("File Menu", timeout=10)
        client.recv_until_quiet(quiet_time=2, max_wait=5)

        # Clear and redisplay
        client.send_key("?")
        time.sleep(3)
        client.recv_until_quiet(quiet_time=2, max_wait=10)

        # Get the raw ANSI text around "SysOp Commands"
        all_text = client.all_text
        idx = all_text.find("SysOp Commands")
        if idx >= 0:
            # Show 200 chars before and after
            snippet = all_text[max(0,idx-200):idx+200]
            print("\n=== RAW ANSI around 'SysOp Commands' ===")
            for i, ch in enumerate(snippet):
                if ch == '\x1b':
                    print("ESC", end="")
                elif ord(ch) < 32 and ch not in '\r\n':
                    print("<%02X>" % ord(ch), end="")
                else:
                    print(ch, end="")
            print()

            # Also show hex
            print("\n=== HEX around 'SysOp Commands' ===")
            raw_bytes = snippet.encode('latin-1', errors='replace')
            for i in range(0, len(raw_bytes), 60):
                chunk = raw_bytes[i:i+60]
                hexstr = ' '.join('%02X' % b for b in chunk)
                ascstr = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
                print("  %s" % hexstr)
                print("  %s" % ascstr)

        plain = client.all_text_plain
        idx2 = plain.find("SysOp")
        if idx2 >= 0:
            print("\n=== PLAIN TEXT around 'SysOp Commands' ===")
            print(repr(plain[max(0,idx2-100):idx2+100]))

    finally:
        client.close()

    assert True
