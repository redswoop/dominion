#ifndef _IO_NCURSES_H_
#define _IO_NCURSES_H_

/*
 * io_ncurses.h — ncurses display layer for local sysop console
 *
 * IMPORTANT: This header MUST be included BEFORE vars.h / platform.h
 * in any .c file that uses ncurses, because:
 *   1. ncurses.h declares echo(), nl(), filter(), getch() — these names
 *      conflict with BBS functions of the same name.
 *   2. io_stream.h (via vars.h) defines `#define echo io.echo` which
 *      would corrupt ncurses's echo() declaration if seen first.
 *
 * This header includes ncurses.h, then removes conflicting macros so
 * BBS headers can be included safely afterward.
 */

#include <ncurses.h>

/* ncurses defines getch() as a macro → wgetch(stdscr).
 * Remove it so the BBS can use ncurses input via wgetch() directly. */
#undef getch

/* ncurses may define echo() as a macro.
 * Remove it before io_stream.h redefines 'echo' as an object-like macro. */
#undef echo

/* 1 if ncurses is active (real terminal), 0 if headless (pipes/files) */
extern int nc_active;

/* Initialize ncurses — call early in init(), replaces manual raw mode.
 * Skips ncurses and falls back to raw termios if stdout is not a tty. */
void ncurses_init(void);

/* Shutdown ncurses — call in end_bbs() */
void ncurses_shutdown(void);

/* Convert DOS attribute byte to ncurses attribute value */
int nc_attr(int dos_attr);

/* Put a CP437 character to stdscr using ncurses (UTF-8 conversion) */
void nc_put_cp437(unsigned char ch);

/* Render a region of the scrn[] shadow buffer to ncurses */
void nc_render_scrn(int start_row, int num_rows);

#endif /* _IO_NCURSES_H_ */
