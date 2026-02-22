/*
 * iotest.cpp — Menu-driven I/O test for the clean Terminal layer
 *
 * Exercises all Terminal capabilities (output, input, color, ANSI) over
 * both local ncurses and remote telnet.  ZERO BBS DEPENDENCIES.
 * No vars.h, no vardec.h, no stubs.
 *
 * Build:  make build/iotest
 * Run:    build/iotest            (local only)
 *         build/iotest -P2024     (also accept telnet on port 2024)
 */

#include "terminal.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* ------------------------------------------------------------------ */
/*  Color palette — matches BBS defaults from mkconfig                 */
/* ------------------------------------------------------------------ */

static const unsigned char palette[] = {
    0x07,   /* 0: default text — white on black */
    0x0B,   /* 1: highlight — bright cyan */
    0x0E,   /* 2: input — yellow */
    0x05,   /* 3: YN prompt — magenta */
    0x1F,   /* 4: prompt — bright white on blue */
    0x02,   /* 5: info — green */
    0x0C,   /* 6: warning — bright red */
    0x09,   /* 7: title — bright blue */
    0x06,   /* 8: misc — brown */
    0x03,   /* 9: misc2 — cyan */
};

/* ansic — set color from the palette, like the BBS does */
static void ansic(Terminal &t, int n)
{
    if (n >= 0 && n <= 9)
        t.setAttr(palette[n]);
}

/* prt — colored output */
static void prt(Terminal &t, int color, const char *s)
{
    ansic(t, color);
    t.puts(s);
    ansic(t, 0);
}


/* ------------------------------------------------------------------ */
/*  Input helpers — standalone, no BBS state                           */
/* ------------------------------------------------------------------ */

/* Wait for one key from allowed set (case-insensitive), echo + newline */
static char onek(Terminal &t, const char *allowed)
{
    while (1) {
        unsigned char ch = std::toupper(t.getKey());
        if (std::strchr(allowed, ch)) {
            t.putch(ch);
            t.newline();
            return (char)ch;
        }
    }
}

/* Read a string with backspace editing.
 * lc=0: force uppercase, lc=1: mixed case. */
static int input1(Terminal &t, char *buf, int maxlen, int lc)
{
    int pos = 0;
    while (1) {
        unsigned char ch = t.getKey();
        if (ch == '\r' || ch == '\n') {
            buf[pos] = 0;
            t.newline();
            return pos;
        }
        if (ch == 8) {
            if (pos > 0) {
                pos--;
                t.backspace();
            }
            continue;
        }
        if (ch == 24 || ch == 21) {  /* Ctrl-X or Ctrl-U: clear line */
            while (pos > 0) {
                pos--;
                t.backspace();
            }
            continue;
        }
        if (ch >= 32 && ch < 127 && pos < maxlen) {
            if (!lc) ch = std::toupper(ch);
            buf[pos++] = ch;
            t.putch(ch);
        }
    }
}

/* Draw a blue input field (like BBS mpl) */
static void mpl(Terminal &t, int width)
{
    t.setAttr(0x1F);  /* bright white on blue */
    for (int i = 0; i < width; i++)
        t.putch(177);  /* CP437 ░ field background */
    /* Back cursor to start of field */
    for (int i = 0; i < width; i++)
        t.puts("\x1b[D");
}

/* Blue-field backspace: cursor left, redraw field char, cursor left */
static void backblue(Terminal &t)
{
    t.setAttr(0x1F);
    t.puts("\x1b[D");
    t.putch(177);
    t.puts("\x1b[D");
}

/* Read string inside a blue input field */
static int input_blue(Terminal &t, char *buf, int maxlen, int lc)
{
    t.setAttr(0x1F);
    int pos = 0;
    while (1) {
        unsigned char ch = t.getKey();
        if (ch == '\r' || ch == '\n') {
            buf[pos] = 0;
            t.setAttr(0x07);
            t.newline();
            return pos;
        }
        if (ch == 8) {
            if (pos > 0) {
                pos--;
                backblue(t);
            }
            continue;
        }
        if (ch >= 32 && ch < 127 && pos < maxlen) {
            if (!lc) ch = std::toupper(ch);
            buf[pos++] = ch;
            t.setAttr(0x1F);
            t.putch(ch);
        }
    }
}

/* Yes/No prompt, returns 1 for Yes, 0 for No */
static int yn(Terminal &t, const char *prompt, int default_yes)
{
    prt(t, 3, prompt);
    if (default_yes)
        t.puts(" [Y/n] ");
    else
        t.puts(" [y/N] ");

    while (1) {
        unsigned char ch = std::toupper(t.getKey());
        if (ch == 'Y') { t.puts("Yes"); t.newline(); return 1; }
        if (ch == 'N') { t.puts("No");  t.newline(); return 0; }
        if (ch == '\r') {
            t.puts(default_yes ? "Yes" : "No");
            t.newline();
            return default_yes;
        }
    }
}


/* ------------------------------------------------------------------ */
/*  TCP listener                                                       */
/* ------------------------------------------------------------------ */

static int start_listener(int port)
{
    int fd, opt = 1;
    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); return -1;
    }
    if (listen(fd, 1) < 0) {
        perror("listen"); close(fd); return -1;
    }
    return fd;
}

static int wait_for_connection(Terminal &t, int listen_fd)
{
    t.clearScreen();
    t.setAttr(0x07);
    t.puts("iotest: Waiting for telnet connection...");
    t.newline();
    t.puts("(Press 'q' to start local-only)");
    t.newline();

    while (1) {
        /* Check for local keypress */
        if (t.keyReady()) {
            unsigned char ch = t.getKeyNB();
            if (ch == 'q' || ch == 'Q') return -1;
        }

        /* Check for incoming TCP connection */
        fd_set fds;
        struct timeval tv = {0, 100000};  /* 100ms */
        FD_ZERO(&fds);
        FD_SET(listen_fd, &fds);
        if (select(listen_fd + 1, &fds, NULL, NULL, &tv) > 0) {
            struct sockaddr_in cli;
            socklen_t len = sizeof(cli);
            int cfd = accept(listen_fd, (struct sockaddr *)&cli, &len);
            if (cfd >= 0) {
                t.printf("Connected (fd=%d). Both sides active.", cfd);
                t.newline();
                usleep(500000);
                return cfd;
            }
        }
    }
}


/* ------------------------------------------------------------------ */
/*  Test: getkey() — single key input                                  */
/* ------------------------------------------------------------------ */

static void test_getkey(Terminal &t)
{
    t.newline();
    prt(t, 5, "=== getkey() test ===");
    t.newline();
    prt(t, 3, "Press keys (Q to return to menu):");
    t.newline();

    while (1) {
        unsigned char ch = t.getKey();
        ansic(t, 1);
        t.printf("Key: ");
        ansic(t, 7);
        t.printf("0x%02X  %3d  ", ch, ch);
        if (ch >= 32 && ch < 127) t.printf("'%c'", ch);
        else if (ch == 13) t.puts("ENTER");
        else if (ch == 8)  t.puts("BACKSPACE");
        else if (ch == 27) t.puts("ESC");
        else               t.printf("^%c", ch + 64);
        t.newline();
        if (ch == 'Q' || ch == 'q') break;
    }
}


/* ------------------------------------------------------------------ */
/*  Test: string input                                                 */
/* ------------------------------------------------------------------ */

static void test_input(Terminal &t)
{
    char buf[41];

    t.newline();
    prt(t, 5, "=== input() test (uppercase) ===");
    t.newline();
    prt(t, 3, "Type text (max 30 chars, auto-uppercased): ");
    ansic(t, 2);
    input1(t, buf, 30, 0);
    ansic(t, 1);
    t.printf("You typed: ");
    ansic(t, 7);
    t.printf("\"%s\"", buf);
    t.newline();
}

static void test_inputl(Terminal &t)
{
    char buf[41];

    t.newline();
    prt(t, 5, "=== inputl() test (mixed case) ===");
    t.newline();
    prt(t, 3, "Type text (max 30 chars): ");
    ansic(t, 2);
    input1(t, buf, 30, 1);
    ansic(t, 1);
    t.printf("You typed: ");
    ansic(t, 7);
    t.printf("\"%s\"", buf);
    t.newline();
}


/* ------------------------------------------------------------------ */
/*  Test: blue input field                                             */
/* ------------------------------------------------------------------ */

static void test_blue_field(Terminal &t)
{
    char buf[41];

    t.newline();
    prt(t, 5, "=== Blue field input test ===");
    t.newline();
    prt(t, 3, "Username: ");
    mpl(t, 20);
    input_blue(t, buf, 20, 1);
    ansic(t, 1);
    t.printf("Got: ");
    ansic(t, 7);
    t.printf("\"%s\"", buf);
    t.newline();
}


/* ------------------------------------------------------------------ */
/*  Test: yes/no                                                       */
/* ------------------------------------------------------------------ */

static void test_yesno(Terminal &t)
{
    t.newline();
    prt(t, 5, "=== Yes/No test ===");
    t.newline();

    int r = yn(t, "Do you like BBS systems?", 1);
    ansic(t, 1);
    t.printf("yn() returned: ");
    ansic(t, 7);
    t.printf("%d", r);
    t.newline();

    r = yn(t, "Is DOS superior?", 0);
    ansic(t, 1);
    t.printf("ny() returned: ");
    ansic(t, 7);
    t.printf("%d", r);
    t.newline();
}


/* ------------------------------------------------------------------ */
/*  Test: onek — single key from set                                   */
/* ------------------------------------------------------------------ */

static void test_onek(Terminal &t)
{
    t.newline();
    prt(t, 5, "=== onek() test ===");
    t.newline();
    prt(t, 3, "Pick a letter [A,B,C,D]: ");
    char ch = onek(t, "ABCD");
    ansic(t, 1);
    t.printf("You picked: ");
    ansic(t, 7);
    t.printf("'%c'", ch);
    t.newline();
}


/* ------------------------------------------------------------------ */
/*  Test: Color display                                                */
/* ------------------------------------------------------------------ */

static void test_colors(Terminal &t)
{
    t.newline();
    prt(t, 5, "=== Palette colors (ansic 0-9) ===");
    t.newline();
    for (int i = 0; i <= 9; i++) {
        ansic(t, i);
        t.printf("Color %d: The quick brown fox", i);
        t.newline();
    }
    ansic(t, 0);

    t.newline();
    prt(t, 5, "=== All 16 foreground colors ===");
    t.newline();
    for (int fg = 0; fg < 16; fg++) {
        t.setAttr((unsigned char)fg);
        t.printf("FG %2d: The quick brown fox ", fg);
        t.newline();
    }
    t.setAttr(0x07);

    t.newline();
    prt(t, 5, "=== Background colors ===");
    t.newline();
    for (int bg = 0; bg < 8; bg++) {
        t.setAttr((unsigned char)(0x0F | (bg << 4)));
        t.printf(" BG %d ", bg);
    }
    t.setAttr(0x07);
    t.newline();
}


/* ------------------------------------------------------------------ */
/*  Main menu                                                          */
/* ------------------------------------------------------------------ */

static void show_menu(Terminal &t)
{
    t.newline();
    prt(t, 5, "=== Dominion I/O Test (clean Terminal) ===");
    t.newline();
    ansic(t, 0);
    t.newline();

    struct { char key; const char *label; const char *desc; } items[] = {
        {'1', "getkey()",     "single key input"},
        {'2', "input()",      "uppercase string"},
        {'3', "inputl()",     "mixed case string"},
        {'4', "blue field",   "mpl + input field"},
        {'5', "yn()/ny()",    "yes/no prompts"},
        {'6', "onek()",       "key from set"},
        {'7', "colors",       "palette + all colors"},
        {'Q', "Quit",         ""},
    };

    for (int i = 0; i < 8; i++) {
        ansic(t, 3);
        t.putch('[');
        ansic(t, 7);
        t.putch(items[i].key);
        ansic(t, 3);
        t.printf("] ");
        ansic(t, 2);
        t.printf("%-12s", items[i].label);
        if (items[i].desc[0]) {
            ansic(t, 3);
            t.printf("- %s", items[i].desc);
        }
        t.newline();
    }

    t.newline();
    prt(t, 3, "Choice: ");
}


/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    int port = 0, listen_fd = -1;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'P')
            port = std::atoi(&argv[i][2]);
    }

    Terminal term;

    if (!term.initLocal()) {
        std::fprintf(stderr, "iotest: need a real terminal\n");
        return 1;
    }

    /* TCP listener */
    if (port > 0) {
        listen_fd = start_listener(port);
        if (listen_fd >= 0) {
            int cfd = wait_for_connection(term, listen_fd);
            if (cfd >= 0) {
                term.setRemote(cfd);
                term.sendTelnetNegotiation();
            }
        }
    }

    /* Clear screen via Terminal (goes to both local + remote) */
    term.putch(12);

    /* Main loop */
    bool quit = false;
    while (!quit) {
        show_menu(term);
        char ch = onek(term, "1234567Q");

        switch (ch) {
        case '1': test_getkey(term);      break;
        case '2': test_input(term);       break;
        case '3': test_inputl(term);      break;
        case '4': test_blue_field(term);  break;
        case '5': test_yesno(term);       break;
        case '6': test_onek(term);        break;
        case '7': test_colors(term);      break;
        case 'Q': quit = true;            break;
        }
    }

    /* Cleanup */
    term.closeRemote();
    if (listen_fd >= 0) close(listen_fd);
    term.shutdown();
    return 0;
}
