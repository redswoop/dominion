/*
 * cmd_registry.cpp — Data-driven command dispatch.
 *
 * Each command "family" (type[0]) has a handler that dispatches on
 * type[1].  The static registry table maps type[0] -> handler.
 *
 * Family handlers:
 *   filecmd()    — 'F' family (file operations)
 *   bbslistcmd() — 'Q' family (BBS list)
 *   navcmd()     — '=' family (menu navigation)
 *   showcmd()    — '?' family (show menu)
 *   msgcommand() — 'M' family fallback (newscan)
 *   othercmd()   — 'O' family fallback (prefs, infoforms, etc.)
 *   sysopcmd()   — 'S' family fallback (SC, S#)
 *   hangupcmd()  — 'I' family (hangup/logoff)
 *
 * Direct registry (exact two-char code → handler):
 *   O*  — 13 'other' commands (sysinfo, chat, bank, etc.)
 *   M*  — 6 message commands (post, mail, scan, etc.)
 *   S*  — 15 sysop commands (boardedit, config, uedit, etc.)
 *   J*  — 3 automessage commands (read, write, reply)
 *   W*  — 3 matrix commands (login, newuser, password)
 *
 * External family handler:
 *   rundoor()    — 'D' (extrn.cpp)
 */

#include "platform.h"
#include "fcns.h"
#include "conio.h"
#include "bbsutl.h"
#include "bbsutl2.h"
#include "disk.h"
#include "utility.h"
#include "jam_bbs.h"
#include "msgbase.h"
#include "file.h"
#include "file1.h"
#include "file2.h"
#include "file3.h"
#include "archive.h"
#include "filesys.h"
#include "config.h"
#include "diredit.h"
#include "newuser.h"
#include "uedit.h"
#include "mm1.h"
#include "stringed.h"
#include "session.h"
#include "system.h"
#include "version.h"
#include "cmd_registry.h"
#include "acs.h"
#include "automsg.h"
#include "bbslist.h"
#include "chat.h"
#include "menu_nav.h"
#include "timebank.h"
#include "extrn.h"
#include "misccmd.h"
#include "sysopf.h"
#include "menued.h"
#include "subedit.h"
#include "lilo.h"
#include "personal.h"

#define sysstatus_pause_on_message 0x0400


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
 * Family dispatchers absorbed from mm.cpp and mm2.cpp
 * ================================================================ */

void badcommand(char onf,char tw)
{
    char s[MAX_PATH_LEN];

    nl();
    sprintf(s,"2\xFA""2\xFA 0Invalid Command Type %c%c",onf,tw);
    sysoplog(s);
    pl(s);
    nl();
}


void msgcommand(char type,char ms[40])
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int c,ok=1,i;
    char s[MAX_PATH_LEN],*p;

    switch(type) {
    case 'N':
        if(ms[1]=='?') {
            p=strtok(ms,";");
            p=strtok(NULL,"");
            if(p!=NULL) outstr(p);
            else
                outstr(get_string2(7));
            ok=yn();
        }
        if(!ok) break;
        if(ms[0]=='G') {
            logtypes(1,"Newscaned All Message Areas");
            gnscan();
        }
        else if (ms[0]=='C') {
            nscan(sess.usub[sess.cursub].subnum,&i);
            logtypes(1,"NewScaned Message Area 4%s",sys.subboards[sess.usub[sess.cursub].subnum].name);
        }
        else if(ms[0]=='A') {
            strcpy(s,sys.conf[sess.curconf].flagstr);
            strcpy(sys.conf[sess.curconf].flagstr,"@");
            changedsl();
            logtypes(1,"NewScaned All Conferences");
            gnscan();
            strcpy(sys.conf[sess.curconf].flagstr,s);
            changedsl();
        }
        else if(!ms[0]) {
            nl();
            outstr(get_string2(8));
            c=onek("\rYNA");
            switch(c) {
            case 'A':
                strcpy(s,sys.conf[sess.curconf].flagstr);
                strcpy(sys.conf[sess.curconf].flagstr,"@");
                changedsl();
                logtypes(1,"NewScaned Messages All Conferences");
                gnscan();
                strcpy(sys.conf[sess.curconf].flagstr,s);
                changedsl();
                break;
            case 'Y':
            case '\r':
                logtypes(1,"Global Newscaned Message Areas");
                gnscan();
                break;
            case 'N':
                logtypes(1,"NewScaned Message Area 4%s",sys.subboards[sess.usub[sess.cursub].subnum].name);
                nscan(sess.usub[sess.cursub].subnum,&i);
                break;
            }
        }
        break;
    default:
        badcommand('M',type);
    }
}

void othercmd(char type,char ms[40])
{
    auto& sess = Session::instance();
    int i;
    char c,s[MAX_PATH_LEN];

    switch(type)
    {
    case 'X':
        if(!ms[0])
            break;
        set_autoval(atoi(ms)-1);
        break;
    case 'D':
        selfValidationCheck(ms);
        break;
    case ';':
        for(c=0;c<strlen(ms);c++) {
            if(ms[c]==';') ms[c]=13;
        }
        ms[strlen(ms)+1]=13;
        strcpy(io.charbuffer,&ms[0]);
        io.charbufferpointer = 1;
        break;
    case 'E':
    case 'G':
        infoform(ms,type=='E');
        logpr(0,"Took InfoForm 4%s",ms);
        break;
    case 'R':
        logtypes(2,"Read Infoforms");
        pl("Which User's responses do you want to see?");
        outstr(": ");
        input(s,31);
        if(s[0]) readform(ms,s);
        break;
#ifdef NUV
    case 'N':
        nuv();
        break;
#endif
    case 'P':
        switch(ms[0]) {
        case '1':
            input_screensize();
            break;
        case '2':
            input_ansistat();
            break;
        case '3':
            if (sess.user.sysstatus & sysstatus_pause_on_page)
                sess.user.sysstatus ^= sysstatus_pause_on_page;
            prt(5,"Pause each screenfull? ");
            if (yn()) sess.user.sysstatus |= sysstatus_pause_on_page;
            nl();
            if (sess.user.sysstatus & sysstatus_pause_on_message)
                sess.user.sysstatus ^= sysstatus_pause_on_message;
            prt(5,"Pause each screenfull while reading messages? ");
            if (yn()) sess.user.sysstatus |= sysstatus_pause_on_message;
            break;
        case '4':
            modify_mailbox();
            break;
        case '5':
            config_qscan(0);
            break;
        case '6':
            input_pw1();
            break;
        case '7':
            make_macros();
            break;
        case '8':
            break;
        case '9':
            outstr("5Use the FullScreen Editor? ");
            sess.user.defed=yn();
            break;
        case '0':
            getmsgformat();
            break;
        case 'A':
            pl("Enter your Default Protocol, 0 for none.");
            i=get_protocol(1);
            if(i>=0||i==-2) sess.user.defprot=i;
            break;
        case 'B':
            break;
        case 'C':
            selecthelplevel();
            break;
        case 'D':
            getfileformat();
            break;
        case 'E':
            config_qscan(1);
            break;
        case 'F':
            print_cur_stat();
            break;
        case 'G':
            change_colors(&sess.user);
            break;
        case 'H':
            inputdat("Enter your Comment",s,sizeof(sess.user.comment),1);
            if(s[0]) strcpy(sess.user.comment,s);
            break;
        case 'J':
            if(sess.user.sysstatus & sysstatus_fullline)
                sess.user.sysstatus ^= sysstatus_fullline;
            npr("5Would you like Hotkey Input? ");
            if(!ny())
                sess.user.sysstatus |= sysstatus_fullline;
            nl();
            break;
        }
    }
}


void sysopcmd(char type,char ms[41])
{
    auto& sess = Session::instance();
    switch(type)
    {
    case 'C':
        { static char _sysop_avail_flag = 0;
        _sysop_avail_flag ^= 0x10;
        pl(_sysop_avail_flag & 0x10 ?
        (char *)"Sysop now unavailable" : (char *)"Sysop now available"); }
        logtypes(3,"Changed Chat Availability");
        topscreen();
        break;
    case '#':
        logtypes(3,"Edited Menus");
        if(ms[0]=='!') menued(sess.menuat);
        else menu("");
        break;
    default:
        badcommand('S',type);
    }
}


void hangupcmd(char type,char ms[40])
{
    auto& sess = Session::instance();
    if(sess.numbatchdl) {
        outstr(get_string(78));
        if(!yn()) return;
    }
    switch(type) {
    case 'H':
        io.hangup=1;
        break;
    case 'A':
    case 'L':
    case 'C':
        nl();
        outstr(ms);
        if(yn()) {
            if(type=='C'||type=='L') {
                outstr("5Leave Feedback to SysOp? ");
                if(yn()) {
                    //                    strcpy(irt,"LogOff Feedback.");
                }
                nl();
                if(type=='L') {
                    outstr("5Leave Message to Next User? ");
                    if(yn()) {
                        write_automessage();
                    }
                }
            }
            printfile("logoff");
            io.hangup=1;
        }
        break;
    default:
        badcommand('I',type);
    }
}


void getcmdtype(void)
{
    menurec mm;

    nl();
    inputdat("Type",mm.type,2,0);
    nl();
    inputdat("Parameters",mm.ms,40,1);
    ex(mm.type,mm.ms);
}


int ex(char type[2],char ms[MAX_PATH_LEN])
{
    int rc = cmd_exec(type, ms);
    if (rc < 0)
        badcommand(type[0], type[1]);
    return rc < 0 ? 1 : rc;
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
    badcommand('J', sub);
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
    badcommand('W', sub);
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
 * Extracted: message commands (was in msgcommand(), mm.cpp)
 * ================================================================ */

static int cmd_yourinfomsg(const char *param)
{
    (void)param;
    yourinfomsg();
    return 1;
}

static int cmd_post(const char *param)
{
    (void)param;
    auto& sess = Session::instance();
    post(sess.cursub);
    return 1;
}

static int cmd_rscanj(const char *param)
{
    (void)param;
    rscanj();
    return 1;
}

static int cmd_readmail(const char *param)
{
    readmailj(atoi(param), 0);
    return 1;
}

static int cmd_smail(const char *param)
{
    auto& sess = Session::instance();
    sess.cursub = 0;
    smail((char *)param);
    return 1;
}

static int cmd_uploadpost(const char *param)
{
    (void)param;
    upload_post();
    return 1;
}

/* ================================================================
 * Extracted: sysop commands (was in sysopcmd(), mm2.cpp)
 * ================================================================ */

static int cmd_boardedit(const char *param)
{
    (void)param;
    logtypes(3, "Edited Message Areas");
    boardedit();
    return 1;
}

static int cmd_glocolor(const char *param)
{
    (void)param;
    glocolor();
    return 1;
}

static int cmd_config(const char *param)
{
    (void)param;
    logtypes(3, "Edited Configuration");
    config();
    return 1;
}

static int cmd_diredit(const char *param)
{
    (void)param;
    logtypes(3, "Edited Directories");
    diredit();
    return 1;
}

static int cmd_readallmail(const char *param)
{
    (void)param;
    logtypes(3, "Read All Mail");
    return 1;
}

static int cmd_chuser(const char *param)
{
    (void)param;
    logtypes(3, "Changed Users");
    chuser();
    return 1;
}

static int cmd_voteprint(const char *param)
{
    (void)param;
    /* voteprint() commented out in original */
    return 1;
}

static int cmd_uedit(const char *param)
{
    (void)param;
    auto& sess = Session::instance();
    logtypes(3, "Edited Users");
    uedit(sess.usernum);
    return 1;
}

static int cmd_ivotes(const char *param)
{
    (void)param;
    logtypes(3, "Editing Voting");
    /* ivotes() commented out in original */
    return 1;
}

static int cmd_zlog(const char *param)
{
    (void)param;
    zlog();
    return 1;
}

static int cmd_edstring(const char *param)
{
    logtypes(3, "Edited Strings");
    if (param[0]) edstring(atoi(param));
    else edstring(0);
    return 1;
}

static int cmd_resetfiles(const char *param)
{
    (void)param;
    reset_files(1);
    return 1;
}

static int cmd_protedit(const char *param)
{
    (void)param;
    logtypes(3, "Edited Protocols");
    protedit();
    return 1;
}

static int cmd_confedit(const char *param)
{
    (void)param;
    logtypes(3, "Edited Conferences");
    confedit();
    return 1;
}

static int cmd_viewlog(const char *param)
{
    (void)param;
    viewlog();
    return 1;
}

/* ================================================================
 * Extracted: automessage commands (was in amsgcommand(), mm2.cpp)
 * ================================================================ */

static int cmd_writeautomsg(const char *param)
{
    (void)param;
    write_automessage();
    return 1;
}

static int cmd_readautomsg(const char *param)
{
    (void)param;
    read_automessage();
    return 1;
}

static int cmd_replyautomsg(const char *param)
{
    (void)param;
    auto& sys = System::instance();
    if (sys.status.amsguser)
        email(sys.status.amsguser, "Reply to AutoMessage", 1);
    return 1;
}

/* ================================================================
 * Extracted: matrix commands (was in matrixcmd(), mm2.cpp)
 * ================================================================ */

static int cmd_checkmatrixpw(const char *param)
{
    (void)param;
    checkmatrixpw();
    return 1;
}

static int cmd_getmatrixpw(const char *param)
{
    (void)param;
    getmatrixpw();
    return 1;
}

static int cmd_matrixnewuser(const char *param)
{
    (void)param;
    nl();
    npr("5Logon as New? ");
    if (yn())
        newuser();
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
    /* message commands (from msgcommand) */
    { "MY", "yourinfomsg",   cmd_yourinfomsg  },
    { "MP", "post",          cmd_post         },
    { "MS", "rscan_j",       cmd_rscanj       },
    { "MM", "read_mail",     cmd_readmail     },
    { "ME", "smail",         cmd_smail        },
    { "MU", "upload_post",   cmd_uploadpost   },
    /* sysop commands (from sysopcmd) */
    { "SB", "board_edit",    cmd_boardedit    },
    { "S-", "glo_color",     cmd_glocolor     },
    { "SP", "config",        cmd_config       },
    { "SF", "dir_edit",      cmd_diredit      },
    { "SM", "read_all_mail", cmd_readallmail  },
    { "SH", "ch_user",       cmd_chuser       },
    { "SI", "vote_print",    cmd_voteprint    },
    { "SU", "user_edit",     cmd_uedit        },
    { "SV", "ivotes",        cmd_ivotes       },
    { "SZ", "zlog",          cmd_zlog         },
    { "SE", "ed_string",     cmd_edstring     },
    { "SR", "reset_files",   cmd_resetfiles   },
    { "SX", "prot_edit",     cmd_protedit     },
    { "SL", "conf_edit",     cmd_confedit     },
    { "SO", "view_log",      cmd_viewlog      },
    /* automessage commands (from amsgcommand) */
    { "JW", "write_automsg", cmd_writeautomsg },
    { "JR", "read_automsg",  cmd_readautomsg  },
    { "JA", "reply_automsg", cmd_replyautomsg },
    /* matrix commands (from matrixcmd) */
    { "WC", "check_matpw",   cmd_checkmatrixpw  },
    { "WL", "get_matpw",     cmd_getmatrixpw    },
    { "WN", "matrix_newuser",cmd_matrixnewuser  },
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
