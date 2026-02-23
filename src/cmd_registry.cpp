/*
 * cmd_registry.c — Data-driven command dispatch.
 *
 * Each command "family" (type[0]) has a handler that dispatches on
 * type[1].  The static registry table maps type[0] -> handler.
 *
 * Inline handlers that used to live in ex() (mm.c) are extracted here:
 *   filecmd()    — 'F' family (file operations)
 *   bbslistcmd() — 'Q' family (BBS list)
 *   navcmd()     — '=' family (menu navigation)
 *   showcmd()    — '?' family (show menu)
 *
 * Existing handlers stay in their original files:
 *   msgcommand() — 'M' (mm.c)
 *   othercmd()   — 'O' (mm.c)
 *   sysopcmd()   — 'S' (mm2.c)
 *   hangupcmd()  — 'I' (mm2.c)
 *   amsgcommand()— 'J' (mm2.c)
 *   matrixcmd()  — 'W' (mm2.c)
 *   rundoor()    — 'D' (extrn.c)
 */

#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#include "version.h"
#include "cmd_registry.h"
#include "menu_nav.h"


/* ================================================================
 * Extracted: file commands  (was inline in ex(), case 'F')
 * ================================================================ */

static int filecmd(char type, const char *ms_const)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int abort, i, i1;
    char s[255], c, ok = 1, *p;
    char ms[MAX_PATH_LEN];

    strncpy(ms, ms_const, sizeof(ms) - 1);
    ms[sizeof(ms) - 1] = '\0';

    switch (type) {
    case 'Z':
        inputdat("Enter Path to Use (Include Trailing Backslash)", s, 60, 1);
        insert_dir(sess.umaxdirs, s, 99, 0);
        sess.udir[sess.umaxdirs].subnum = sess.umaxdirs;
        sess.curdir = sess.umaxdirs;
        logtypes(3, "Created temporary path in 3%s", s);
        break;
    case 'M':
        mark(sess.curdir);
        break;
    case 'L':
        listfiles(ms);
        break;
    case 'B':
        batchdled(0);
        break;
    case 'G':
        listgen();
        break;
    case 'Y':
        yourinfodl();
        break;
    case 'D':
        if (ms[0]) newdl(atoi(ms));
        else newdl(sess.curdir);
        break;
    case 'V':
        arc_cl(1);
        break;
    case 'R':
        removefile();
        break;
    case 'P':
        setldate();
        break;
    case 'F':
        finddescription(ms);
        break;
    case '1':
        localupload();
        break;
    case '2':
        editfile();
        break;
    case '3':
        nl();
        outstr("5Validate Globally? ");
        if (yn()) {
            i = sess.curdir;
            for (sess.curdir = 0; sess.curdir < sys.num_dirs && sess.udir[sess.curdir].subnum >= 0; sess.curdir++)
                valfiles();
            sess.curdir = i;
        } else
            valfiles();
        break;
    case '5':
        move_file();
        break;
    case '6':
        sort_all(ms);
        logtypes(3, "Sorted Files");
        break;
    case '8':
        unlisteddl(ms);
        break;
    case '9':
        arc_cl(0);
        logtypes(3, "Added Archive Comments");
        break;
    case '0':
        arc_cl(2);
        logtypes(3, "Added Gif Comment to Files");
        break;
    case '!':
        arc_cl(3);
        logtypes(3, "Added Automatic File Descriptions");
        break;
    case 'C':
        create_file();
        break;
    case 'U':
        upload(ms);
        break;
    case 'N':
        io.lines_listed = 0;
        nl();
        if (ms[1] == '?') {
            p = strtok(ms, ";");
            p = strtok(NULL, "");
            if (p != NULL) outstr(p);
            else
                outstr(get_string2(7));
            ok = yn();
        }
        if (!ok) break;
        abort = 0;
        if (ms[0] == 'C') {
            setformat();
            nscandir(sess.curdir, &abort, 0, &i);
            logtypes(1, "NewScaned File Area %s", sys.directories[sess.udir[sess.curdir].subnum].name);
            break;
        }
        if (ms[0] == 'G') {
            nscanall();
            logtypes(1, "NewScaned All File Areas");
            break;
        } else if (ms[0] == 'A') {
            strcpy(s, sys.conf[sess.curconf].flagstr);
            strcpy(sys.conf[sess.curconf].flagstr, "@");
            changedsl();
            logtypes(1, "NewScaned File Areas on All Conferences");
            nscanall();
            strcpy(sys.conf[sess.curconf].flagstr, s);
            changedsl();
        } else if (!ms[0]) {
            nl();
            outstr(get_string2(8));
            c = onek("\rYNA");
            switch (c) {
            case 'A':
                strcpy(s, sys.conf[sess.curconf].flagstr);
                strcpy(sys.conf[sess.curconf].flagstr, "@");
                changedsl();
                logtypes(1, "NewScaned File Areas on All Conferences");
                nscanall();
                strcpy(sys.conf[sess.curconf].flagstr, s);
                changedsl();
                break;
            case 'Y':
            case '\r':
                logtypes(1, "Global Newscaned File Areas");
                nscanall();
                break;
            case 'N':
                nscandir(sess.curdir, &abort, 0, &i);
                logtypes(1, "NewScaned File Area %s", sys.directories[sess.udir[sess.curdir].subnum].name);
            }
        }
        break;
    default:
        badcommand('F', type);
    }
    return 1;
}


/* ================================================================
 * Extracted: BBS list commands  (was inline in ex(), case 'Q')
 * ================================================================ */

static int bbslistcmd(char type, const char *ms)
{
    switch (type) {
    case 'A':
        addbbs(ms[0] ? (char *)ms : (char *)"bbslist.msg");
        break;
    case 'R':
        printfile(ms[0] ? (char *)ms : (char *)"bbslist.msg");
        pausescr();
        break;
    case 'S':
        searchbbs(ms[0] ? (char *)ms : (char *)"bbslist.msg");
        break;
    default:
        badcommand('Q', type);
    }
    return 1;
}


/* ================================================================
 * Extracted: navigation commands  (was inline in ex(), case '=')
 * Returns 0 for menu-change commands, 1 otherwise.
 * ================================================================ */

static int navcmd(char type, const char *ms_const)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i, i1;
    char *p;
    char ms[MAX_PATH_LEN];

    strncpy(ms, ms_const, sizeof(ms) - 1);
    ms[sizeof(ms) - 1] = '\0';

    switch (type) {
    case 'F':
        if (ms[0]) strcpy(sess.pp.format, ms);
        readmnufmt(sess.pp);
        break;
    case 'G':
        p = strtok(ms, ",;");
        i = atoi(p);
        p = strtok(NULL, ";,");
        i1 = atoi(p);
        go(i, i1);
        break;
    case 'B':
        menubatch(ms);
        return 0;
    case '#':
        break;
    case 'J':
        jumpconf(ms);
        break;
    case ']':
        sess.curconf++;
        if (sess.curconf >= sys.num_conf) sess.curconf = 0;
        changedsl();
        break;
    case '[':
        sess.curconf--;
        if (sess.curconf < 0) sess.curconf = sys.num_conf - 1;
        changedsl();
        break;
    case '/':
        menu_nav_push(ms);
        return 0;
    case '\\':
        menu_nav_pop();
        return 0;
    case '^':
        menu_nav_goto(ms);
        return 0;
    case '*':
        if (ms[0] == 'M') sess.cursub = sublist(ms[1]);
        else if (ms[0] == 'F') sess.curdir = dirlist(ms[1]);
        break;
    case '+':
        if (ms[0] == 'M') {
            if (sess.cursub < 64 && sess.usub[sess.cursub + 1].subnum >= 0) {
                ++sess.cursub;
                sess.msgr++;
            } else
                sess.cursub = 0;
            sess.msgr = 1;
        } else if (ms[0] == 'F') {
            if (sys.directories[sess.udir[sess.curdir].subnum].type)
                changedsl();
            if (sess.curdir < 64 && sess.udir[sess.curdir + 1].subnum >= 0) ++sess.curdir;
            else sess.curdir = 0;
        }
        break;
    case '-':
        if (ms[0] == 'M') {
            if (sess.cursub > 0)
                --sess.cursub;
            else
                while ((sess.usub[sess.cursub + 1].subnum >= 0) && (sess.cursub < 64))
                    ++sess.cursub;
            sess.msgr = 1;
        } else if (ms[0] == 'F') {
            if (sys.directories[sess.udir[sess.curdir].subnum].type)
                changedsl();
            if (sess.curdir > 0) sess.curdir--;
            else
                while ((sess.udir[sess.curdir + 1].subnum >= 0) && (sess.curdir < 63)) ++sess.curdir;
        }
        break;
    default:
        badcommand('=', type);
    }
    return 1;
}


/* ================================================================
 * Thin wrappers — adapt existing handlers to registry signature
 * ================================================================ */

static int cmd_show(char sub, const char *param)
{
    (void)sub; (void)param;
    showmenu();
    return 1;
}

static int cmd_amsg(char sub, const char *param)
{
    (void)param;
    amsgcommand(sub);
    return 1;
}

static int cmd_door(char sub, const char *param)
{
    rundoor(sub, (char *)param);
    return 1;
}

static int cmd_msg(char sub, const char *param)
{
    msgcommand(sub, (char *)param);
    return 1;
}

static int cmd_file(char sub, const char *param)
{
    return filecmd(sub, param);
}

static int cmd_sysop(char sub, const char *param)
{
    sysopcmd(sub, (char *)param);
    return 1;
}

static int cmd_other(char sub, const char *param)
{
    othercmd(sub, (char *)param);
    return 1;
}

static int cmd_hangup(char sub, const char *param)
{
    hangupcmd(sub, (char *)param);
    return 1;
}

static int cmd_matrix(char sub, const char *param)
{
    (void)param;
    matrixcmd(sub);
    return 1;
}

static int cmd_bbslist(char sub, const char *param)
{
    return bbslistcmd(sub, param);
}

static int cmd_nav(char sub, const char *param)
{
    return navcmd(sub, param);
}


/* ================================================================
 * Extracted: individual commands (exact-match layer)
 * ================================================================ */

static int cmd_sysinfo(const char *param)
{
    (void)param;
    nl();
    npr("%s, Compiled %s\r\n", wwiv_version, wwiv_date);
    nl();
    pausescr();
    printfile("system");
    pausescr();
    return 1;
}

static int cmd_lastcallers(const char *param)
{
    (void)param;
    lastfewcall();
    return 1;
}

static int cmd_oneliner(const char *param)
{
    (void)param;
    oneliner();
    return 1;
}

static int cmd_listusers(const char *param)
{
    (void)param;
    logtypes(2, "Listed Users");
    list_users();
    return 1;
}

static int cmd_printline(const char *param)
{
    pl((char *)param);
    return 1;
}

static int cmd_printfile(const char *param)
{
    printfile((char *)param);
    return 1;
}

static int cmd_sysoponline(const char *param)
{
    (void)param;
    if (sysop2()) pl(get_string(4));
    else pl(get_string(5));
    return 1;
}

static int cmd_addsay(const char *param)
{
    (void)param;
    addsay();
    return 1;
}

static int cmd_getsay(const char *param)
{
    (void)param;
    get_say(1);
    return 1;
}

static int cmd_searchrum(const char *param)
{
    (void)param;
    searchrum();
    return 1;
}

static int cmd_bank(const char *param)
{
    bank2(atoi(param));
    return 1;
}

static int cmd_todayhistory(const char *param)
{
    (void)param;
    today_history();
    return 1;
}

static int cmd_chat(const char *param)
{
    auto& sys = System::instance();
    if (!(sys.nifty.nifstatus & nif_chattype)) reqchat((char *)param);
    else reqchat1((char *)param);
    return 1;
}


/* ================================================================
 * Direct registry — exact two-char code → handler
 * Checked before family dispatch, so extracted commands override.
 * ================================================================ */

static const cmd_direct_t direct_registry[] = {
    { "OI", "sysinfo",       cmd_sysinfo      },
    { "OW", "last_callers",  cmd_lastcallers  },
    { "O1", "oneliner",      cmd_oneliner     },
    { "OU", "list_users",    cmd_listusers    },
    { "OL", "print_line",    cmd_printline    },
    { "OF", "print_file",    cmd_printfile    },
    { "OO", "sysop_online",  cmd_sysoponline  },
    { "O|", "add_say",       cmd_addsay       },
    { "OS", "get_say",       cmd_getsay       },
    { "O\\","search_rum",    cmd_searchrum    },
    { "OT", "bank",          cmd_bank         },
    { "OA", "today_history", cmd_todayhistory },
    { "OC", "chat",          cmd_chat         },
    { "",   NULL,             NULL             }
};


/* ================================================================
 * Registry table
 * ================================================================ */

static const cmd_family_t registry[] = {
    { '?', "show_menu",   cmd_show    },
    { 'J', "auto_msg",    cmd_amsg    },
    { 'D', "door",        cmd_door    },
    { 'M', "message",     cmd_msg     },
    { 'F', "file",        cmd_file    },
    { 'S', "sysop",       cmd_sysop   },
    { 'O', "other",       cmd_other   },
    { 'I', "hangup",      cmd_hangup  },
    { 'W', "matrix",      cmd_matrix  },
    { 'Q', "bbslist",     cmd_bbslist },
    { '=', "navigation",  cmd_nav     },
    { 0,   NULL,          NULL        }
};


const cmd_family_t *cmd_lookup(char prefix)
{
    const cmd_family_t *p;
    for (p = registry; p->prefix; p++) {
        if (p->prefix == prefix)
            return p;
    }
    return NULL;
}


int cmd_exec(const char type[3], const char *param)
{
    /* Exact match first — individually extracted commands */
    for (const cmd_direct_t *d = direct_registry; d->code[0]; d++) {
        if (d->code[0] == type[0] && d->code[1] == type[1])
            return d->handler(param);
    }

    /* Fall back to family dispatch */
    const cmd_family_t *fam = cmd_lookup(type[0]);
    if (!fam)
        return -1;
    return fam->handler(type[1], param);
}
