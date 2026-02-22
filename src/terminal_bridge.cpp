/*
 * terminal_bridge.cpp — C-callable bridge to the Terminal class
 *
 * Owns the static Terminal instance (g_term).  All functions are
 * extern "C" wrappers that delegate to g_term methods.
 *
 * State sync: Terminal maintains its own topLine_, screenBottom_,
 * curatr_, cx_, cy_.  The BBS has parallel state in io_session_t
 * (via macros: topline, screenbottom, curatr).  During the transition,
 * the bridge functions that modify these states sync both directions.
 * Callers in conio.c/com.c push BBS state before calling, and pull
 * Terminal state after.  Eventually the BBS macros will read directly
 * from Terminal, eliminating the sync.
 */

#include "terminal.h"
#include "terminal_bridge.h"

static Terminal g_term;


/* ================================================================== */
/*  Lifecycle                                                          */
/* ================================================================== */

int term_init_local(void)
{
    return g_term.initLocal() ? 1 : 0;
}

void term_shutdown(void)
{
    g_term.shutdown();
}


/* ================================================================== */
/*  Remote TCP stream                                                  */
/* ================================================================== */

void term_set_remote(int fd)        { g_term.setRemote(fd); }
void term_close_remote(void)        { g_term.closeRemote(); }
void term_detach_remote(void)       { g_term.detachRemote(); }
int  term_remote_connected(void)    { return g_term.remoteConnected() ? 1 : 0; }
void term_send_telnet_negotiation(void) { g_term.sendTelnetNegotiation(); }
void term_send_terminal_init(void)  { g_term.sendTerminalInit(); }
void term_send_terminal_restore(void) { g_term.sendTerminalRestore(); }


/* ================================================================== */
/*  Remote-only I/O                                                    */
/* ================================================================== */

void term_remote_putch(unsigned char c)     { g_term.remotePutch(c); }
void term_remote_write_raw(const char *s)   { g_term.remoteWriteRaw(s); }
int  term_remote_data_ready(void)           { return g_term.remoteDataReady() ? 1 : 0; }
unsigned char term_remote_get_key(void)     { return g_term.remoteGetKey(); }


/* ================================================================== */
/*  Dual-stream output                                                 */
/* ================================================================== */

void term_putch(unsigned char c)    { g_term.putch(c); }
void term_puts(const char *s)       { g_term.puts(s); }
void term_newline(void)             { g_term.newline(); }


/* ================================================================== */
/*  Color                                                              */
/* ================================================================== */

void term_set_attr(unsigned char attr)      { g_term.setAttr(attr); }
unsigned char term_get_attr(void)           { return g_term.attr(); }
void term_set_cur_attr(unsigned char attr)  { g_term.setCurAttr(attr); }
int  term_make_ansi(unsigned char attr, char *buf) { return g_term.makeAnsi(attr, buf); }
void term_emit_attr(int attr)               { g_term.emitAttr(attr); }
int  term_nc_attr(int dos_attr)             { return g_term.ncAttr(dos_attr); }


/* ================================================================== */
/*  Input                                                              */
/* ================================================================== */

int  term_key_ready(void)               { return g_term.keyReady() ? 1 : 0; }
unsigned char term_get_key(void)        { return g_term.getKey(); }
unsigned char term_get_key_nb(void)     { return g_term.getKeyNB(); }
int  term_local_key_ready(void)         { return g_term.localKeyReady() ? 1 : 0; }
unsigned char term_local_get_key(void)  { return g_term.localGetKey(); }
unsigned char term_local_get_key_nb(void) { return g_term.localGetKeyNB(); }


/* ================================================================== */
/*  Screen primitives                                                  */
/* ================================================================== */

void term_clear_screen(void)        { g_term.clearScreen(); }
void term_move_cursor(int x, int y) { g_term.moveCursor(x, y); }
void term_scroll_up(int t, int b, int l) { g_term.scrollUp(t, b, l); }
void term_out1chx(unsigned char ch) { g_term.out1chx(ch); }
void term_out1ch(unsigned char ch)  { g_term.out1ch(ch); }
void term_cr(void)                  { g_term.cr(); }
void term_lf(void)                  { g_term.lf(); }
void term_bs(void)                  { g_term.bs(); }
void term_backspace(void)           { g_term.backspace(); }


/* ================================================================== */
/*  Screen state                                                       */
/* ================================================================== */

int  term_cursor_x(void)            { return g_term.cursorX(); }
int  term_cursor_y(void)            { return g_term.cursorY(); }
int  term_cursor_y_abs(void)        { return g_term.cursorYabs(); }
int  term_screen_bottom(void)       { return g_term.screenBottom(); }
int  term_top_line(void)            { return g_term.topLine(); }
void term_set_top_line(int t)       { g_term.setTopLine(t); }
void term_set_screen_bottom(int b)  { g_term.setScreenBottom(b); }
void term_set_cursor_pos(int x, int y) { g_term.setCursorPos(x, y); }
int  term_local_active(void)        { return g_term.localActive() ? 1 : 0; }


/* ================================================================== */
/*  Screen buffer                                                      */
/* ================================================================== */

char *term_screen_buffer(void)              { return g_term.screenBuffer(); }
void term_set_screen_buffer(char *buf)      { g_term.setScreenBuffer(buf); }
void term_scrn_put(int x, int y, unsigned char ch, unsigned char attr)
{
    g_term.scrnPut(x, y, ch, attr);
}
void term_render_scrn(int start_row, int num_rows)
{
    g_term.renderScrn(start_row, num_rows);
}


/* ================================================================== */
/*  BBS state sync                                                     */
/* ================================================================== */

void term_sync_from_bbs(int tl, int sb, int ca, int cx, int cy)
{
    g_term.setTopLine(tl);
    g_term.setScreenBottom(sb);
    g_term.setCurAttr((unsigned char)ca);
    g_term.setCursorPos(cx, cy);
}

void term_sync_to_bbs(int *ca_out, int *cx_out, int *cy_out)
{
    if (ca_out) *ca_out = g_term.attr();
    if (cx_out) *cx_out = g_term.cursorX();
    if (cy_out) *cy_out = g_term.cursorYabs();
}


/* ================================================================== */
/*  Direct access                                                      */
/* ================================================================== */

void *term_instance(void)
{
    return &g_term;
}
