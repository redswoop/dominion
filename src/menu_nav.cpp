/*
 * menu_nav.cpp — Navigator-based menu loop.
 *
 * Contains the main menu loop (menu_nav_loop) and navigation stack
 * operations (push/pop/goto).  Replaces the old while(!hangup) menuman()
 * loop in bbs.c, and the menuman()/handleinput() code from mm.c.
 *
 * The nav stack is a std::vector<std::string> that replaces the old
 * sess.mstack[10][15] + sess.mdepth.  For compatibility with extrn.c
 * (door program state save/restore), we keep sess.mstack/mdepth
 * synchronized via sync_mstack().
 *
 * menuman() is kept as a public function (one iteration of the menu
 * loop) because lilo.c calls it for the matrix (login) menu.
 */

#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "conio.h"
#include "bbsutl.h"
#include "sysoplog.h"
#include "disk.h"
#include "utility.h"
#include "mm1.h"
#include "stringed.h"
#include "session.h"
#include "system.h"
#include "version.h"

#include "acs.h"
#include "terminal/ansi_attr.h"
#include "chat.h"
#include "cmd_registry.h"
#include "menu_nav.h"
#include "menudb.h"

#include <vector>
#include <string>

extern int SYSTEMDEBUG;


extern struct mmfmt_t {
    char fmt[41], fill, promptfn[8], ansifn[8], ansiftfn[8], center;
} mmfmt;


/* ================================================================
 * Nav stack
 * ================================================================ */

static std::vector<std::string> nav_stack;

/* Keep sess.mstack/mdepth in sync for extrn.c door save/restore. */
static void sync_mstack(void)
{
    auto& sess = Session::instance();
    int depth = (int)nav_stack.size();
    if (depth > 10) depth = 10;
    sess.mdepth = depth;
    for (int i = 0; i < depth; i++) {
        strncpy(sess.mstack[i], nav_stack[i].c_str(), 14);
        sess.mstack[i][14] = '\0';
    }
}


/* ================================================================
 * Navigation operations (called from navcmd in cmd_registry.c)
 * ================================================================ */

int menu_nav_push(const char *menu_name)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    nl();
    /* Save current menu on stack */
    if (nav_stack.size() >= 10) {
        /* Stack overflow: reset to firstmenu + current */
        nav_stack.clear();
        nav_stack.push_back(sys.nifty.firstmenu);
        nav_stack.push_back(sess.menuat);
    } else {
        nav_stack.push_back(sess.menuat);
    }
    sync_mstack();

    if (!readmenu((char *)menu_name)) {
        /* Failed to load — pop back */
        nav_stack.pop_back();
        sync_mstack();
        readmenu((char *)nav_stack.back().c_str());
        return 0;
    }
    return 1;
}

int menu_nav_pop(void)
{
    auto& sys = System::instance();
    nl();
    if (!nav_stack.empty()) {
        std::string prev = nav_stack.back();
        nav_stack.pop_back();
        sync_mstack();
        if (!read_menu((char *)prev.c_str(), 0))
            readmenu(sys.nifty.firstmenu);
    } else {
        nav_stack.clear();
        sync_mstack();
        readmenu(sys.nifty.firstmenu);
    }
    return 1;
}

int menu_nav_goto(const char *menu_name)
{
    auto& sys = System::instance();
    nl();
    nav_stack.clear();
    sync_mstack();
    if (!readmenu((char *)menu_name))
        readmenu(sys.nifty.firstmenu);
    return 1;
}


/* ================================================================
 * Input dispatch (was handleinput() in mm.c)
 * ================================================================ */

static void nav_dispatch(char *s, int begx)
{
    auto& sess = Session::instance();
    int i, i1, c;

    for (i = 0; i < sess.maxcmd && !io.hangup; i++) {
        c = slok(sess.tg[i].sl, 0);
        if (!c)
            continue;

        if (!strcmp(sess.tg[i].key, s) || !strcmp(sess.tg[i].key, "@")) {
            while (wherex() > begx)
                backspace();
            pl(sess.tg[i].line);

            if (!ex(sess.tg[i].type, sess.tg[i].ms))
                return;

        } else if (!strcmp(sess.tg[i].key, "#")) {

            if (sess.tg[i].ms[0] == 'M') {
                for (i1 = 0; i1 < MAX_SUBS; i1++)
                    if (!strcmp(sess.usub[i1].keys, s))
                        sess.cursub = i1;
            } else if (sess.tg[i].ms[0] == 'F') {
                for (i1 = 0; i1 < MAX_DIRS; i1++)
                    if (!strcmp(sess.udir[i1].keys, s))
                        sess.curdir = i1;
            }
        }
    }
}


/* ================================================================
 * One menu iteration (was menuman() in mm.c)
 *
 * Public so lilo.c can call it for the matrix (login) menu.
 * ================================================================ */

void menuman(void)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char cmd, c, begx, s[161], test[MAX_PATH_LEN];
    char *ss = s;
    int i, helpl, i1;
    char avail[61];
    FILE *ff;

    if (!io.charbufferpointer) {
        nl();
        nl();
    }

    tleft(1);

    if (sess.usub[sess.cursub].subnum == -1) sess.cursub = 0;
    helpl = sess.user.helplevel();
    if (sess.pp.helplevel) helpl = sess.pp.helplevel - 1;
    if (!io.charbufferpointer)
        switch (helpl) {
        case 0:
            break;
        case 2:
            showmenu();
        case 1:
            ansic(13);
            npr("[0");
            for (i = 0; i < sess.maxcmd; i++)
                if (!(sess.tg[i].attr == command_pulldown) &&
                    !(sess.tg[i].attr == command_title) &&
                    !(sess.tg[i].attr == command_hidden))
                    npr("%s,", sess.tg[i].key);
            backspace();
            ansic(13);
            npr("]\r\n");
            break;
        }

    if (io.chatcall && sess.chatsoundon) chatsound();
    memset(s, 0, 40);
    sprintf(s, "%s%s.prm", sys.cfg.menudir, mmfmt.promptfn);
    if (exist(s) && (sess.pp.attr & menu_extprompt)) {
        if (sess.pp.attr & menu_promptappend)
            pl(sess.pp.prompt);
        ff = fopen(s, "rt");
        while (fgets(s, 161, ff) != NULL) {
            filter(s, '\n');
            outstr(s);
        }
        fclose(ff);
    } else
        outstr(sess.pp.prompt);
    begx = wherex();
    i1 = c = 0;
    for (i = 0; i < sess.maxcmd; i++) {
        if (sess.tg[i].key[0] == '#') {
            if (sess.tg[i].ms[0] == 'M' || sess.tg[i].ms[0] == 'F')
                i1 = 1;
            else if (strlen(sess.tg[i].key) == 1)
                avail[c++] = '#';
        } else {
            if (strlen(sess.tg[i].key) == 1)
                avail[c++] = sess.tg[i].key[0];
            else if (strlen(sess.tg[i].key) == 2 && sess.tg[i].key[0] == '/')
                avail[c++] = sess.tg[i].key[1];
        }
    }
    avail[c++] = '?';
    avail[c] = 0;

    if (sess.pp.boarder == 0) {
        if (sess.user.sysstatus() & sysstatus_fullline)
            ss = smkey(avail, i1, 1, 1, 1);
        else
            ss = smkey(avail, i1, 1, 0, 0);
    } else if (sess.pp.boarder == 1)
        ss = smkey(avail, i1, 1, 0, 0);
    else
        input(ss, 51);

    strcpy(s, ss);
    if (SYSTEMDEBUG)
        sysoplog(s);

    if (!s[0]) {
        for (cmd = 0; cmd < sess.maxcmd; cmd++) {
            if (sess.tg[cmd].attr == command_default)
                if (!ex(sess.tg[cmd].type, sess.tg[cmd].ms)) cmd = sess.maxcmd;
        }
    } else if ((!strcmp(s, "AVAIL"))) {
        for (i = 0; i < sess.maxcmd; i++) if (slok(sess.tg[i].sl, 1)) npr("2%s0,", sess.tg[i].key);
        backspace();
        nl();
    } else if ((!strcmp(s, "HELP"))) {
        printmenu(7);
    } else if ((!strcmp(s, "SPEED"))) {
        npr("ComSpeed=%d, ModemSpeed=%d\r\n", sess.com_speed, sess.modem_speed);
    } else if ((!strcmp(s, "RELOAD"))) {
        if (sess.actsl <= sys.cfg.newusersl)
            readmenu(sys.nifty.newusermenu);
        else
            readmenu(sys.nifty.firstmenu);
        nav_stack.clear();
        sync_mstack();
    } else if ((!strcmp(s, "VER"))) {
        nl();
        npr("%s, Compiled %s\r\n", wwiv_version, wwiv_date);
        nl();
        i1 = 0;
    } else if ((!strcmp(s, "GRAPHICS"))) {
        ex("OP", "2");
    } else if ((!strcmp(s, "CLS"))) {
        if (okansi()) {
            makeansi(15, test, io.curatr);
            outstr(test);
        }
        outchr(12);
    } else if (strstr(s, ",") || strstr(s, " ")) {
        for (c = 0; c < (int)strlen(s); c++) {
            if (s[c] == ',' || s[c] == 32)
                s[c] = 13;
        }
        c = strlen(s);
        s[c] = 13;
        s[c + 1] = 0;
        strcpy(&io.charbuffer[1], &s[0]);
        io.charbuffer[0] = ';';
        io.charbufferpointer = 1;
    } else if ((!strcmp(s, get_string(37))) && checkacs(6)) {
        getcmdtype();
    } else if ((!strcmp(s, "/OFF"))) {
        io.hangup = 1;
    } else if (s[0] == '?') {
        if (helpl != 2) {
            nl();
            showmenu();
        }
    } else
        nav_dispatch(s, begx);

    if (!io.charbufferpointer)
        nl();
}


/* ================================================================
 * Main menu loop (replaces while(!hangup) menuman() in bbs.c)
 * ================================================================ */

void menu_nav_loop(void)
{
    /* Initialize nav stack */
    nav_stack.clear();
    sync_mstack();

    while (!io.hangup)
        menuman();
}
