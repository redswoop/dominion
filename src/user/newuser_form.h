/*
 * newuser_form.h — BBS new user registration ScreenForm
 *
 * make_newuser_form()    — builds the ScreenForm for new user registration
 * newuser_form_apply()   — applies FormResult values to sess.user
 *
 * Called from newuser.cpp after default initialization.
 */

#pragma once
#include "tui/screen_form.h"
#include <string>

/* Build the new user registration form.
 * datadir: path to the gfiles directory (sys.cfg.gfilesdir) — used to
 * locate newans.ans. Must end with path separator. */
ScreenForm make_newuser_form(const std::string& datadir);

/* Apply a completed FormResult to sess.user.
 * Called by the form's on_submit lambda.  Sets name, realname, sex,
 * birthdate, phone, street, city, comment, comp_type, password,
 * and a set of post-form defaults (helplevel, lastconf, etc.). */
void newuser_form_apply(const FormResult& r);
