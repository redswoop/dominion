#ifndef _IO_NCURSES_H_
#define _IO_NCURSES_H_

/*
 * io_ncurses.h — ncurses include guard + macro conflict cleanup
 *
 * MUST be included BEFORE vars.h / platform.h in any .c file that
 * uses ncurses, because ncurses.h declares echo(), getch() etc. which
 * conflict with BBS names.
 */

#include <ncurses.h>

/* ncurses defines getch() as a macro → wgetch(stdscr).
 * Remove it so the BBS can use ncurses input via wgetch() directly. */
#undef getch

/* ncurses may define echo() as a macro — remove to avoid collisions. */
#undef echo

/* 1 if ncurses is active (real terminal), 0 if headless (pipes/files).
 * Owned by terminal_bridge.cpp. */
extern int nc_active;

#endif /* _IO_NCURSES_H_ */
