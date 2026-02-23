#include "tcpio.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "session.h"
#include "system.h"
#include "terminal_bridge.h"
#pragma hdrstop

/*
 * tcpio.c — TCP socket I/O layer (Terminal-backed)
 *
 * Originally x00com.c (FOSSIL INT 14h serial driver), now pure TCP sockets.
 * Remote I/O (output, input, telnet, IAC) delegates to Terminal via bridge.
 * Listen socket management stays here.
 *
 * sys.listen_fd = server socket (listens for incoming connections)
 * client_fd = connected client socket (macro → io.stream[IO_REMOTE].fd_out)
 */


void dtr(int i)
/* dtr(0) = hang up (close client socket), dtr(1) = ready */
{
    if (!ok_modem_stuff) return;

    if (i == 0 && client_fd >= 0) {
        term_detach_remote();
        close(client_fd);
        io.stream[IO_REMOTE].fd_in = -1;
        io.stream[IO_REMOTE].fd_out = -1;
    }
    /* dtr(1) is a no-op for TCP — we're always "ready" */
}

void outcomch(char ch)
/* Output one character to the remote user via TCP (CP437→UTF-8) */
{
    if (!ok_modem_stuff) return;
    if (client_fd < 0) return;
    term_remote_putch((unsigned char)ch);
}

char peek1c()
{
    unsigned char ch;
    int n;

    if (!ok_modem_stuff) return 0;
    if (client_fd < 0) return 0;

    n = recv(client_fd, &ch, 1, MSG_PEEK);
    if (n == 1)
        return (char)ch;
    return 0;
}

char get1c()
/* Read one character from the remote user, non-blocking.
 * Filters out telnet IAC sequences. Returns 0 if nothing available. */
{
    unsigned char ch;

    if (!ok_modem_stuff) return 0;
    if (client_fd < 0) return 0;

    ch = term_remote_get_key();
    if (!ch) {
        /* Terminal returns 0 for nothing available or disconnect.
         * If disconnected, Terminal already closed its fd.
         * Sync io.stream to reflect the close. */
        if (!term_remote_connected() && client_fd >= 0) {
            close(client_fd);
            io.stream[IO_REMOTE].fd_in = -1;
            io.stream[IO_REMOTE].fd_out = -1;
        }
        return 0;
    }
    return (char)ch;
}

int comhit()
/* Check if data is available from the remote user */
{
    if (!ok_modem_stuff) return 0;
    if (client_fd < 0) return 0;
    return term_remote_data_ready();
}

void dump()
/* Drain pending data from the client socket */
{
    unsigned char buf[256];
    struct timeval tv = {0, 0};
    fd_set fds;

    if (!ok_modem_stuff) return;
    if (client_fd < 0) return;

    while (1) {
        FD_ZERO(&fds);
        FD_SET(client_fd, &fds);
        if (select(client_fd + 1, &fds, NULL, NULL, &tv) <= 0)
            break;
        if (read(client_fd, buf, sizeof(buf)) <= 0)
            break;
    }
}

void set_baud(unsigned int rate)
/* No-op for TCP — baud rate is meaningless */
{
    (void)rate;
}


void initport(int port_num)
/* Initialize the TCP listening socket.
 * If sys.tcp_port is set, listen on that port.
 * Otherwise this is a no-op (local-only mode). */
{
    auto& sys = System::instance();
    struct sockaddr_in addr;
    int opt = 1;
    int port;

    if (!ok_modem_stuff) return;
    if (!sys.tcp_port) return;  /* no TCP port configured */
    if (sys.listen_fd >= 0) return;  /* already listening */

    port = sys.tcp_port;

    sys.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sys.listen_fd < 0) {
        printf("\n\nFailed to create TCP socket!\n\n");
        return;
    }

    setsockopt(sys.listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sys.listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("\n\nFailed to bind to port %d: %s\n\n", port, strerror(errno));
        close(sys.listen_fd);
        sys.listen_fd = -1;
        return;
    }

    if (listen(sys.listen_fd, 1) < 0) {
        printf("\n\nFailed to listen on port %d: %s\n\n", port, strerror(errno));
        close(sys.listen_fd);
        sys.listen_fd = -1;
        return;
    }

    printf("TCP listening on port %d\n", port);
    fflush(stdout);
    (void)port_num;
}

void closeport()
/* Close the listening and client sockets */
{
    auto& sys = System::instance();
    if (!ok_modem_stuff) return;

    if (client_fd >= 0) {
        term_detach_remote();
        close(client_fd);
        client_fd = -1;
    }
    if (sys.listen_fd >= 0) {
        close(sys.listen_fd);
        sys.listen_fd = -1;
    }
}


int cdet()
/* Check if the remote client is still connected.
 * Returns non-zero if connected (mimics carrier detect). */
{
    if (!ok_modem_stuff) return 0;
    if (client_fd < 0) return 0;
    return term_remote_connected() ? 0x80 : 0;
}


/* Send initial telnet negotiation to a new client.
 * Ensures Terminal knows about this fd, then delegates. */
void send_telnet_negotiation(int fd)
{
    term_set_remote(fd);
    term_send_telnet_negotiation();
}

/* Switch remote terminal to alternate screen with black background */
void send_terminal_init(int fd)
{
    (void)fd;  /* Terminal already has the fd from send_telnet_negotiation */
    term_send_terminal_init();
}

/* Restore remote terminal to primary screen */
void send_terminal_restore(int fd)
{
    (void)fd;
    term_send_terminal_restore();
}
