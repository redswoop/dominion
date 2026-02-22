/*
 * io_ncurses.c — ncurses display layer (Terminal-backed shim)
 *
 * Delegates to the Terminal class via terminal_bridge.h.
 * Terminal owns ncurses initialization, color pair setup, and shutdown.
 * This file provides the legacy C API that conio.c, com.c, etc. call.
 */

#include "io_ncurses.h"
#include "terminal_bridge.h"
#include "vars.h"
#include "cp437.h"

int nc_active = 0;

void ncurses_init(void)
{
    nc_active = term_init_local();

    /* Tell Terminal to use the BBS's scrn buffer (already allocated by init()) */
    if (scrn)
        term_set_screen_buffer(scrn);

    if (!nc_active)
        setvbuf(stdout, NULL, _IOLBF, 0);
}


void ncurses_shutdown(void)
{
    term_shutdown();
    nc_active = 0;
}


int nc_attr(int dos_attr)
{
    return term_nc_attr(dos_attr);
}


void nc_put_cp437(unsigned char ch)
{
    if (!nc_active) return;
    addstr(cp437_to_utf8[ch]);
}


void nc_render_scrn(int start_row, int num_rows)
{
    if (!scrn || !nc_active) return;
    term_render_scrn(start_row, num_rows);
}
