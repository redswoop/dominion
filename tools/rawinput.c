/*
 * rawinput.c — Raw byte inspector for keyboard/TCP input
 *
 * Shows the exact hex value of every byte received, so you can see
 * what your terminal sends for backspace, enter, arrow keys, etc.
 *
 * Build:  make rawinput    (or: cc -o rawinput rawinput.c)
 * Run:    build/rawinput          # local stdin mode
 *         build/rawinput -P2023   # TCP mode, connect with: telnet 127.0.0.1 2023
 *
 * Press Ctrl-C to quit (local mode) or disconnect (TCP mode).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <ctype.h>

static struct termios orig_termios;
static int raw_mode = 0;
static volatile int quit = 0;

static void restore_term(void)
{
    if (raw_mode) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_mode = 0;
    }
}

static void sighandler(int sig)
{
    (void)sig;
    quit = 1;
}

static void enter_raw(void)
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw_mode = 1;
    atexit(restore_term);
}

static char *byte_name(unsigned char ch)
{
    static char buf[32];
    static char *ctrl_names[] = {
        "NUL","SOH","STX","ETX","EOT","ENQ","ACK","BEL",
        "BS", "TAB","LF", "VT", "FF", "CR", "SO", "SI",
        "DLE","DC1","DC2","DC3","DC4","NAK","SYN","ETB",
        "CAN","EM", "SUB","ESC","FS", "GS", "RS", "US"
    };
    if (ch < 32)
        return ctrl_names[ch];
    if (ch == 127)
        return "DEL";
    if (ch >= 128) {
        sprintf(buf, "0x%02X", ch);
        return buf;
    }
    sprintf(buf, "'%c'", ch);
    return buf;
}

static void print_byte(unsigned char ch, int seq)
{
    printf("  [%3d] 0x%02X  %-6s", seq, ch, byte_name(ch));
    /* BBS input1() interpretation */
    if (ch == 8)
        printf("  <- input1: BACKSPACE");
    else if (ch == 127)
        printf("  <- skey1 maps to 0x08 (BS) <- input1: BACKSPACE");
    else if (ch == 13)
        printf("  <- input1: ENTER (done)");
    else if (ch == 10)
        printf("  <- input1: IGNORED (no case for LF)");
    else if (ch == 27)
        printf("  <- input1: starts ANSI escape");
    else if (ch > 31)
        printf("  <- input1: PRINTABLE (added to buffer)");
    else
        printf("  <- input1: ctrl char (switch default: ignored)");
    printf("\n");
    fflush(stdout);
}

/* ---- Local stdin mode ---- */
static void run_local(void)
{
    unsigned char ch;
    int seq = 0;

    printf("=== rawinput: LOCAL MODE (stdin) ===\n");
    printf("Terminal is in raw mode. Press keys to see their byte values.\n");
    printf("Press Ctrl-\\ (0x1C) to quit.\n\n");
    printf("  [ seq] hex   name   BBS interpretation\n");
    printf("  -----  ----  -----  ------------------\n");
    fflush(stdout);

    enter_raw();

    while (!quit) {
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            seq++;
            print_byte(ch, seq);
            if (ch == 0x1C) /* Ctrl-\ */
                break;
        }
    }
}

/* ---- TCP mode ---- */

/* Telnet constants */
#define IAC   255
#define WILL  251
#define WONT  252
#define DO    253
#define DONT  254
#define SB    250
#define SE    240

static void run_tcp(int port)
{
    int srv, cli, opt = 1;
    struct sockaddr_in addr;
    unsigned char ch;
    int seq = 0;
    int iac_state = 0;

    printf("=== rawinput: TCP MODE (port %d) ===\n", port);
    printf("Connect with:  telnet 127.0.0.1 %d\n", port);
    printf("Or:            nc 127.0.0.1 %d\n\n", port);
    fflush(stdout);

    srv = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
    listen(srv, 1);

    printf("Listening... ");
    fflush(stdout);

    cli = accept(srv, NULL, NULL);
    if (cli < 0) {
        perror("accept");
        exit(1);
    }
    printf("connected!\n\n");

    /* Send telnet negotiations (same as BBS) */
    {
        unsigned char neg[] = {
            IAC, WILL, 1,   /* WILL ECHO */
            IAC, WILL, 3,   /* WILL SGA */
            IAC, DO, 31,    /* DO NAWS */
        };
        write(cli, neg, sizeof(neg));
    }

    printf("  [ seq] hex   name   BBS interpretation\n");
    printf("  -----  ----  -----  ------------------\n");
    printf("  (telnet IAC sequences shown but filtered)\n\n");
    fflush(stdout);

    while (!quit) {
        int n = read(cli, &ch, 1);
        if (n <= 0)
            break;

        seq++;

        /* Show IAC sequences separately */
        switch (iac_state) {
        case 0:
            if (ch == IAC) {
                printf("  [%3d] 0x%02X  IAC    <- telnet: start command\n", seq, ch);
                iac_state = 1;
            } else {
                print_byte(ch, seq);
            }
            break;
        case 1:
            printf("  [%3d] 0x%02X  ", seq, ch);
            if (ch == IAC) {
                printf("IAC    <- telnet: literal 0xFF\n");
                iac_state = 0;
            } else if (ch == WILL) {
                printf("WILL   <- telnet: WILL (next=option)\n");
                iac_state = 2;
            } else if (ch == WONT) {
                printf("WONT   <- telnet: WONT (next=option)\n");
                iac_state = 2;
            } else if (ch == DO) {
                printf("DO     <- telnet: DO (next=option)\n");
                iac_state = 2;
            } else if (ch == DONT) {
                printf("DONT   <- telnet: DONT (next=option)\n");
                iac_state = 2;
            } else if (ch == SB) {
                printf("SB     <- telnet: subnegotiation start\n");
                iac_state = 3;
            } else {
                printf("cmd=%d  <- telnet: command\n", ch);
                iac_state = 0;
            }
            break;
        case 2: /* option byte */
            printf("  [%3d] 0x%02X  opt=%-3d<- telnet: option\n", seq, ch, ch);
            iac_state = 0;
            break;
        case 3: /* subneg data */
            printf("  [%3d] 0x%02X  sub    <- telnet: subneg data\n", seq, ch);
            if (ch == IAC)
                iac_state = 4;
            break;
        case 4:
            printf("  [%3d] 0x%02X  ", seq, ch);
            if (ch == SE) {
                printf("SE     <- telnet: subneg end\n");
                iac_state = 0;
            } else {
                printf("sub    <- telnet: subneg data\n");
                iac_state = 3;
            }
            break;
        }
        fflush(stdout);
    }

    printf("\n--- disconnected ---\n");
    close(cli);
    close(srv);
}

int main(int argc, char *argv[])
{
    int port = 0;
    int i;

    signal(SIGINT, sighandler);
    signal(SIGPIPE, SIG_IGN);

    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-P", 2) == 0 || strncmp(argv[i], "-p", 2) == 0) {
            port = atoi(argv[i] + 2);
        }
    }

    if (port > 0)
        run_tcp(port);
    else
        run_local();

    return 0;
}
