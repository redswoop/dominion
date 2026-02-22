/*
 * terminal.h — Clean terminal I/O layer
 *
 * LEAF COMPONENT: depends on ncurses, TCP sockets, and nothing else.
 * No BBS headers, no vardec.h, no vars.h, no globals.
 *
 * Provides dual-stream output (local ncurses + remote TCP), ANSI color
 * generation/parsing, CP437→UTF-8 conversion, screen buffer management,
 * and keyboard/telnet input. Everything a BBS (or any terminal app)
 * needs for character-mode I/O, without knowing anything about BBS
 * data structures, user records, or configuration.
 */

#ifndef TERMINAL_H_
#define TERMINAL_H_

#include <termios.h>

class Terminal {
public:
    Terminal();
    ~Terminal();

    /* -- Lifecycle -- */
    bool initLocal();               /* ncurses setup; false if no tty */
    void shutdown();

    /* -- Remote TCP stream -- */
    void setRemote(int fd);
    void closeRemote();
    bool remoteConnected() const;
    void sendTelnetNegotiation();
    void sendTerminalInit();        /* alt screen + black bg */
    void sendTerminalRestore();     /* restore primary screen */

    /* -- Output (writes to all active streams) -- */
    void putch(unsigned char c);
    void puts(const char *s);
    void newline();                 /* \r\n */
    void printf(const char *fmt, ...) __attribute__((format(printf, 2, 3)));

    /* -- Color -- */
    void setAttr(unsigned char attr);
    unsigned char attr() const { return curatr_; }
    int makeAnsi(unsigned char attr, char *buf);

    /* -- Input -- */
    bool keyReady();                /* any input from any source? */
    unsigned char getKey();         /* blocking */
    unsigned char getKeyNB();       /* non-blocking, 0 if nothing */

    /* -- Screen -- */
    void clearScreen();
    void moveCursor(int x, int y);
    int cursorX() const { return cx_; }
    int cursorY() const { return cy_ - topLine_; }
    int screenBottom() const { return screenBottom_; }
    int topLine() const { return topLine_; }
    bool localActive() const { return ncActive_; }

    /* -- Helpers for input UI -- */
    void backspace();               /* BS, space, BS */

private:
    /* -- Remote I/O -- */
    void remotePutch(unsigned char c);   /* single char, CP437→UTF-8 */
    void remoteWriteRaw(const char *s);  /* raw bytes to TCP */
    bool remoteDataReady();
    unsigned char remoteGetKey();         /* with IAC filtering */
    bool telnetFilter(unsigned char *ch);

    /* -- Local screen primitives -- */
    void out1chx(unsigned char ch);      /* char + attr to screen buf + ncurses */
    void out1ch(unsigned char ch);       /* dispatch CR/LF/BS/FF/printable */
    void scrollUp(int top, int bottom, int lines);
    void cr();
    void lf();
    void bs();

    /* -- Screen buffer -- */
    void scrnPut(int x, int y, unsigned char ch, unsigned char attr);
    void scrnScroll(int top, int bottom, int n);
    void renderScrn(int startRow, int numRows);

    /* -- ANSI -- */
    void executeAnsi();
    void addAnsiParam(char *s, int val);

    /* -- ncurses -- */
    int ncAttr(int dosAttr);
    void emitAttr(int attr);

    /* -- Keyboard -- */
    int ncToScancode(int key);
    bool localKeyReady();
    unsigned char localGetKey();         /* blocking */
    unsigned char localGetKeyNB();       /* non-blocking, 255 if nothing */

    /* -- Stream state -- */
    struct Stream {
        int fd = -1;
        bool active = false;
        bool needs_iac = false;
    };
    Stream remote_;

    /* -- Screen state -- */
    char *scrn_ = nullptr;
    int topLine_ = 0;
    int screenBottom_ = 24;
    int cx_ = 0, cy_ = 0;
    int lastAttr_ = -1;
    unsigned char curatr_ = 0x07;

    /* -- ANSI parser -- */
    char ansiStr_[81] = {};
    int ansiPtr_ = 0;
    int savedX_ = 0, savedY_ = 0;

    /* -- Telnet -- */
    int iacState_ = 0;

    /* -- Keyboard -- */
    int pendingScancode_ = -1;

    /* -- Terminal -- */
    bool ncActive_ = false;
    struct termios origTermios_ = {};
};

#endif /* TERMINAL_H_ */
