"""Test BBS startup and TCP lifecycle."""

import socket
import time


class TestStartup:
    """Verify the BBS starts, accepts TCP, and can be stopped."""

    def test_bbs_accepts_tcp_connection(self, bbs_instance):
        """BBS starts and accepts a TCP connection."""
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5)
        try:
            s.connect(('127.0.0.1', bbs_instance.port))
            # Should receive telnet negotiation bytes immediately
            data = s.recv(64)
            assert len(data) >= 9, f"Expected telnet negotiation, got {len(data)} bytes"
            # First 9 bytes: IAC WILL ECHO, IAC WILL SGA, IAC DO NAWS
            assert data[:3] == b'\xff\xfb\x01', "Expected IAC WILL ECHO"
            assert data[3:6] == b'\xff\xfb\x03', "Expected IAC WILL SGA"
            assert data[6:9] == b'\xff\xfd\x1f', "Expected IAC DO NAWS"
        finally:
            s.close()

    def test_bbs_sends_ansi_query(self, bbs_client):
        """After telnet negotiation, BBS sends ESC[6n to detect ANSI."""
        # bbs_client auto-responds to ESC[6n via BBSClient._process_incoming
        # Just verify we receive *something* after connect (the welcome/login screen)
        text = bbs_client.recv_until_quiet(quiet_time=2, max_wait=5)
        assert len(text) > 0, "Expected BBS to send data after connection"

    def test_bbs_stops_cleanly(self, bbs_instance):
        """BBS can be killed with SIGTERM."""
        assert bbs_instance.running
        rc = bbs_instance.stop()
        assert not bbs_instance.running

    def test_bbs_exits_after_session(self, bbs_instance):
        """With -Q flag, BBS exits after one user session completes."""
        client = bbs_instance.create_client()
        try:
            # Wait for login prompt, log in
            client.expect("Your Handle or User Number", timeout=10)
            client.send_line("SYSOP")
            client.expect("Password", timeout=10)
            client.send_line("SYSOP")
            # Consume some post-login output
            client.recv_until_quiet(quiet_time=1, max_wait=5)
        finally:
            # Disconnect — BBS detects hangup via cdet() and sets hangup=1
            client.close()

        # Wait for BBS process to exit (due to -Q)
        try:
            bbs_instance.process.wait(timeout=15)
        except Exception:
            pass
        assert not bbs_instance.running, "BBS should exit after session with -Q flag"
