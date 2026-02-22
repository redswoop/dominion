/*
 * terminal.cpp — Clean terminal I/O implementation
 *
 * Extracted and reorganized from com.c, tcpio.c, conio.c, io_ncurses.c.
 * Zero BBS dependencies. This file includes only system headers and cp437.h.
 */

#include "terminal.h"
#include "cp437.h"

#include <ncurses.h>
#undef getch    /* ncurses macro conflicts */
#undef echo     /* ncurses macro conflicts */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cctype>
#include <csignal>
#include <unistd.h>
#include <locale.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <errno.h>

/* ================================================================== */
/*  CP437 color index → ncurses color constant                        */
/*  DOS:     0=black 1=blue 2=green 3=cyan 4=red 5=magenta 6=brown 7=white */
/*  ncurses: 0=black 1=red  2=green 3=yellow 4=blue 5=magenta 6=cyan 7=white */
/* ================================================================== */

static const int dos2nc[8] = {
    COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
    COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE
};


/* ================================================================== */
/*  Crash handler — restore terminal before dying                     */
/* ================================================================== */

static Terminal *g_crash_term = nullptr;

static void crash_handler(int sig)
{
    if (g_crash_term)
        g_crash_term->shutdown();
    signal(sig, SIG_DFL);
    raise(sig);
}


/* ================================================================== */
/*  Constructor / Destructor                                          */
/* ================================================================== */

Terminal::Terminal()
{
    scrn_ = (char *)std::calloc(4000, 1);  /* 80×25×2 */
}

Terminal::~Terminal()
{
    shutdown();
    std::free(scrn_);
    scrn_ = nullptr;
}


/* ================================================================== */
/*  Lifecycle                                                          */
/* ================================================================== */

bool Terminal::initLocal()
{
    if (!isatty(STDOUT_FILENO)) {
        ncActive_ = false;
        if (isatty(STDIN_FILENO)) {
            tcgetattr(STDIN_FILENO, &origTermios_);
            struct termios raw = origTermios_;
            raw.c_lflag &= ~(ICANON | ECHO | ISIG);
            raw.c_iflag &= ~(IXON | ICRNL);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        }
        return false;
    }

    setlocale(LC_ALL, "");
    initscr();
    ncActive_ = true;

    if (has_colors()) {
        start_color();
        for (int bg = 0; bg < 8; bg++)
            for (int fg = 0; fg < 8; fg++)
                init_pair(bg * 8 + fg + 1, dos2nc[fg], dos2nc[bg]);
    }

    raw();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    scrollok(stdscr, FALSE);
    idlok(stdscr, TRUE);
    erase();
    refresh();
    resizeterm(25, 80);

    g_crash_term = this;
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGBUS,  crash_handler);
    signal(SIGFPE,  crash_handler);
    signal(SIGTERM, crash_handler);
    signal(SIGINT,  crash_handler);

    return true;
}

void Terminal::shutdown()
{
    if (ncActive_) {
        endwin();
        ncActive_ = false;
    } else if (isatty(STDIN_FILENO)) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios_);
    }
    if (g_crash_term == this)
        g_crash_term = nullptr;
}


/* ================================================================== */
/*  Remote TCP stream                                                  */
/* ================================================================== */

void Terminal::setRemote(int fd)
{
    remote_.fd = fd;
    remote_.active = true;
    remote_.needs_iac = true;
    iacState_ = 0;
}

void Terminal::closeRemote()
{
    if (remote_.fd >= 0) {
        close(remote_.fd);
        remote_.fd = -1;
    }
    remote_.active = false;
}

bool Terminal::remoteConnected() const
{
    if (remote_.fd < 0) return false;
    char buf;
    int n = recv(remote_.fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
    if (n == 0) return false;
    if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) return false;
    return true;
}

/* Telnet protocol */
#define IAC   255
#define WILL  251
#define WONT  252
#define DO    253
#define DONT  254
#define IAC_SB  250
#define IAC_SE  240
#define TELOPT_ECHO   1
#define TELOPT_SGA    3
#define TELOPT_NAWS   31

void Terminal::sendTelnetNegotiation()
{
    unsigned char neg[] = {
        IAC, WILL, TELOPT_ECHO,
        IAC, WILL, TELOPT_SGA,
        IAC, DO,   TELOPT_NAWS,
    };
    write(remote_.fd, neg, sizeof(neg));
    iacState_ = 0;
}

void Terminal::sendTerminalInit()
{
    const char *seq = "\033[?1049h\033[0m\033[40m\033[2J\033[H";
    write(remote_.fd, seq, std::strlen(seq));
}

void Terminal::sendTerminalRestore()
{
    const char *seq = "\033[0m\033[?1049l";
    write(remote_.fd, seq, std::strlen(seq));
}

void Terminal::remotePutch(unsigned char c)
{
    if (remote_.fd < 0) return;
    if (c >= 0x80) {
        const char *utf8 = cp437_to_utf8[c];
        write(remote_.fd, utf8, std::strlen(utf8));
    } else {
        write(remote_.fd, &c, 1);
    }
}

void Terminal::remoteWriteRaw(const char *s)
{
    if (remote_.fd < 0) return;
    write(remote_.fd, s, std::strlen(s));
}

bool Terminal::remoteDataReady()
{
    if (remote_.fd < 0) return false;
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(remote_.fd, &fds);
    return (select(remote_.fd + 1, &fds, NULL, NULL, &tv) > 0);
}

unsigned char Terminal::remoteGetKey()
{
    unsigned char ch;
    struct timeval tv = {0, 0};
    fd_set fds;

    if (remote_.fd < 0) return 0;

    FD_ZERO(&fds);
    FD_SET(remote_.fd, &fds);
    if (select(remote_.fd + 1, &fds, NULL, NULL, &tv) <= 0)
        return 0;

    while (1) {
        int n = read(remote_.fd, &ch, 1);
        if (n <= 0) {
            if (n == 0) closeRemote();
            return 0;
        }
        if (telnetFilter(&ch))
            return ch;
        FD_ZERO(&fds);
        FD_SET(remote_.fd, &fds);
        if (select(remote_.fd + 1, &fds, NULL, NULL, &tv) <= 0)
            return 0;
    }
}

bool Terminal::telnetFilter(unsigned char *ch)
{
    unsigned char c = *ch;
    switch (iacState_) {
    case 0:
        if (c == IAC) { iacState_ = 1; return false; }
        return true;
    case 1:
        if (c == IAC) { iacState_ = 0; *ch = 0xFF; return true; }
        if (c == WILL || c == WONT || c == DO || c == DONT) { iacState_ = 2; return false; }
        if (c == IAC_SB) { iacState_ = 3; return false; }
        iacState_ = 0; return false;
    case 2:
        iacState_ = 0; return false;
    case 3:
        if (c == IAC) iacState_ = 4;
        return false;
    case 4:
        iacState_ = (c == IAC_SE) ? 0 : 3;
        return false;
    }
    iacState_ = 0;
    return false;
}


/* ================================================================== */
/*  ncurses helpers                                                    */
/* ================================================================== */

int Terminal::ncAttr(int dosAttr)
{
    int fg = dosAttr & 7;
    int bg = (dosAttr >> 4) & 7;
    int pair = bg * 8 + fg + 1;
    int bold = (dosAttr & 0x08) ? A_BOLD : 0;
    int blink = (dosAttr & 0x80) ? A_BLINK : 0;
    return COLOR_PAIR(pair) | bold | blink;
}

void Terminal::emitAttr(int attr)
{
    if (attr == lastAttr_) return;
    if (ncActive_) attrset(ncAttr(attr));
    lastAttr_ = attr;
}


/* ================================================================== */
/*  Screen buffer                                                      */
/* ================================================================== */

void Terminal::scrnPut(int x, int y, unsigned char ch, unsigned char attr)
{
    if (!scrn_) return;
    if (x < 0 || x >= 80 || y < 0 || y > screenBottom_) return;
    int off = (y * 80 + x) * 2;
    if (off >= 0 && off < 4000) {
        scrn_[off] = ch;
        scrn_[off + 1] = attr;
    }
}

void Terminal::scrnScroll(int t, int b, int n)
{
    if (!scrn_) return;
    if (n <= 0) {
        for (int row = t; row <= b; row++)
            std::memset(&scrn_[row * 160], 0, 160);
        return;
    }
    for (int row = t; row <= b - n; row++)
        std::memmove(&scrn_[row * 160], &scrn_[(row + n) * 160], 160);
    for (int row = b - n + 1; row <= b; row++)
        std::memset(&scrn_[row * 160], 0, 160);
}

void Terminal::renderScrn(int startRow, int numRows)
{
    if (!scrn_ || !ncActive_) return;
    for (int row = startRow; row < startRow + numRows && row < 25; row++) {
        move(row, 0);
        for (int col = 0; col < 80; col++) {
            int off = (row * 80 + col) * 2;
            unsigned char ch = (unsigned char)scrn_[off];
            unsigned char at = (unsigned char)scrn_[off + 1];
            attrset(ncAttr(at));
            addstr(cp437_to_utf8[ch ? ch : ' ']);
        }
    }
    refresh();
}


/* ================================================================== */
/*  Screen primitives                                                  */
/* ================================================================== */

void Terminal::scrollUp(int t, int b, int l)
{
    if (l == 0) {
        if (ncActive_) {
            for (int row = t; row <= b; row++) {
                move(row, 0);
                clrtoeol();
            }
        }
        scrnScroll(t, b, 0);
    } else {
        if (ncActive_) {
            setscrreg(t, b);
            scrollok(stdscr, TRUE);
            wscrl(stdscr, l);
            scrollok(stdscr, FALSE);
            setscrreg(0, screenBottom_);
        }
        scrnScroll(t, b, l);
    }
    lastAttr_ = -1;
    if (ncActive_) refresh();
}

void Terminal::moveCursor(int x, int y)
{
    if (x < 0) x = 0;
    if (x > 79) x = 79;
    if (y < 0) y = 0;
    y += topLine_;
    if (y > screenBottom_) y = screenBottom_;
    cx_ = x;
    cy_ = y;
    if (ncActive_) { move(y, x); refresh(); }
}

void Terminal::cr()
{
    cx_ = 0;
    if (ncActive_) { move(cy_, 0); refresh(); }
}

void Terminal::lf()
{
    if (cy_ == screenBottom_) {
        scrollUp(topLine_, screenBottom_, 1);
        if (ncActive_) { move(cy_, cx_); refresh(); }
    } else {
        cy_++;
        if (ncActive_) { move(cy_, cx_); refresh(); }
    }
}

void Terminal::bs()
{
    if (cx_ == 0) {
        if (cy_ != topLine_) {
            cx_ = 79;
            cy_--;
            if (ncActive_) { move(cy_, cx_); refresh(); }
        }
    } else {
        cx_--;
        if (ncActive_) { move(cy_, cx_); refresh(); }
    }
}

void Terminal::clearScreen()
{
    lastAttr_ = -1;
    scrollUp(topLine_, screenBottom_, 0);
    moveCursor(0, 0);
}

void Terminal::out1chx(unsigned char ch)
{
    emitAttr(curatr_);
    scrnPut(cx_, cy_, ch, curatr_);
    if (ncActive_) {
        move(cy_, cx_);
        addstr(cp437_to_utf8[ch]);
    }
    cx_++;
    if (cx_ >= 80) {
        cx_ = 0;
        if (cy_ == screenBottom_)
            scrollUp(topLine_, screenBottom_, 1);
        else
            cy_++;
        if (ncActive_) move(cy_, cx_);
    }
    if (ncActive_) refresh();
}

void Terminal::out1ch(unsigned char ch)
{
    if (ch > 31)
        out1chx(ch);
    else if (ch == 13)
        cr();
    else if (ch == 10)
        lf();
    else if (ch == 12)
        clearScreen();
    else if (ch == 8)
        bs();
    else if (ch == 7) {
        if (ncActive_) { beep(); }
    }
}


/* ================================================================== */
/*  ANSI generation                                                    */
/* ================================================================== */

void Terminal::addAnsiParam(char *s, int val)
{
    char tmp[20];
    if (s[0])
        std::strcat(s, ";");
    else
        std::strcpy(s, "\x1B[");
    std::snprintf(tmp, sizeof(tmp), "%d", val);
    std::strcat(s, tmp);
}

int Terminal::makeAnsi(unsigned char attr, char *s)
{
    static const char *temp = "04261537";
    unsigned char catr = curatr_;
    s[0] = 0;

    if (attr != catr) {
        if ((catr & 0x88) ^ (attr & 0x88)) {
            addAnsiParam(s, 0);
            addAnsiParam(s, 30 + temp[attr & 0x07] - '0');
            addAnsiParam(s, 40 + temp[(attr & 0x70) >> 4] - '0');
            catr = attr & 0x77;
        }
        if ((catr & 0x07) != (attr & 0x07))
            addAnsiParam(s, 30 + temp[attr & 0x07] - '0');
        /* Always include explicit background when emitting color changes */
        if (s[0] || (catr & 0x70) != (attr & 0x70))
            addAnsiParam(s, 40 + temp[(attr & 0x70) >> 4] - '0');
        if ((catr & 0x08) ^ (attr & 0x08))
            addAnsiParam(s, 1);
        if ((catr & 0x80) ^ (attr & 0x80))
            addAnsiParam(s, 5);
    }
    if (s[0])
        std::strcat(s, "m");
    return (int)std::strlen(s);
}


/* ================================================================== */
/*  ANSI parsing (for incoming ANSI art / data)                       */
/* ================================================================== */

void Terminal::executeAnsi()
{
    int args[11], argptr, count, ptr, tempptr, ox, oy;
    char cmd, tmp[11];
    static const char *clrlst = "04261537";

    if (ansiStr_[1] != '[') {
        /* Not a CSI sequence — ignore */
    } else {
        argptr = tempptr = 0;
        ptr = 2;
        for (count = 0; count < 10; count++)
            args[count] = tmp[count] = 0;
        cmd = ansiStr_[ansiPtr_ - 1];
        ansiStr_[ansiPtr_ - 1] = 0;

        while (ansiStr_[ptr] && argptr < 10 && tempptr < 10) {
            if (ansiStr_[ptr] == ';') {
                tmp[tempptr] = 0;
                tempptr = 0;
                args[argptr++] = std::atoi(tmp);
            } else {
                tmp[tempptr++] = ansiStr_[ptr];
            }
            ++ptr;
        }
        if (tempptr && argptr < 10) {
            tmp[tempptr] = 0;
            args[argptr++] = std::atoi(tmp);
        }
        if (cmd >= 'A' && cmd <= 'D' && !args[0])
            args[0] = 1;

        switch (cmd) {
        case 'f':
        case 'H':
            moveCursor(args[1] - 1, args[0] - 1);
            break;
        case 'A':
            moveCursor(cursorX(), cursorY() - args[0]);
            break;
        case 'B':
            moveCursor(cursorX(), cursorY() + args[0]);
            break;
        case 'C':
            moveCursor(cursorX() + args[0], cursorY());
            break;
        case 'D':
            moveCursor(cursorX() - args[0], cursorY());
            break;
        case 's':
            savedX_ = cursorX();
            savedY_ = cursorY();
            break;
        case 'u':
            moveCursor(savedX_, savedY_);
            break;
        case 'J':
            if (args[0] == 2)
                clearScreen();
            break;
        case 'k':
        case 'K':
            ox = cursorX();
            oy = cursorY();
            if (ncActive_) { clrtoeol(); refresh(); }
            if (scrn_) {
                int row = oy + topLine_;
                for (int col = ox; col < 80; col++) {
                    int off = (row * 80 + col) * 2;
                    if (off >= 0 && off < 4000) {
                        scrn_[off] = ' ';
                        scrn_[off + 1] = curatr_;
                    }
                }
            }
            moveCursor(ox, oy);
            break;
        case 'm':
            if (!argptr) { argptr = 1; args[0] = 0; }
            for (count = 0; count < argptr; count++) {
                switch (args[count]) {
                case 0: curatr_ = 0x07; break;
                case 1: curatr_ |= 0x08; break;
                case 4: break;
                case 5: curatr_ |= 0x80; break;
                case 7: {
                    int p = curatr_ & 0x77;
                    curatr_ = (curatr_ & 0x88) | (p << 4) | (p >> 4);
                    break;
                }
                case 8: curatr_ = 0; break;
                default:
                    if (args[count] >= 30 && args[count] <= 37)
                        curatr_ = (curatr_ & 0xf8) | (clrlst[args[count] - 30] - '0');
                    else if (args[count] >= 40 && args[count] <= 47)
                        curatr_ = (curatr_ & 0x8f) | ((clrlst[args[count] - 40] - '0') << 4);
                }
            }
            break;
        }
    }
    ansiPtr_ = 0;
}


/* ================================================================== */
/*  Color                                                              */
/* ================================================================== */

void Terminal::setAttr(unsigned char attr)
{
    if (attr == curatr_) return;

    char buf[30];
    makeAnsi(attr, buf);

    if (buf[0]) {
        /* Send ANSI sequence to remote */
        if (remote_.active)
            remoteWriteRaw(buf);

        /* Update local state directly (no need to re-parse) */
        curatr_ = attr;
        emitAttr(attr);
    }
}


/* ================================================================== */
/*  Output                                                             */
/* ================================================================== */

void Terminal::putch(unsigned char c)
{
    /* Remote output (skip tabs — they expand locally) */
    if (remote_.active && c != 9)
        remotePutch(c);

    /* ANSI escape accumulation */
    if (ansiPtr_ > 0) {
        ansiStr_[ansiPtr_++] = c;
        ansiStr_[ansiPtr_] = 0;
        if (((c < '0' || c > '9') && c != '[' && c != ';') ||
            ansiStr_[1] != '[' || ansiPtr_ > 75)
            executeAnsi();
    }
    else if (c == 27) {
        ansiStr_[0] = 27;
        ansiPtr_ = 1;
        ansiStr_[ansiPtr_] = 0;
    }
    else if (c == 9) {
        /* Tab expansion */
        int x = cursorX();
        int stop = ((x / 8) + 1) * 8;
        for (int i = x; i < stop; i++)
            putch(' ');
    }
    else {
        out1ch(c);
        /* After form feed, reset remote terminal attributes */
        if (c == 12 && remote_.active)
            remoteWriteRaw("\x1b[0;1m");
    }
}

void Terminal::puts(const char *s)
{
    while (*s)
        putch((unsigned char)*s++);
}

void Terminal::newline()
{
    putch('\r');
    putch('\n');
}

void Terminal::printf(const char *fmt, ...)
{
    va_list ap;
    char buf[512];
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    puts(buf);
}

void Terminal::backspace()
{
    puts("\b \b");
}


/* ================================================================== */
/*  Keyboard input                                                     */
/* ================================================================== */

int Terminal::ncToScancode(int key)
{
    switch (key) {
    case KEY_F(1):  return 59;
    case KEY_F(2):  return 60;
    case KEY_F(3):  return 61;
    case KEY_F(4):  return 62;
    case KEY_F(5):  return 63;
    case KEY_F(6):  return 64;
    case KEY_F(7):  return 65;
    case KEY_F(8):  return 66;
    case KEY_F(9):  return 67;
    case KEY_F(10): return 68;
    case KEY_UP:    return 72;
    case KEY_DOWN:  return 80;
    case KEY_LEFT:  return 75;
    case KEY_RIGHT: return 77;
    case KEY_HOME:  return 71;
    case KEY_END:   return 79;
    default:        return 0;
    }
}

bool Terminal::localKeyReady()
{
    if (pendingScancode_ >= 0) return true;
    if (!ncActive_) return false;
    nodelay(stdscr, TRUE);
    int ch = wgetch(stdscr);
    if (ch == ERR) return false;
    ungetch(ch);
    return true;
}

unsigned char Terminal::localGetKey()
{
    if (pendingScancode_ >= 0) {
        int sc = pendingScancode_;
        pendingScancode_ = -1;
        return (unsigned char)sc;
    }
    if (!ncActive_) { usleep(100000); return 0; }
    nodelay(stdscr, FALSE);
    int ch;
    while ((ch = wgetch(stdscr)) == ERR)
        usleep(1000);
    nodelay(stdscr, TRUE);

    if (ch == KEY_ENTER || ch == '\n') return '\r';
    if (ch == KEY_BACKSPACE || ch == 127) return 8;
    if (ch >= KEY_MIN) {
        pendingScancode_ = ncToScancode(ch);
        return 0;
    }
    return (unsigned char)ch;
}

unsigned char Terminal::localGetKeyNB()
{
    if (pendingScancode_ >= 0) {
        int sc = pendingScancode_;
        pendingScancode_ = -1;
        return (unsigned char)sc;
    }
    if (!ncActive_) return 255;
    nodelay(stdscr, TRUE);
    int ch = wgetch(stdscr);
    if (ch == ERR) return 255;
    if (ch == KEY_ENTER || ch == '\n') return '\r';
    if (ch == KEY_BACKSPACE || ch == 127) return 8;
    if (ch >= KEY_MIN) {
        pendingScancode_ = ncToScancode(ch);
        return 0;
    }
    return (unsigned char)ch;
}


/* ================================================================== */
/*  Public input API                                                   */
/* ================================================================== */

bool Terminal::keyReady()
{
    return localKeyReady() || (remote_.active && remoteDataReady());
}

unsigned char Terminal::getKeyNB()
{
    /* Local keyboard first */
    if (localKeyReady()) {
        unsigned char ch = localGetKeyNB();
        if (ch != 255) return ch;
    }
    /* Remote TCP */
    if (remote_.active && remoteDataReady())
        return remoteGetKey();
    return 0;
}

unsigned char Terminal::getKey()
{
    unsigned char ch;
    do {
        while (!keyReady())
            usleep(10000);  /* 10ms poll */
        ch = getKeyNB();
    } while (!ch);
    if (ch == 127) ch = 8;  /* DEL → BS */
    return ch;
}
