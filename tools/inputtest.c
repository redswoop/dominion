/*
 * inputtest.c — Keyboard input diagnostic for ncurses BBS
 *
 * Uses ncurses directly for display (no BBS output pipeline).
 * Tests raw wgetch, then BBS getkey(), then input1().
 *
 * Build:  make build/inputtest
 * Run:    build/inputtest
 */

#include "io_ncurses.h"   /* MUST come before vars.h */

#define _DEFINE_GLOBALS_
#include "vars.h"
#include "cp437.h"

/* Globals that BBS modules reference */
char menuat[15];
int node = 0;

/* Stubs — just enough for com.o / conio.o to link */
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
int okavt()                      { return 1; }
unsigned int userdb_find(char *name) { (void)name; return 0; }
int userdb_max_num()             { return 1; }


/* ------------------------------------------------------------------ */
/*  Test 1: Raw wgetch — see exactly what ncurses gives us            */
/* ------------------------------------------------------------------ */

static void test_raw_wgetch(void)
{
    int ch, row, seq = 0;

    clear();
    mvprintw(0, 0, "=== Test 1: Raw ncurses wgetch() ===");
    mvprintw(1, 0, "Press keys to see codes. 'q' = next test, ESC = quit.");
    mvprintw(3, 0, " seq  value  hex     name");
    mvprintw(4, 0, " ---  -----  ------  ----");
    row = 5;
    refresh();

    nodelay(stdscr, FALSE);  /* blocking */

    while (1) {
        ch = wgetch(stdscr);
        if (ch == ERR) continue;
        seq++;

        if (row >= LINES - 1) {
            row = 5;
            move(5, 0);
            clrtobot();
        }

        mvprintw(row, 0, " %3d  %5d  0x%04X  ", seq, ch, ch);

        if (ch == KEY_BACKSPACE)     printw("KEY_BACKSPACE");
        else if (ch == KEY_DC)       printw("KEY_DC (Delete)");
        else if (ch == KEY_ENTER)    printw("KEY_ENTER");
        else if (ch == KEY_UP)       printw("KEY_UP");
        else if (ch == KEY_DOWN)     printw("KEY_DOWN");
        else if (ch == KEY_LEFT)     printw("KEY_LEFT");
        else if (ch == KEY_RIGHT)    printw("KEY_RIGHT");
        else if (ch == KEY_HOME)     printw("KEY_HOME");
        else if (ch == KEY_END)      printw("KEY_END");
        else if (ch == KEY_F(1))     printw("F1");
        else if (ch == KEY_F(2))     printw("F2");
        else if (ch == KEY_F(3))     printw("F3");
        else if (ch == KEY_F(10))    printw("F10");
        else if (ch == 127)          printw("127 (DEL / macOS backspace)");
        else if (ch == 8)            printw("8 (BS)");
        else if (ch == 13)           printw("13 (CR)");
        else if (ch == 10)           printw("10 (LF / Enter in raw)");
        else if (ch == 27)           printw("27 (ESC)");
        else if (ch == 9)            printw("9 (TAB)");
        else if (ch >= 256)          printw("SPECIAL(%d)", ch);
        else if (ch >= 32 && ch < 127) printw("'%c'", ch);
        else                         printw("^%c", ch + 64);

        row++;
        refresh();

        if (ch == 'q' || ch == 'Q') break;
        if (ch == 27) return;
    }

    nodelay(stdscr, TRUE);
}


/* ------------------------------------------------------------------ */
/*  Test 2: BBS getkey() — after our ncurses→DOS mappings             */
/* ------------------------------------------------------------------ */

static void test_getkey_mapped(void)
{
    unsigned char ch;
    int row, seq = 0;

    clear();
    mvprintw(0, 0, "=== Test 2: BBS getkey() ===");
    mvprintw(1, 0, "Press keys. 'q' = next test, ESC = quit.");
    mvprintw(3, 0, " seq  hex   dec  meaning");
    mvprintw(4, 0, " ---  ----  ---  -------");
    row = 5;
    refresh();

    while (!hangup) {
        ch = getkey();
        seq++;

        if (row >= LINES - 1) {
            row = 5;
            move(5, 0);
            clrtobot();
        }

        mvprintw(row, 0, " %3d  0x%02X  %3d  ", seq, ch, ch);

        if (ch == 8)             printw("BACKSPACE (good!)");
        else if (ch == 13)       printw("ENTER (good!)");
        else if (ch == 127)      printw("DEL (BUG - should be 8)");
        else if (ch == 0)        printw("NUL (scan code prefix)");
        else if (ch == 27)       printw("ESC");
        else if (ch >= 32 && ch < 127) printw("'%c'", ch);
        else                     printw("ctrl-%c", ch + 64);

        row++;
        refresh();

        if (ch == 'q' || ch == 'Q') break;
        if (ch == 27) { hangup = 1; break; }
    }
}


/* ------------------------------------------------------------------ */
/*  Test 3: BBS input1() — real editable input field                  */
/* ------------------------------------------------------------------ */

static void test_bbs_input1(void)
{
    char buf[41];

    clear();
    mvprintw(0, 0, "=== Test 3: BBS input1() ===");
    mvprintw(1, 0, "Type text, use BACKSPACE to delete, ENTER to submit.");
    mvprintw(2, 0, "(This uses the real BBS input1 function)");
    mvprintw(4, 0, "Prompt> ");
    refresh();

    /* Position conio cursor to match where ncurses cursor is */
    movecsr(8, 4);
    curatr = 0x1F;  /* white on blue, like BBS input fields */

    memset(buf, 0, sizeof(buf));
    mpl(20);
    input1(buf, 20, 1, 1);

    mvprintw(6, 0, "Result: \"%s\" (%d chars)", buf, (int)strlen(buf));
    mvprintw(7, 0, "Press any key...");
    refresh();
    nodelay(stdscr, FALSE);
    wgetch(stdscr);
    nodelay(stdscr, TRUE);
}


/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */

void main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

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

    /* Init ncurses */
    ncurses_init();
    if (!nc_active) {
        fprintf(stderr, "inputtest: need a real terminal\n");
        exit(1);
    }
    term_raw_mode = 1;

    /* Run tests */
    test_raw_wgetch();
    if (!hangup) test_getkey_mapped();
    if (!hangup) test_bbs_input1();

    if (!hangup) {
        clear();
        mvprintw(0, 0, "All tests done. Press any key to exit.");
        refresh();
        nodelay(stdscr, FALSE);
        wgetch(stdscr);
    }

    ncurses_shutdown();
    if (scrn) free(scrn);
}
