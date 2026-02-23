/*
 * mci_bbs.cpp — BBS resolver callback for MCI expansion
 *
 * This is the ONE file that couples MCI to BBS globals.
 * Translates the ~40 data codes from the old setmci() switch.
 * Side-effect codes (M, P, Y, Z, \, `) return 0 — handled by setmci().
 * Top-ten mutation codes (j, k, l, m, n, g) return 0 — handled by setmci().
 */

#include "io_ncurses.h"  /* must come before vars.h */
#include "platform.h"
#include "fcns.h"
#include "stringed.h"
#include "session.h"
#include "system.h"
#include "topten.h"
#include "userdb.h"
#include "mci.h"
#include "mci_bbs.h"

/* sess.pp now in vars.h (Phase B0) */


/*
 * bbs_mci_resolve — resolver callback
 *
 * Returns 1 + fills buf for pure data codes.
 * Returns 0 for side-effect, mutation, or unknown codes.
 */


static bool bbs_mci_resolve(char code, char *buf, int bufsize, void *ctx)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    (void)ctx;
    char s[161];
    userrec u;

    s[0] = '\0';

    switch (code) {

    /* --- Pure data codes --- */

    case ']':
        snprintf(s, sizeof(s), "%s", sess.pp.pausefile);
        break;
    case '[':
        snprintf(s, sizeof(s), "%s", times());
        break;
    case '!':
        sprintf(s, "%.0f%%", sys.cfg.req_ratio * 100);
        break;
    case '%':
        sprintf(s, "%.0f%%", sys.cfg.post_call_ratio * 100);
        break;
    case '#':
        sprintf(s, "%.0f%%", ratio() * 100);
        break;
    case '$':
        sprintf(s, "%.0f%%", post_ratio() * 100);
        break;
    case '^':
        strcpy(s, ratio() >= sys.cfg.req_ratio ? "Passed" : "Failed");
        break;
    case '&':
        strcpy(s, post_ratio() >= sys.cfg.post_call_ratio ? "Passed" : "Failed");
        break;
    case '*':
        sprintf(s, "%-4d", sess.user.msgpost);
        break;
    case '(':
        sprintf(s, "%-4d", sess.user.logons);
        break;
    case ')':
        sprintf(s, "%-4d", sess.user.ontoday);
        break;
    case '+':
        sprintf(s, "%s", sys.status.lastuser);
        break;
    case '@':
        strcpy(s, get_string(sysop2() ? 4 : 5));
        break;
    case '-':
        sprintf(s, "%s [%s]", nam(&sess.user, sess.usernum), sess.user.comment);
        break;

    case 'a':
        userdb_load(sys.status.amsguser, &u);
        if (sys.status.amsganon == 1) {
            if (so()) {
                strcpy(s, " ");
                strcat(s, nam(&u, sys.status.amsguser));
                strcat(s, " ");
            } else {
                strcpy(s, "Anonymous!");
            }
        } else {
            strcpy(s, nam(&u, sys.status.amsguser));
        }
        break;
    case 'b':
        strcpy(s, sys.proto[sess.user.defprot].description);
        break;
    case 'c':
        sprintf(s, "%d", sess.user.timebank);
        break;
    case 'd':
        sprintf(s, "%d", numwaiting(&sess.user));
        break;
    case 'e':
        strcpy(s, sess.user.comment);
        break;
    case 'f':
        sprintf(s, "%d", sys.nummsgs - sess.msgr);
        break;

    /* Top-ten read codes — pure data, no mutation */
    case 'h':
        strcpy(s, topten(0));
        break;
    case 'i':
        strcpy(s, topten(1));
        break;

    /* Top-ten mutation codes — return 0, handled by setmci() */
    case 'g': return false;
    case 'j': return false;
    case 'k': return false;
    case 'l': return false;
    case 'm': return false;
    case 'n': return false;

    case 'o':
        sprintf(s, "%d", sess.numbatchdl);
        break;
    case 'p':
        sprintf(s, "%d", sess.numbatch - sess.numbatchdl);
        break;
    case 'q':
        sprintf(s, "%-1.f", sess.batchsize);
        break;
    case 'r':
        sprintf(s, "%s", ctim(sess.batchtime));
        break;
    case 't':
        sprintf(s, "%.0f", nsl() / 60.0);
        break;

    case 'A':
        if (sess.usub[sess.cursub].subnum > -1)
            sprintf(s, "%s", sess.usub[sess.cursub].keys);
        else
            strcpy(s, "-1");
        break;
    case 'B':
        if (sess.usub[sess.cursub].subnum > -1)
            sprintf(s, "%s", sys.subboards[sess.usub[sess.cursub].subnum].name);
        else
            strcpy(s, "No Subs");
        break;
    case 'C':
        if (sess.udir[sess.curdir].subnum > -1)
            sprintf(s, "%s", sess.udir[sess.curdir].keys);
        else
            strcpy(s, "-1");
        break;
    case 'D':
        if (sess.udir[sess.curdir].subnum > -1)
            sprintf(s, "%s%s",
                    sys.directories[sess.udir[sess.curdir].subnum].name,
                    (sys.directories[sess.udir[sess.curdir].subnum].mask & mask_no_ratio) ? " [NR]" : "");
        else
            strcpy(s, "No Dirs");
        break;
    case 'E':
        sprintf(s, "%s", sess.user.laston);
        break;
    case 'F':
        sprintf(s, "%d", sess.user.fpts);
        break;
    case 'G':
        sprintf(s, "%s", sys.conf[sess.curconf].name);
        break;
    case 'H':
        sprintf(s, "%s", pnam(&sess.user));
        break;
    case 'I':
        s[0] = '\0';
        break;
    case 'J':
        sprintf(s, "%d", sess.user.dsl);
        break;
    case 'K':
        sprintf(s, "%-4ld", sess.user.uk);
        break;
    case 'L':
        sprintf(s, "%d", sess.usernum);
        break;
    case 'N':
        strcpy(s, nam(&sess.user, sess.usernum));
        break;
    case 'O':
        sprintf(s, "%-4d", sess.user.downloaded);
        break;
    case 'Q':
        sprintf(s, "%d", sess.numf);
        break;
    case 'R':
        sprintf(s, "%s", sess.user.realname);
        break;
    case 'S':
        itoa(sess.user.sl, s, 10);
        break;
    case 'T':
        strcpy(s, ctim(nsl()));
        break;
    case 'U':
        sprintf(s, "%-4d", sess.user.uploaded);
        break;
    case 'V':
        sprintf(s, "%d", sess.msgr);
        break;
    case 'W':
        sprintf(s, "%d", sys.nummsgs);
        break;
    case 'X':
        sprintf(s, "%-4ld", sess.user.dk);
        break;

    /* --- Side-effect codes: return 0, handled by setmci() --- */
    case '`': return false;
    case '\\': return false;
    case 'M': return false;
    case 'P': return false;
    case 'Y': return false;
    case 'Z': return false;

    default: return false;
    }

    /* Copy result to caller's buffer */
    strncpy(buf, s, bufsize - 1);
    buf[bufsize - 1] = '\0';
    return true;
}


/*
 * mci_bbs_init — register resolver and ${name} mappings
 *
 * Called once from main() after io_init().
 */
void mci_bbs_init(void)
{
    mci_set_resolver(bbs_mci_resolve, nullptr);

    /* ${name} → backtick code mappings */
    mci_register_name("username", 'N');
    mci_register_name("realname", 'R');
    mci_register_name("sl", 'S');
    mci_register_name("dsl", 'J');
    mci_register_name("timeleft", 'T');
    mci_register_name("timemins", 't');
    mci_register_name("usernum", 'L');
    mci_register_name("laston", 'E');
    mci_register_name("subname", 'B');
    mci_register_name("subkeys", 'A');
    mci_register_name("dirname", 'D');
    mci_register_name("dirkeys", 'C');
    mci_register_name("conference", 'G');
    mci_register_name("sysopavail", '@');
    mci_register_name("time", '[');
    mci_register_name("logons", '(');
    mci_register_name("ontoday", ')');
    mci_register_name("lastcaller", '+');
    mci_register_name("mailwaiting", 'd');
    mci_register_name("msgpost", '*');
    mci_register_name("uploaded", 'U');
    mci_register_name("downloaded", 'O');
    mci_register_name("uk", 'K');
    mci_register_name("dk", 'X');
    mci_register_name("ratio", '#');
    mci_register_name("postratio", '$');
    mci_register_name("phone", 'H');
    mci_register_name("comment", 'e');
    mci_register_name("protocol", 'b');
    mci_register_name("timebank", 'c');
    mci_register_name("filepoints", 'F');
    mci_register_name("pause", 'P');
    mci_register_name("newline", 'M');
}
