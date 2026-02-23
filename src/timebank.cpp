/* timebank.cpp — Time banking (deposit/withdraw/menu). */

#include "timebank.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "conio.h"
#include "timest.h"
#include "stringed.h"
#include "session.h"
#include "userdb.h"
#include "system.h"
#include "misccmd.h"
#include "sysopf.h"


void add_time(int limit)
{
    auto& sess = Session::instance();
    int minutes;
    char s[MAX_PATH_LEN];
    double nsln;

    nsln=nsl();
    npr("0Time in Bank: 5%d\r\n",sess.user.timebank);
    npr("0Bank Limit  : 5%d\r\n",limit);
    nl();
    npr("5%.0f0 minutes left online.\r\n",nsln/60.0);
    npr("How many minutes would you like to deposit? ");
    mpl(3);
    ansic(4);
    inputl(s,3);
    minutes = atoi(s);
    if (minutes<1) return;
    if (io.hangup) return;
    if (minutes > (int)((nsl() - 20.0)/60.0)) {
        nl();
        prt(7, "You do not have enough time left to deposit that much.");
        nl();
        pausescr();
        return;
    }
    if ((minutes + sess.user.timebank) > limit) {
        nl();
        npr("7You may only have up to %d minutes in your account at once.\r\n", limit);
        nl();
        pausescr();
        return;
    }
    sess.user.timebank += minutes;
    userdb_save(sess.usernum,&sess.user);
    logtypes(2,"Deposit 3%d0 minutes.", minutes);
    nl();
    npr("%d minute%c deposited.", minutes,((minutes > 1) ? 's' : 0));
    nl();

    if (sess.extratimecall > 0) {
        if (sess.extratimecall >= (double)(minutes * 60)) {
            sess.extratimecall -= (double)(minutes * 60);
            return;
        }
        minutes -= (int)(sess.extratimecall / 60.0);
        sess.extratimecall = 0.0;
    }
    sess.user.extratime -= (float)(minutes * 60);
    pausescr();
}

void remove_time()
{
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];
    int minutes;

    nl();
    nl();
    npr( "Time in account: %d minutes.\r\n", sess.user.timebank);
    nl();
    ansic(0);
    outstr("How many minutes would you like to withdraw? ");
    ansic(4);
    inputl(s,3);
    minutes = atoi(s);
    if (minutes<1) return;

    if (io.hangup) return;
    if (minutes > sess.user.timebank) {
        nl();
        nl();
        prt(7, "You don't have that much time in the account!");
        nl();
        nl();
        pausescr();
        return;
    }
    logtypes(2,"Withdrew 3%d0 minutes.", minutes);
    sess.user.extratime += (float)(minutes * 60);
    sess.user.timebank -= minutes;
    nl();
    nl();
    npr("4%d minute%c withdrawn.", minutes,((minutes > 1) ? 's' :0));
    nl();
    pausescr();
}

void bank2(int limit)
{
    auto& sess = Session::instance();
    int done = 0;
    char s[MAX_PATH_LEN], ch;

    logtypes(2,"Entered TimeBank");
    do {
        dtitle("Dominion Time Bank");
        nl();
        tleft(0);
        npr("0Time in Bank: 5%d\r\n",sess.user.timebank);
        npr("0Bank Limit  : 5%d\r\n",limit);
        nl();
        outstr(get_string(72));
        ch = onek("Q\rDW");
        switch (ch) {
        case '\r':
        case 'Q':
            done = 1;
            break;
        case 'D':
            add_time(limit);
            break;
        case 'W':
            if(sess.user.restrict & restrict_timebank) {
                nl();
                pl(get_string(53));
                nl();
                logtypes(2,"Tried to withdraw time, but was dissallowed");
                break;
            }
            remove_time();
            break;
        }
    }
    while ((!io.hangup) && (!done));
    nl();
}
