/*
 * iotest.c — Menu-driven I/O test for Dominion BBS
 *
 * Exercises all IO mechanisms (output, input, color, ANSI) over both
 * local ncurses console and remote telnet.  Runs local-only by default;
 * pass -P<port> to accept a telnet connection on that port.
 *
 * When a telnet client is connected, both sides are live: the sysop
 * console and remote user see the same output, and either can type.
 * This is exactly how the real BBS works.
 *
 * Build:  make build/iotest
 * Run:    build/iotest            (local only)
 *         build/iotest -P2024     (also accept telnet on port 2024)
 */

#include "io_ncurses.h"   /* MUST come before vars.h */

#define _DEFINE_GLOBALS_
#include "vars.h"
#include "cp437.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

/* Globals that BBS modules reference */
char menuat[15];
int node = 0;

/* ------------------------------------------------------------------ */
/*  Stubs — just enough for com.o / conio.o / tcpio.o to link         */
/* ------------------------------------------------------------------ */

int printfile(char *fn)          { (void)fn; return 0; }
void setbeep(int i)              { (void)i; }
void wait1(long l)               { usleep(l * 1000); }
double nsl()                     { return 999.0; }
char *ctim(double d)             { (void)d; return "99:99"; }
int sysop2()                     { return 0; }
void bank2(int m)                { (void)m; }
void filter(char *s, unsigned char c) { (void)s; (void)c; }
int opp(int i)                   { return !i; }
int exist(char *s)               { (void)s; return 0; }
void userdb_load(unsigned int un, userrec *u)  { (void)un; memset(u, 0, sizeof(userrec)); }
int  userdb_save(unsigned int un, userrec *u)  { (void)un; (void)u; return 0; }
void val_cur_user(int i)         { (void)i; }
void set_autoval(int i)          { (void)i; }
void changedsl()                 {}
void reset_act_sl()              {}
char *nam(userrec *u1, unsigned int un) { (void)u1; (void)un; return "Test User"; }
char *ctype(int i)               { (void)i; return ""; }
int numwaiting(userrec *u)       { (void)u; return 0; }
int read_menu(char fn[15], int doauto) { (void)fn; (void)doauto; return 0; }
int runprog(char *s, int swp)    { (void)s; (void)swp; return 0; }
void pr_wait(int i)              { (void)i; }
void fastscreen(char *s)         { (void)s; }
char *get_string(int i)          { (void)i; return ""; }
void chat1(char *s, int i)       { (void)s; (void)i; }
void save_state(char *s, int i)  { (void)s; (void)i; }
void set_global_handle(int i)    { (void)i; }
void sl1(int i, char *s)         { (void)i; (void)s; }
void sysoplog(char *s)           { (void)s; }
void far *malloca(unsigned long nbytes) { return malloc(nbytes); }
int menudb_exists(const char *key) { (void)key; return 0; }
int ex(char type[2], char ms[MAX_PATH_LEN]) { (void)type; (void)ms; return 0; }
int MCISTR[20];
void dtitle(char msg[MAX_PATH_LEN]) { (void)msg; }
void global_char(char ch)       { (void)ch; }
void readms(char *s)             { if(s) s[0]=0; }
void setmci(char ch)             { (void)ch; }
void reprint()                   {}
void ptime()                     {}
long timer1()                    {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 18 + tv.tv_usec / 55555;
}
void jumpconf(char ms[41])       { (void)ms; }
int slok(char val[31], char menu) { (void)val; (void)menu; return 1; }
int checkacs(int w)              { (void)w; return 1; }
int okansi()                     { return 1; }
int so()                         { return 1; }
int cs()                         { return 1; }
int okavt()                      { return 0; }
unsigned int userdb_find(char *name) { (void)name; return 0; }
int userdb_max_num()             { return 1; }


/* ------------------------------------------------------------------ */
/*  TCP listener — accept one telnet connection                       */
/* ------------------------------------------------------------------ */

static int start_listener(int port)
{
    int fd, opt = 1;
    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
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

static int accept_connection(int listen_fd_local)
{
    struct sockaddr_in cli;
    socklen_t len = sizeof(cli);
    int cfd = accept(listen_fd_local, (struct sockaddr *)&cli, &len);
    if (cfd < 0) return -1;

    /* Set up remote stream */
    io.stream[IO_REMOTE].fd_in = cfd;
    io.stream[IO_REMOTE].fd_out = cfd;
    io.stream[IO_REMOTE].in_active = 1;
    io.stream[IO_REMOTE].out_active = 1;
    io.stream[IO_REMOTE].available = 1;
    io.stream[IO_REMOTE].needs_iac = 1;

    send_telnet_negotiation(cfd);
    return cfd;
}


/* ------------------------------------------------------------------ */
/*  Test: getkey() — single key input                                 */
/* ------------------------------------------------------------------ */

static void test_getkey(void)
{
    unsigned char ch;

    pl("");
    prt(5, "=== getkey() test ===");
    nl();
    prt(3, "Press keys (Q to return to menu):");
    nl();

    while (!hangup) {
        ch = getkey();
        npr("\x03" "1Key: \x03" "70x%02X  %3d  ", ch, ch);
        if (ch >= 32 && ch < 127) npr("'%c'", ch);
        else if (ch == 13) npr("ENTER");
        else if (ch == 8)  npr("BACKSPACE");
        else if (ch == 27) npr("ESC");
        else               npr("^%c", ch + 64);
        nl();
        if (ch == 'Q' || ch == 'q') break;
    }
}


/* ------------------------------------------------------------------ */
/*  Test: input() — uppercase string input                            */
/* ------------------------------------------------------------------ */

static void test_input(void)
{
    char buf[41];

    nl();
    prt(5, "=== input() test (uppercase) ===");
    nl();
    prt(3, "Type text (max 30 chars, auto-uppercased): ");
    nl();
    ansic(2);
    mpl(30);
    input(buf, 30);
    nl();
    npr("\x03" "1You typed: \x03" "7\"%s\"", buf);
    nl();
}


/* ------------------------------------------------------------------ */
/*  Test: inputl() — mixed case string input                          */
/* ------------------------------------------------------------------ */

static void test_inputl(void)
{
    char buf[41];

    nl();
    prt(5, "=== inputl() test (mixed case) ===");
    nl();
    prt(3, "Type text (max 30 chars): ");
    nl();
    ansic(2);
    mpl(30);
    inputl(buf, 30);
    nl();
    npr("\x03" "1You typed: \x03" "7\"%s\"", buf);
    nl();
}


/* ------------------------------------------------------------------ */
/*  Test: inputdate() — date entry                                    */
/* ------------------------------------------------------------------ */

static void test_inputdate(void)
{
    char buf[41];

    nl();
    prt(5, "=== inputdate() test ===");
    nl();
    prt(3, "Enter a date (MM/DD/YY): ");
    memset(buf, 0, sizeof(buf));
    inputdate(buf, 0);
    nl();
    npr("\x03" "1Date: \x03" "7\"%s\"", buf);
    nl();
}


/* ------------------------------------------------------------------ */
/*  Test: inputfone() — phone number entry                            */
/* ------------------------------------------------------------------ */

static void test_inputfone(void)
{
    char buf[41];

    nl();
    prt(5, "=== inputfone() test ===");
    nl();
    prt(3, "Enter phone (###-###-####): ");
    memset(buf, 0, sizeof(buf));
    inputfone(buf);
    nl();
    npr("\x03" "1Phone: \x03" "7\"%s\"", buf);
    nl();
}


/* ------------------------------------------------------------------ */
/*  Test: yn() / ny() — yes/no prompts                                */
/* ------------------------------------------------------------------ */

static void test_yesno(void)
{
    int r;

    nl();
    prt(5, "=== yn() / ny() test ===");
    nl();
    prt(3, "Do you like BBS systems (Y/n)? ");
    r = yn();
    npr("\x03" "1yn() returned: \x03" "7%d", r);
    nl();
    prt(3, "Is DOS superior (y/N)? ");
    r = ny();
    npr("\x03" "1ny() returned: \x03" "7%d", r);
    nl();
}


/* ------------------------------------------------------------------ */
/*  Test: onek() — single key from allowed set                        */
/* ------------------------------------------------------------------ */

static void test_onek(void)
{
    char ch;

    nl();
    prt(5, "=== onek() test ===");
    nl();
    prt(3, "Pick a letter [A,B,C,D]: ");
    ch = onek("ABCD");
    nl();
    npr("\x03" "1You picked: \x03" "7'%c'", ch);
    nl();
}


/* ------------------------------------------------------------------ */
/*  Test: mpl() + input1() — blue input field                         */
/* ------------------------------------------------------------------ */

static void test_mpl_input(void)
{
    char buf[41];

    nl();
    prt(5, "=== mpl() + input1() blue field test ===");
    nl();
    prt(3, "Username: ");
    memset(buf, 0, sizeof(buf));
    mpl(20);
    input1(buf, 20, 1, 1);
    nl();
    npr("\x03" "1Got: \x03" "7\"%s\"", buf);
    nl();
}


/* ------------------------------------------------------------------ */
/*  Test: Color display — all ansic() colors                          */
/* ------------------------------------------------------------------ */

static void test_colors(void)
{
    int i;

    nl();
    prt(5, "=== Color test — ansic(0) through ansic(9) ===");
    nl();
    for (i = 0; i <= 9; i++) {
        ansic(i);
        npr("Color %d: The quick brown fox ", i);
    }
    ansic(0);
    nl();

    prt(5, "=== Ctrl-3 colors 0 through 9 ===");
    nl();
    npr("\x03" "0 Color 0 \x03" "1 Color 1 \x03" "2 Color 2 \x03" "3 Color 3 \x03" "4 Color 4");
    nl();
    npr("\x03" "5 Color 5 \x03" "6 Color 6 \x03" "7 Color 7 \x03" "8 Color 8 \x03" "9 Color 9");
    nl();
    ansic(0);
}


/* ------------------------------------------------------------------ */
/*  Main menu                                                          */
/* ------------------------------------------------------------------ */

static void show_menu(void)
{
    nl();
    prt(5, "=== Dominion I/O Test ===");
    nl();
    ansic(0); nl();
    npr("\x03" "3[\x03" "71\x03" "3] \x03" "2getkey()     \x03" "3- single key");          nl();
    npr("\x03" "3[\x03" "72\x03" "3] \x03" "2input()      \x03" "3- uppercase line");       nl();
    npr("\x03" "3[\x03" "73\x03" "3] \x03" "2inputl()     \x03" "3- mixed case line");      nl();
    npr("\x03" "3[\x03" "74\x03" "3] \x03" "2inputdate()  \x03" "3- date entry");           nl();
    npr("\x03" "3[\x03" "75\x03" "3] \x03" "2inputfone()  \x03" "3- phone entry");          nl();
    npr("\x03" "3[\x03" "76\x03" "3] \x03" "2yn() / ny()  \x03" "3- yes/no");              nl();
    npr("\x03" "3[\x03" "77\x03" "3] \x03" "2onek()       \x03" "3- key from set");         nl();
    npr("\x03" "3[\x03" "78\x03" "3] \x03" "2mpl()+input1()\x03" "3 - blue field");         nl();
    npr("\x03" "3[\x03" "79\x03" "3] \x03" "2Color test   \x03" "3- all ansic() colors");   nl();
    npr("\x03" "3[\x03" "7Q\x03" "3] \x03" "2Quit");                                        nl();
    nl();
    prt(3, "Choice: ");
}


/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */

void main(int argc, char *argv[])
{
    int i, port = 0, lfd = -1;
    char ch;

    /* Parse args */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'P') {
            port = atoi(&argv[i][2]);
        }
    }

    /* Initialize I/O and BBS globals */
    io_init(&io);
    screenbottom = 24;
    topline = 0;
    curatr = 0x07;
    defscreenbottom = 24;
    screenlen = 4000;
    scrn = malloc(screenlen);
    if (scrn) memset(scrn, 0, screenlen);
    lines_listed = 0;
    x_only = 0;
    change_color = 0;
    change_ecolor = 0;
    hangup = 0;
    okskey = 0;
    chatting = 0;
    global_handle = 0;
    okmacro = 0;
    input_extern = 0;
    in_extern = 0;
    client_fd = -1;
    listen_fd = -1;

    thisuser.screenchars = 80;
    thisuser.screenlines = 25;
    thisuser.sysstatus = 0x0004;  /* sysstatus_ansi */
    actsl = 255;

    /* Color tables — same defaults as mkconfig */
    nifty.defaultcol[0]  = 7;    /* default text: white on black */
    nifty.defaultcol[1]  = 11;   /* highlight: bright cyan */
    nifty.defaultcol[2]  = 14;   /* input: yellow */
    nifty.defaultcol[3]  = 5;    /* YN prompt: magenta */
    nifty.defaultcol[4]  = 31;   /* prompt: bright white on blue */
    nifty.defaultcol[5]  = 2;    /* info: green */
    nifty.defaultcol[6]  = 12;   /* warning: bright red */
    nifty.defaultcol[7]  = 9;    /* title: bright blue */
    nifty.defaultcol[8]  = 6;    /* misc: brown */
    nifty.defaultcol[9]  = 3;    /* misc2: cyan */
    memcpy(thisuser.colors, nifty.defaultcol, 20);
    mciok = 1;  /* enable MCI/pipe color expansion */

    /* Init ncurses */
    ncurses_init();
    if (!nc_active) {
        fprintf(stderr, "iotest: need a real terminal\n");
        exit(1);
    }
    term_raw_mode = 1;

    /* TCP listener */
    if (port > 0) {
        lfd = start_listener(port);
        if (lfd >= 0) {
            listen_fd = lfd;
            /* Show waiting message on local console */
            clear();
            mvprintw(0, 0, "iotest: Listening on port %d. Waiting for telnet connection...", port);
            mvprintw(1, 0, "(Press 'q' on console to start local-only, or connect via telnet)");
            refresh();

            /* Non-blocking wait — accept or let user skip */
            nodelay(stdscr, TRUE);
            while (1) {
                fd_set fds;
                struct timeval tv;
                int r, kch;

                /* Check for local keypress */
                kch = wgetch(stdscr);
                if (kch == 'q' || kch == 'Q') break;

                /* Check for incoming connection */
                FD_ZERO(&fds);
                FD_SET(lfd, &fds);
                tv.tv_sec = 0;
                tv.tv_usec = 100000; /* 100ms */
                r = select(lfd + 1, &fds, NULL, NULL, &tv);
                if (r > 0) {
                    int cfd = accept_connection(lfd);
                    if (cfd >= 0) {
                        mvprintw(2, 0, "Telnet client connected (fd=%d). Both sides active.", cfd);
                        refresh();
                        usleep(500000);
                        break;
                    }
                }
            }
        }
    }

    /* Clear screen via BBS output path (goes to both local + remote) */
    outchr(12);  /* form feed = cls */

    /* Main loop */
    while (!hangup) {
        show_menu();
        ch = onek("123456789Q");

        switch (ch) {
            case '1': test_getkey();       break;
            case '2': test_input();        break;
            case '3': test_inputl();       break;
            case '4': test_inputdate();    break;
            case '5': test_inputfone();    break;
            case '6': test_yesno();        break;
            case '7': test_onek();         break;
            case '8': test_mpl_input();    break;
            case '9': test_colors();       break;
            case 'Q': hangup = 1;         break;
        }
    }

    /* Cleanup */
    if (client_fd >= 0) close(client_fd);
    if (lfd >= 0) close(lfd);
    ncurses_shutdown();
    if (scrn) free(scrn);
}
