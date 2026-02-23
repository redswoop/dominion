/*
 * menu_nav.h — Navigator-based menu loop.
 *
 * Replaces the old while(!hangup) menuman() loop in bbs.c.
 * Manages the menu stack with proper push/pop/goto operations.
 *
 * Navigation functions are called from navcmd() in cmd_registry.c.
 */

#ifndef _MENU_NAV_H_
#define _MENU_NAV_H_

/* Main menu loop.  Called from bbs.c after login. */
void menu_nav_loop(void);

/* One menu iteration.  Called from lilo.cpp matrix loop. */
void menuman(void);

/* Navigation operations — called from navcmd() in cmd_registry.c.
 * These modify the nav stack and load the new menu.
 * Returns 1 on success, 0 on failure (fallback to firstmenu). */
int menu_nav_push(const char *menu_name);
int menu_nav_pop(void);
int menu_nav_goto(const char *menu_name);

#endif /* _MENU_NAV_H_ */
