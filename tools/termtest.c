/*
 * termtest.c — Standalone terminal rendering test
 *
 * Tests the sysop console output path (conio.c + platform_stubs.c)
 * without the full BBS.  Links against real .o files to test the
 * actual code paths.
 *
 * Build:  make termtest
 * Run:    build/termtest [path/to/file.bin]
 *
 * What it tests:
 *   1. Color grid — all 128 fg/bg combos via out1chx()
 *   2. clrscrb()  — cleared area should be black, not terminal default
 *   3. SCROLL_UP  — cleared/scrolled region should be black
 *   4. clrscr()   — full screen clear should be black
 *   5. .BIN file  — if a path is given, display it via out1ch()
 */

#define _DEFINE_GLOBALS_
#include "vars.h"
#include "cp437.h"

#include <termios.h>

/* ------------------------------------------------------------------ */
/*  Globals referenced by conio.o but not in vars.h                    */
/* ------------------------------------------------------------------ */

char menuat[15];

/* ------------------------------------------------------------------ */
/*  Stubs for conio.o dependencies we don't exercise                   */
/*                                                                     */
/*  This is the dependency map — every function here is pulled in by   */
/*  conio.o (mostly via skey, topscreen, tleft) but never called by    */
/*  the rendering functions we're testing (out1chx, SCROLL_UP, etc.)   */
/* ------------------------------------------------------------------ */

/* --- com.c (output/input) --- */
void ansic(int n)                { (void)n; }
void outchr(unsigned char c)     { out1ch(c); }  /* local-only for .BIN test */
void nl()                        { out1ch('\r'); out1ch('\n'); }
void pl(char *s)                 { (void)s; }
void logpr(char *fmt, ...)       { (void)fmt; }
void npr(char *fmt, ...)         { (void)fmt; }
int printfile(char *fn)          { (void)fn; return 0; }
void setbeep(int i)              { (void)i; }

/* --- x00com.c (serial/TCP) --- */
void dtr(int i)                  { (void)i; }
void dump()                      {}
void closeport()                 {}

/* --- time/utility --- */
void wait1(long l)               { (void)l; }
double nsl()                     { return 999.0; }
char *ctim(double d)             { (void)d; return "99:99"; }
int sysop2()                     { return 0; }
void bank2(int m)                { (void)m; }
void filter(char *s, unsigned char c) { (void)s; (void)c; }
int opp(int i)                   { return !i; }
int exist(char *s)               { (void)s; return 0; }

/* --- user/config --- */
void read_user(unsigned int un, userrec *u)  { (void)un; (void)u; }
void write_user(unsigned int un, userrec *u) { (void)un; (void)u; }
void val_cur_user(int i)         { (void)i; }
void set_autoval(int i)          { (void)i; }
void changedsl()                 {}
void reset_act_sl()              {}
char *nam(userrec *u1, unsigned int un) { (void)u1; (void)un; return "Test User"; }
char *ctype(int i)               { (void)i; return ""; }
int numwaiting(userrec *u)       { (void)u; return 0; }

/* --- menu/command --- */
int read_menu(char fn[15], int doauto) { (void)fn; (void)doauto; return 0; }
int runprog(char *s, int swp)    { (void)s; (void)swp; return 0; }
void pr_wait(int i)              { (void)i; }
void fastscreen(char *s)         { (void)s; }

/* --- misc --- */
void chat1(char *s, int i)       { (void)s; (void)i; }
void save_state(char *s, int i)  { (void)s; (void)i; }
void set_global_handle(int i)    { (void)i; }
void sl1(int i, char *s)         { (void)i; (void)s; }
void sysoplog(char *s)           { (void)s; }
void far *malloca(unsigned long nbytes) { return malloc(nbytes); }

/* ------------------------------------------------------------------ */
/*  Terminal setup / teardown                                          */
/* ------------------------------------------------------------------ */

static struct termios _orig_termios;
static int _raw_mode = 0;

static void enter_raw_mode(void)
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &_orig_termios);
    raw = _orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    _raw_mode = 1;
}

static void leave_raw_mode(void)
{
    if (_raw_mode) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &_orig_termios);
        _raw_mode = 0;
    }
}

static void cleanup(void)
{
    printf("\033[0m\033[?25h\033[?1049l");
    fflush(stdout);
    leave_raw_mode();
}

static void wait_key(void)
{
    unsigned char ch;
    while (read(STDIN_FILENO, &ch, 1) != 1)
        ;
}

static void banner(int y, char *msg)
{
    movecsr(0, y);
    textattr(0x0F);
    cprintf("%s", msg);
}

static void prompt(int y, char *msg)
{
    movecsr(0, y);
    textattr(0x07);
    cprintf("%s", msg);
}

/* ------------------------------------------------------------------ */
/*  Test cases                                                         */
/* ------------------------------------------------------------------ */

static void test_80x25_frame(void)
{
    int r, c;

    curatr = 0x07;
    clrscrb();

    /* Draw 80x25 double-line box using CP437 box chars */
    curatr = 0x0B;  /* bright cyan on black */
    movecsr(0, 0);
    out1chx(0xC9);  /* top-left ╔ */
    for (c = 1; c < 79; c++) out1chx(0xCD);  /* ═ */
    out1chx(0xBB);  /* top-right ╗ */

    for (r = 1; r < 24; r++) {
        movecsr(0, r);
        out1chx(0xBA);  /* left ║ */
        movecsr(79, r);
        out1chx(0xBA);  /* right ║ */
    }

    movecsr(0, 24);
    out1chx(0xC8);  /* bottom-left ╚ */
    for (c = 1; c < 79; c++) out1chx(0xCD);  /* ═ */
    /* Write bottom-right corner directly to avoid out1chx scroll trigger
     * (writing col 79 row 24 via out1chx advances cursor past end → scroll) */
    printf("%s", cp437_to_utf8[0xBC]);
    fflush(stdout);

    /* Column rulers at rows 1 and 23 */
    curatr = 0x08;  /* dark gray */
    for (c = 1; c < 79; c++) {
        if (c % 10 == 0) {
            movecsr(c, 1);
            out1chx('0' + (c / 10));
            movecsr(c, 23);
            out1chx('0' + (c / 10));
        } else if (c % 5 == 0) {
            movecsr(c, 1);
            out1chx('+');
            movecsr(c, 23);
            out1chx('+');
        } else {
            movecsr(c, 1);
            out1chx(0xB0);  /* ░ light shade */
            movecsr(c, 23);
            out1chx(0xB0);
        }
    }

    /* Row numbers down left side */
    for (r = 2; r < 23; r++) {
        movecsr(1, r);
        curatr = 0x08;
        cprintf("%2d", r);
    }

    /* Center label */
    curatr = 0x0F;
    movecsr(30, 11);
    cprintf("80 x 25 Reference Frame");
    curatr = 0x07;
    movecsr(24, 13);
    cprintf("Both corners of the box should be");
    movecsr(24, 14);
    cprintf("visible. If not, resize terminal.");

    movecsr(2, 22);
    textattr(0x07);
    cprintf("Press any key...");
    wait_key();
}

static void test_color_grid(void)
{
    int fg, bg;
    char *bg_names[] = {"Black","Blue","Green","Cyan","Red","Magenta","Brown","LtGray"};

    curatr = 0x07;
    clrscrb();
    banner(0, "=== Test 1: Color Grid (out1chx) ===");
    prompt(1, "Each row = one bg color.  16 fg chars per row.");
    prompt(2, "All cells should show correct bg — NO purple/default.");

    for (bg = 0; bg < 8; bg++) {
        movecsr(0, 4 + bg);
        textattr(0x07);
        cprintf("%-8s ", bg_names[bg]);
        for (fg = 0; fg < 16; fg++) {
            curatr = (bg << 4) | fg;
            out1chx('A' + fg);
            out1chx(' ');
        }
    }

    prompt(13, "Press any key...");
    wait_key();
}

static void test_clrscrb(void)
{
    curatr = 0x07;
    clrscrb();
    banner(0, "=== Test 2: clrscrb() ===");
    prompt(2, "The entire screen should be BLACK background.");
    prompt(3, "If you see purple/default terminal bg, clrscrb is broken.");
    prompt(5, "Press any key...");
    wait_key();
}

static void test_scroll_clear(void)
{
    int i;

    curatr = 0x07;
    clrscrb();

    for (i = 0; i <= 24; i++) {
        movecsr(0, i);
        textattr(0x1E);  /* yellow on blue */
        cprintf("Line %2d -- OUTSIDE clear zone            ", i);
    }

    SCROLL_UP(5, 20, 0);  /* clear lines 5-20 */

    /* Overwrite full lines to hide leftover blue fill text */
    movecsr(0, 0); textattr(0x0F);
    cprintf("%-80s", "=== Test 3: SCROLL_UP clear [5..20] ===");
    movecsr(0, 1); textattr(0x07);
    cprintf("%-80s", "Lines 5-20 should be BLACK.  0-4 and 21-24: yellow on blue.");
    prompt(23, "Press any key...");
    wait_key();
}

static void test_scroll_up(void)
{
    int i;

    curatr = 0x07;
    clrscrb();
    banner(0, "=== Test 4: SCROLL_UP scroll 5 lines ===");

    for (i = 2; i < 22; i++) {
        movecsr(0, i);
        textattr(0x02 + (i % 7));
        cprintf("Scroll line %2d (will shift up 5)", i);
    }
    prompt(23, "Press key to scroll...");
    wait_key();

    SCROLL_UP(2, 21, 5);

    prompt(23, "New lines at bottom of [2..21] should be BLACK. Press key...");
    wait_key();
}

static void test_clrscr(void)
{
    /* Fill screen with colored text first */
    int i;
    for (i = 0; i <= 24; i++) {
        movecsr(0, i);
        textattr(0x4F);  /* white on red */
        cprintf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    }
    prompt(12, "Press key to call clrscr()...");
    wait_key();

    clrscr();
    banner(0, "=== Test 5: clrscr() ===");
    prompt(2, "Screen should be entirely BLACK background.");
    prompt(3, "If you see purple/default bg, clrscr is broken.");
    prompt(5, "Press any key...");
    wait_key();
}

static void test_bin_file(char *path)
{
    int f, len, i;
    unsigned char *buf;

    curatr = 0x07;
    clrscrb();

    f = open(path, O_RDONLY);
    if (f < 0) {
        banner(0, "=== Test 6: .BIN file (FAILED TO OPEN) ===");
        prompt(2, path);
        prompt(3, "Press any key...");
        wait_key();
        return;
    }

    len = lseek(f, 0, SEEK_END);
    lseek(f, 0, SEEK_SET);
    buf = malloc(len);
    if (!buf) { close(f); return; }
    read(f, buf, len);
    close(f);

    /*
     * .BIN files are raw attribute/character pairs from TheDraw:
     * byte 0 = character, byte 1 = attribute, repeating.
     * 80 columns per row.
     */
    for (i = 0; i + 1 < len; i += 2) {
        curatr = buf[i + 1];
        out1ch(buf[i]);
    }

    free(buf);

    movecsr(0, 23);
    textattr(0x07);
    cprintf("[%s] Press any key...", path);
    wait_key();
}

/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */

void main(int argc, char *argv[])
{
    atexit(cleanup);

    /* Alt screen + black bg + clear */
    printf("\033[?1049h\033[0m\033[40m\033[2J\033[H");
    fflush(stdout);

    enter_raw_mode();

    /* Initialize minimum globals for conio.c / com.c */
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
    outcom = 0;       /* no remote output */
    incom = 0;
    using_modem = 0;
    hangup = 0;
    okskey = 0;       /* disable sysop key handler */
    chatting = 0;
    global_handle = 0;

    /* Run tests */
    test_80x25_frame();
    test_color_grid();
    test_clrscrb();
    test_scroll_clear();
    test_scroll_up();
    test_clrscr();

    /* If a .BIN file path was given, show it */
    if (argc > 1) {
        int i;
        for (i = 1; i < argc; i++)
            test_bin_file(argv[i]);
    }

    if (scrn) free(scrn);
}
