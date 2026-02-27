/*
 * vt100.cpp — Minimal ANSI terminal emulator for console mirroring
 *
 * Parses the ANSI subset that the BBS generates and maintains an 80x25
 * CGA screen buffer compatible with Terminal::renderBuffer().
 *
 * ANSI subset supported:
 *   ESC[...m    SGR colors (maps SGR→CGA)
 *   ESC[...H/f  Cursor position
 *   ESC[A/B/C/D Cursor movement
 *   ESC[2J      Clear screen
 *   ESC[K/0K/1K/2K  Clear line variants
 *   ESC[s/u     Save/restore cursor
 *   ESC[38;2;r;g;bm  True-color fg (consumed, CGA attr already set by standard SGR)
 *   ESC[48;2;r;g;bm  True-color bg (consumed, CGA attr already set by standard SGR)
 *   CR, LF, BS, TAB, FF, BEL — control characters
 *
 * UTF-8 input is reverse-mapped to CP437 using the cp437_to_utf8[] table.
 */

#include "vt100.h"
#include "cp437.h"
#include <cstring>
#include <cstdlib>

/* SGR color index → CGA color (same mapping as terminal.cpp / stream_processor.c) */
const char VT100::clrlst_[8] = { '0', '4', '2', '6', '1', '5', '3', '7' };

/* ================================================================== */
/*  UTF-8 → CP437 reverse mapping (built lazily)                      */
/* ================================================================== */

/* Hash table for multi-byte UTF-8 → CP437 reverse lookup.
 * Key: up to 4 UTF-8 bytes packed into a uint32_t.
 * Value: CP437 byte (0-255), 0 = empty slot. */
struct Utf8Entry {
    unsigned int key;
    unsigned char val;
};

#define UTF8_MAP_SIZE 512  /* power of 2, > 256 entries */
static Utf8Entry utf8Map_[UTF8_MAP_SIZE];
static bool utf8MapBuilt_ = false;

static unsigned int packUtf8(const unsigned char *s, int len)
{
    unsigned int k = 0;
    for (int i = 0; i < len && i < 4; i++)
        k = (k << 8) | s[i];
    return k;
}

static void buildUtf8Map()
{
    if (utf8MapBuilt_) return;
    std::memset(utf8Map_, 0, sizeof(utf8Map_));

    for (int i = 0; i < 256; i++) {
        const char *s = cp437_to_utf8[i];
        int len = (int)std::strlen(s);
        if (len <= 1) continue;  /* ASCII handled directly */

        unsigned int key = packUtf8((const unsigned char *)s, len);
        unsigned int idx = key & (UTF8_MAP_SIZE - 1);
        /* Linear probing */
        for (int j = 0; j < UTF8_MAP_SIZE; j++) {
            unsigned int slot = (idx + j) & (UTF8_MAP_SIZE - 1);
            if (utf8Map_[slot].key == 0) {
                utf8Map_[slot].key = key;
                utf8Map_[slot].val = (unsigned char)i;
                break;
            }
        }
    }
    utf8MapBuilt_ = true;
}

static unsigned char lookupUtf8(const unsigned char *buf, int len)
{
    unsigned int key = packUtf8(buf, len);
    unsigned int idx = key & (UTF8_MAP_SIZE - 1);
    for (int j = 0; j < UTF8_MAP_SIZE; j++) {
        unsigned int slot = (idx + j) & (UTF8_MAP_SIZE - 1);
        if (utf8Map_[slot].key == 0) return '?';
        if (utf8Map_[slot].key == key) return utf8Map_[slot].val;
    }
    return '?';
}


/* ================================================================== */
/*  Constructor / Reset                                                */
/* ================================================================== */

VT100::VT100()
{
    buildUtf8Map();
    reset();
}

void VT100::reset()
{
    std::memset(scrn_, 0, sizeof(scrn_));
    cx_ = cy_ = 0;
    attr_ = 0x07;  /* light grey on black */
    savedX_ = savedY_ = 0;
    state_ = Normal;
    csiLen_ = 0;
    utf8Len_ = 0;
    utf8Expected_ = 0;
}


/* ================================================================== */
/*  UTF-8 → CP437                                                     */
/* ================================================================== */

unsigned char VT100::utf8ToCp437(const unsigned char *buf, int len)
{
    return lookupUtf8(buf, len);
}


/* ================================================================== */
/*  Screen buffer operations                                           */
/* ================================================================== */

void VT100::clearLine(int row, int startCol, int endCol)
{
    if (row < 0 || row >= 25) return;
    if (startCol < 0) startCol = 0;
    if (endCol > 79) endCol = 79;
    for (int col = startCol; col <= endCol; col++) {
        int off = (row * 80 + col) * 2;
        scrn_[off] = ' ';
        scrn_[off + 1] = attr_;
    }
}

void VT100::clearScreen()
{
    for (int row = 0; row < 25; row++)
        clearLine(row, 0, 79);
    cx_ = cy_ = 0;
}

void VT100::scrollUp(int top, int bottom, int lines)
{
    if (lines <= 0) return;
    if (top < 0) top = 0;
    if (bottom > 24) bottom = 24;

    /* Shift rows up */
    for (int row = top; row <= bottom - lines; row++)
        std::memmove(&scrn_[row * 160], &scrn_[(row + lines) * 160], 160);

    /* Clear vacated rows at bottom */
    for (int row = bottom - lines + 1; row <= bottom; row++)
        clearLine(row, 0, 79);
}

void VT100::putChar(unsigned char ch)
{
    if (cx_ < 0) cx_ = 0;
    if (cy_ < 0) cy_ = 0;

    int off = (cy_ * 80 + cx_) * 2;
    if (off >= 0 && off < 4000) {
        scrn_[off] = ch;
        scrn_[off + 1] = attr_;
    }
    cx_++;
    if (cx_ >= 80) {
        cx_ = 0;
        if (cy_ >= 24)
            scrollUp(0, 24, 1);
        else
            cy_++;
    }
}


/* ================================================================== */
/*  SGR parsing                                                        */
/* ================================================================== */

void VT100::parseSGR(int *args, int nargs)
{
    for (int i = 0; i < nargs; i++) {
        /* Check for true-color sequences: 38;2;r;g;b or 48;2;r;g;b */
        if (args[i] == 38 && i + 1 < nargs && args[i + 1] == 2) {
            /* Skip: 38;2;R;G;B = consume 5 args total */
            i += 4;  /* skip 2,R,G,B (loop increments past 38) */
            if (i >= nargs) break;
            continue;
        }
        if (args[i] == 48 && i + 1 < nargs && args[i + 1] == 2) {
            i += 4;
            if (i >= nargs) break;
            continue;
        }

        switch (args[i]) {
        case 0: attr_ = 0x07; break;  /* reset */
        case 1: attr_ |= 0x08; break; /* bold/bright */
        case 4: break;                 /* underline — ignore */
        case 5: attr_ |= 0x80; break; /* blink */
        case 7: {                      /* reverse */
            int p = attr_ & 0x77;
            attr_ = (attr_ & 0x88) | (p << 4) | (p >> 4);
            break;
        }
        case 8: attr_ = 0x00; break;  /* concealed */
        default:
            if (args[i] >= 30 && args[i] <= 37) {
                /* Foreground: SGR 30-37 → CGA via clrlst */
                attr_ = (attr_ & 0xf8) | (clrlst_[args[i] - 30] - '0');
            }
            else if (args[i] >= 40 && args[i] <= 47) {
                /* Background: SGR 40-47 → CGA via clrlst */
                attr_ = (attr_ & 0x8f) | ((clrlst_[args[i] - 40] - '0') << 4);
            }
            break;
        }
    }
}


/* ================================================================== */
/*  CSI sequence execution                                             */
/* ================================================================== */

void VT100::executeCSI()
{
    if (csiLen_ <= 0) return;

    char cmd = csiBuf_[csiLen_ - 1];
    csiBuf_[csiLen_ - 1] = '\0';

    /* Parse semicolon-delimited args */
    int args[16];
    int nargs = 0;
    std::memset(args, 0, sizeof(args));

    if (csiBuf_[0]) {
        char *p = csiBuf_;
        while (*p && nargs < 16) {
            args[nargs++] = std::atoi(p);
            char *semi = std::strchr(p, ';');
            if (!semi) break;
            p = semi + 1;
        }
    }

    /* Default single arg to 1 for movement commands */
    if (cmd >= 'A' && cmd <= 'D' && (nargs == 0 || args[0] == 0))
        args[0] = 1;

    switch (cmd) {
    case 'H': case 'f':  /* Cursor position */
        cy_ = (nargs > 0 ? args[0] : 1) - 1;
        cx_ = (nargs > 1 ? args[1] : 1) - 1;
        if (cx_ < 0) cx_ = 0;
        if (cx_ > 79) cx_ = 79;
        if (cy_ < 0) cy_ = 0;
        if (cy_ > 24) cy_ = 24;
        break;

    case 'A':  /* Cursor up */
        cy_ -= args[0];
        if (cy_ < 0) cy_ = 0;
        break;

    case 'B':  /* Cursor down */
        cy_ += args[0];
        if (cy_ > 24) cy_ = 24;
        break;

    case 'C':  /* Cursor right */
        cx_ += args[0];
        if (cx_ > 79) cx_ = 79;
        break;

    case 'D':  /* Cursor left */
        cx_ -= args[0];
        if (cx_ < 0) cx_ = 0;
        break;

    case 'J':  /* Erase in Display */
        if (args[0] == 2)
            clearScreen();
        else if (args[0] == 0) {
            /* Clear from cursor to end of screen */
            clearLine(cy_, cx_, 79);
            for (int row = cy_ + 1; row < 25; row++)
                clearLine(row, 0, 79);
        }
        else if (args[0] == 1) {
            /* Clear from start to cursor */
            for (int row = 0; row < cy_; row++)
                clearLine(row, 0, 79);
            clearLine(cy_, 0, cx_);
        }
        break;

    case 'K': case 'k':  /* Erase in Line */
        if (nargs == 0 || args[0] == 0)
            clearLine(cy_, cx_, 79);      /* clear to EOL */
        else if (args[0] == 1)
            clearLine(cy_, 0, cx_);        /* clear to start */
        else if (args[0] == 2)
            clearLine(cy_, 0, 79);         /* clear entire line */
        break;

    case 'm':  /* SGR — Select Graphic Rendition */
        if (nargs == 0) {
            nargs = 1;
            args[0] = 0;
        }
        parseSGR(args, nargs);
        break;

    case 's':  /* Save cursor */
        savedX_ = cx_;
        savedY_ = cy_;
        break;

    case 'u':  /* Restore cursor */
        cx_ = savedX_;
        cy_ = savedY_;
        break;
    }
}


/* ================================================================== */
/*  Main feed function                                                 */
/* ================================================================== */

bool VT100::feed(unsigned char byte)
{
    /* UTF-8 multi-byte accumulation */
    if (utf8Expected_ > 0) {
        if ((byte & 0xC0) == 0x80) {
            /* Continuation byte */
            utf8Buf_[utf8Len_++] = byte;
            utf8Expected_--;
            if (utf8Expected_ == 0) {
                unsigned char cp = utf8ToCp437(utf8Buf_, utf8Len_);
                utf8Len_ = 0;
                if (state_ == Normal) {
                    putChar(cp);
                    return true;
                }
            }
            return false;
        } else {
            /* Invalid continuation — reset and fall through to process this byte */
            utf8Len_ = 0;
            utf8Expected_ = 0;
        }
    }

    switch (state_) {
    case Normal:
        if (byte == 0x1B) {
            state_ = GotEsc;
            return false;
        }
        /* Check for UTF-8 multi-byte start */
        if (byte >= 0xC0 && byte <= 0xDF) {
            utf8Buf_[0] = byte;
            utf8Len_ = 1;
            utf8Expected_ = 1;
            return false;
        }
        if (byte >= 0xE0 && byte <= 0xEF) {
            utf8Buf_[0] = byte;
            utf8Len_ = 1;
            utf8Expected_ = 2;
            return false;
        }
        if (byte >= 0xF0 && byte <= 0xF7) {
            utf8Buf_[0] = byte;
            utf8Len_ = 1;
            utf8Expected_ = 3;
            return false;
        }
        /* Control characters */
        switch (byte) {
        case 0x0D:  /* CR */
            cx_ = 0;
            return true;
        case 0x0A:  /* LF */
            if (cy_ >= 24)
                scrollUp(0, 24, 1);
            else
                cy_++;
            return true;
        case 0x08:  /* BS */
            if (cx_ > 0) cx_--;
            return true;
        case 0x09:  /* TAB */
            cx_ = ((cx_ / 8) + 1) * 8;
            if (cx_ > 79) cx_ = 79;
            return true;
        case 0x0C:  /* FF — clear screen */
            clearScreen();
            return true;
        case 0x07:  /* BEL — ignore */
            return false;
        case 0x00:  /* NUL — ignore */
            return false;
        }
        /* Printable (ASCII 0x20-0x7E, or high bytes for direct CP437) */
        if (byte >= 0x20) {
            putChar(byte);
            return true;
        }
        return false;

    case GotEsc:
        if (byte == '[') {
            state_ = InCSI;
            csiLen_ = 0;
            return false;
        }
        /* ESC followed by something else — ignore the sequence */
        state_ = Normal;
        return false;

    case InCSI:
        /* Accumulate CSI parameters and intermediate bytes */
        if (csiLen_ < 78) {
            csiBuf_[csiLen_++] = byte;
            csiBuf_[csiLen_] = '\0';
        }
        /* Check if byte is a final character (letter) */
        if ((byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z')) {
            executeCSI();
            state_ = Normal;
            return true;
        }
        /* Still accumulating params (digits, semicolons, ?) */
        if ((byte >= '0' && byte <= '9') || byte == ';' || byte == '?') {
            return false;
        }
        /* Unknown intermediate byte — keep accumulating */
        return false;
    }

    return false;
}

bool VT100::feedBlock(const unsigned char *data, int len)
{
    bool changed = false;
    for (int i = 0; i < len; i++) {
        if (feed(data[i]))
            changed = true;
    }
    return changed;
}
