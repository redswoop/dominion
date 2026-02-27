/*
 * vt100.h — Minimal ANSI terminal emulator for console mirroring
 *
 * LEAF COMPONENT: depends only on cp437.h. No BBS headers.
 *
 * Maintains an 80x25x2 CGA screen buffer (same format as Terminal::scrn_)
 * that can be rendered via Terminal::renderBuffer(). Parses the ANSI subset
 * that the BBS actually generates (SGR colors, cursor positioning, clear ops).
 *
 * Usage:
 *   VT100 vt;
 *   vt.feed(byte);           // parse one byte
 *   term.renderBuffer(vt.buffer(), 0, 25);  // mirror to ncurses
 */

#ifndef VT100_H_
#define VT100_H_

class VT100 {
public:
    VT100();

    /* Feed one byte of output from the BBS child process.
     * Returns true if the screen buffer changed (caller should refresh). */
    bool feed(unsigned char byte);

    /* Feed a block of bytes. Returns true if any changed the buffer. */
    bool feedBlock(const unsigned char *data, int len);

    /* Access the 80x25x2 CGA screen buffer (char+attr pairs, 4000 bytes).
     * Compatible with Terminal::renderBuffer(). */
    const char *buffer() const { return scrn_; }

    /* Cursor position (0-based) */
    int cursorX() const { return cx_; }
    int cursorY() const { return cy_; }

    /* Reset to power-on state */
    void reset();

private:
    /* Screen buffer: 80 cols x 25 rows x 2 bytes (char + CGA attr) */
    char scrn_[4000];

    /* Cursor */
    int cx_, cy_;

    /* Current CGA color attribute */
    unsigned char attr_;

    /* Saved cursor (ESC[s / ESC[u) */
    int savedX_, savedY_;

    /* ANSI CSI parser state */
    enum ParseState { Normal, GotEsc, InCSI };
    ParseState state_;
    char csiBuf_[80];
    int csiLen_;

    /* UTF-8 accumulator for reverse mapping */
    unsigned char utf8Buf_[4];
    int utf8Len_;
    int utf8Expected_;

    /* Internal operations */
    void putChar(unsigned char ch);
    void executeCSI();
    void parseSGR(int *args, int nargs);
    void clearLine(int row, int startCol, int endCol);
    void clearScreen();
    void scrollUp(int top, int bottom, int lines);
    unsigned char utf8ToCp437(const unsigned char *buf, int len);

    /* SGR color index mapping (same as terminal.cpp clrlst) */
    static const char clrlst_[8];
};

#endif /* VT100_H_ */
