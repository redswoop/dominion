#include "vars.h"
#include "cp437.h"
#pragma hdrstop

/*
 * x00com.c — TCP socket I/O layer
 *
 * Replaces the FOSSIL (INT 14h) serial port driver with TCP socket operations.
 * listen_fd = server socket (listens for incoming connections)
 * client_fd = connected client socket (the remote user)
 */

/* Telnet protocol bytes */
#define IAC   255
#define WILL  251
#define WONT  252
#define DO    253
#define DONT  254
#define IAC_SB  250
#define IAC_SE  240

#define TELOPT_ECHO        1
#define TELOPT_SGA         3
#define TELOPT_TTYPE       24
#define TELOPT_NAWS        31

/* State for filtering IAC sequences from incoming data */
static int _iac_state = 0;

/* Filter telnet IAC sequences from incoming byte stream.
 * Returns 1 if the byte is real data, 0 if it was consumed by IAC handling. */
static int _telnet_filter(unsigned char *ch)
{
    unsigned char c = *ch;

    switch (_iac_state) {
    case 0: /* normal */
        if (c == IAC) {
            _iac_state = 1;
            return 0;
        }
        return 1;
    case 1: /* got IAC */
        if (c == IAC) {
            /* IAC IAC = literal 0xFF */
            _iac_state = 0;
            *ch = 0xFF;
            return 1;
        }
        if (c == WILL || c == WONT || c == DO || c == DONT) {
            _iac_state = 2; /* expect option byte */
            return 0;
        }
        if (c == IAC_SB) {
            _iac_state = 3; /* subnegotiation */
            return 0;
        }
        _iac_state = 0;
        return 0;
    case 2: /* option byte after WILL/WONT/DO/DONT */
        _iac_state = 0;
        return 0;
    case 3: /* subnegotiation data — consume until IAC SE */
        if (c == IAC)
            _iac_state = 4;
        return 0;
    case 4: /* got IAC inside subneg */
        _iac_state = (c == IAC_SE) ? 0 : 3;
        return 0;
    }
    _iac_state = 0;
    return 0;
}


void dtr(int i)
/* dtr(0) = hang up (close client socket), dtr(1) = ready */
{
    if (!ok_modem_stuff) return;

    if (i == 0 && client_fd >= 0) {
        close(client_fd);
        io.stream[IO_REMOTE].fd_in = -1;
        io.stream[IO_REMOTE].fd_out = -1;
    }
    /* dtr(1) is a no-op for TCP — we're always "ready" */
}

void outcomch(char ch)
/* Output one character to the remote user via TCP */
{
    unsigned char uch = (unsigned char)ch;
    const char *utf8;
    if (!ok_modem_stuff) return;
    if (client_fd < 0) return;
    if (uch >= 0x80) {
        utf8 = cp437_to_utf8[uch];
        write(client_fd, utf8, strlen(utf8));
    } else {
        write(client_fd, &ch, 1);
    }
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
    int n;
    struct timeval tv = {0, 0};
    fd_set fds;

    if (!ok_modem_stuff) return 0;
    if (client_fd < 0) return 0;

    /* Non-blocking check */
    FD_ZERO(&fds);
    FD_SET(client_fd, &fds);
    if (select(client_fd + 1, &fds, NULL, NULL, &tv) <= 0)
        return 0;

    /* Read and filter telnet sequences */
    while (1) {
        n = read(client_fd, &ch, 1);
        if (n <= 0) {
            if (n == 0) {
                /* Client disconnected (EOF).  Close the socket so comhit()
                 * stops reporting data available, allowing getkey()'s inner
                 * loop to run checkhangup() and detect the disconnect. */
                close(client_fd);
                io.stream[IO_REMOTE].fd_in = -1;
                io.stream[IO_REMOTE].fd_out = -1;
            }
            return 0;
        }
        if (_telnet_filter(&ch))
            return (char)ch;
        /* Was an IAC byte — check if more data available */
        FD_ZERO(&fds);
        FD_SET(client_fd, &fds);
        if (select(client_fd + 1, &fds, NULL, NULL, &tv) <= 0)
            return 0;
    }
}

int comhit()
/* Check if data is available from the remote user */
{
    struct timeval tv = {0, 0};
    fd_set fds;

    if (!ok_modem_stuff) return 0;
    if (client_fd < 0) return 0;

    FD_ZERO(&fds);
    FD_SET(client_fd, &fds);
    return (select(client_fd + 1, &fds, NULL, NULL, &tv) > 0);
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
 * If tcp_port is set, listen on that port.
 * Otherwise this is a no-op (local-only mode). */
{
    struct sockaddr_in addr;
    int opt = 1;
    int port;

    if (!ok_modem_stuff) return;
    if (!tcp_port) return;  /* no TCP port configured */
    if (listen_fd >= 0) return;  /* already listening */

    port = tcp_port;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        printf("\n\nFailed to create TCP socket!\n\n");
        return;
    }

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("\n\nFailed to bind to port %d: %s\n\n", port, strerror(errno));
        close(listen_fd);
        listen_fd = -1;
        return;
    }

    if (listen(listen_fd, 1) < 0) {
        printf("\n\nFailed to listen on port %d: %s\n\n", port, strerror(errno));
        close(listen_fd);
        listen_fd = -1;
        return;
    }

    printf("TCP listening on port %d\n", port);
    fflush(stdout);
    (void)port_num;
}

void closeport()
/* Close the listening socket */
{
    if (!ok_modem_stuff) return;

    if (client_fd >= 0) {
        close(client_fd);
        client_fd = -1;
    }
    if (listen_fd >= 0) {
        close(listen_fd);
        listen_fd = -1;
    }
}


int cdet()
/* Check if the remote client is still connected.
 * Returns non-zero if connected (mimics carrier detect). */
{
    char buf;
    int n;

    if (!ok_modem_stuff) return 0;
    if (client_fd < 0) return 0;

    /* Use peek to test if connection is alive */
    n = recv(client_fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
    if (n == 0) return 0;        /* EOF — client disconnected */
    if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        return 0;                /* error — treat as disconnected */
    return 0x80;                 /* connected (bit 7 set, like modem DCD) */
}


/* Send initial telnet negotiation to a new client */
void send_telnet_negotiation(int fd)
{
    unsigned char neg[] = {
        IAC, WILL, TELOPT_ECHO,    /* BBS will handle echo */
        IAC, WILL, TELOPT_SGA,     /* Suppress go-ahead (full duplex) */
        IAC, DO,   TELOPT_NAWS,    /* Request window size */
    };
    write(fd, neg, sizeof(neg));
    _iac_state = 0;
}

/* Switch remote terminal to alternate screen with black background */
void send_terminal_init(int fd)
{
    /* ?1049h = alt screen, 0m = reset attrs, 40m = black bg, 2J = clear, H = home */
    const char *seq = "\033[?1049h\033[0m\033[40m\033[2J\033[H";
    write(fd, seq, strlen(seq));
}

/* Restore remote terminal to primary screen */
void send_terminal_restore(int fd)
{
    const char *seq = "\033[0m\033[?1049l";
    write(fd, seq, strlen(seq));
}
