/*
 * ansi_attr.h — Pure ANSI SGR escape sequence generation (Layer 3)
 *
 * No BBS globals.  No vars.h.  Testable in complete isolation.
 * Caller passes current attribute; function always produces output
 * when attrs differ.  Caller decides whether to call (capability check).
 */

#ifndef _ANSI_ATTR_H_
#define _ANSI_ATTR_H_

void makeansi(unsigned char target_attr, char *out, unsigned char current_attr);

#endif /* _ANSI_ATTR_H_ */
