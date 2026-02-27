/*
 * terminal_bridge.h — C-callable wrappers around the Terminal class
 *
 * This is the seam between BBS C code and the C++ Terminal layer.
 * All functions are extern "C" so they can be called from .c files.
 * The Terminal instance is owned by terminal_bridge.cpp (static g_term).
 */

#ifndef TERMINAL_BRIDGE_H_
#define TERMINAL_BRIDGE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* -- Lifecycle -- */
int  term_init_local(void);          /* returns 1 if ncurses active */
void term_shutdown(void);

/* -- Remote TCP stream -- */
void term_set_remote(int fd);
void term_set_remote_no_iac(int fd);  /* like setRemote but no telnet IAC */
void term_close_remote(void);
void term_detach_remote(void);
int  term_remote_connected(void);
void term_send_telnet_negotiation(void);
void term_send_terminal_init(void);
void term_send_terminal_restore(void);

/* -- Remote-only I/O -- */
void term_remote_putch(unsigned char c);
void term_remote_write_raw(const char *s);
int  term_remote_data_ready(void);
unsigned char term_remote_get_key(void);

/* -- Dual-stream output -- */
void term_putch(unsigned char c);
void term_puts(const char *s);
void term_newline(void);

/* -- Color -- */
void term_set_attr(unsigned char attr);
unsigned char term_get_attr(void);
void term_set_cur_attr(unsigned char attr);
int  term_make_ansi(unsigned char attr, char *buf);
void term_emit_attr(int attr);
int  term_nc_attr(int dos_attr);
void term_put_cp437(unsigned char ch);

/* -- Input -- */
int  term_key_ready(void);
unsigned char term_get_key(void);
unsigned char term_get_key_nb(void);
int  term_local_key_ready(void);
unsigned char term_local_get_key(void);
unsigned char term_local_get_key_nb(void);

/* -- Screen primitives -- */
void term_clear_screen(void);
void term_clear_to_eol(void);
void term_move_cursor(int x, int y);
void term_goto(int x, int y);          /* cursor position: local + TCP */
void term_scroll_up(int top, int bottom, int lines);
void term_out1chx(unsigned char ch);
void term_out1ch(unsigned char ch);
void term_cr(void);
void term_lf(void);
void term_bs(void);
void term_backspace(void);

/* -- Screen state -- */
int  term_cursor_x(void);
int  term_cursor_y(void);
int  term_cursor_y_abs(void);
int  term_screen_bottom(void);
int  term_top_line(void);
void term_set_top_line(int t);
void term_set_screen_bottom(int b);
void term_set_cursor_pos(int x, int yabs);
int  term_local_active(void);

/* -- Screen buffer -- */
char *term_screen_buffer(void);
void term_set_screen_buffer(char *buf);
void term_scrn_put(int x, int y, unsigned char ch, unsigned char attr);
void term_render_scrn(int start_row, int num_rows);

/* -- State binding (Phase 3: share memory with BBS io_session_t) -- */
void term_bind_state(int *ca, int *tl, int *sb);

/* -- Resize -- */
void term_resize(int rows, int cols);

/* -- Direct access for transition period -- */
void *term_instance(void);  /* returns Terminal* */

#ifdef __cplusplus
}
#endif

#endif /* TERMINAL_BRIDGE_H_ */
