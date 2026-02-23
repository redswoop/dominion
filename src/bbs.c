#pragma hdrstop

#define _DEFINE_GLOBALS_
#include "vars.h"
#include <math.h>
#include "menudb.h"
#include "mci_bbs.h"

#define modem_time 3.5
/* extern unsigned _stklen=30000U; — DOS stack size, not needed on macOS */
extern double thing;
extern char menuat[15];
int node=0,SYSTEMDEBUG=0;


int main(int argc, char *argv[])
{
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],ch;
    int i,i2,v,port,show=0;
    unsigned int ui=0, us=0;
    double dt;
    unsigned short c_s,c_o;
    int splash_pause=0;

    if(getenv("DOM")) cd_to(getenv("DOM"));
    if(!exist("config.json")) {
        cd_to(searchpath("config.json"));
    }

    io_init(&io);
    mci_bbs_init();
    already_on=0;
    endday=0;
    oklevel=0;
    noklevel=255;
    ooneuser=0;
    no_hangup=0;
    ok_modem_stuff=1;
    tcp_port=0;
    listen_fd=-1;
    term_raw_mode=0;
    if (exist("exitdata.dom"))
        restoring_shrink=1;
    else
        restoring_shrink=0;

    port=0;

    textcolor(9);
    cprintf("� ");
    textcolor(15);
    cprintf("%s, Dominous 1993\n\n\r",wwiv_version);

    for (i=1; i<argc; i++) {
        strcpy(s,argv[i]);
        if ((s[0]=='-') || (s[0]=='/')) {
            ch=toupper(s[1]);
            switch(ch) {
            case '?':
                printf("/N  - System Node\n");
                printf("/P  - Com Port (1-5) or TCP port (e.g. -P2323)\n");
                printf("/Q  - quit the bbs after one user done\n");
                printf("/H  - don't hang up on user when he loggs off\n");
                printf("/M  - don't access modem at all\n");
                printf("/L  - Logon as SysOp Locally\n");
                printf("/I  - Load Quietly(Bypass all loading screens)\n");
                printf("/W  - Pause at splash screen\n");
                printf("\n");
                exit(0);
            case 'D': 
                SYSTEMDEBUG=1; 
                break;
            case 'L': 
                already_on=2; 
                ui=19200; 
                us=19200; 
                break;
            case 'E':
                oklevel=atoi(&(s[2]));
                break;
            case 'N':
                node=atoi(s+2);
                break;
            case 'P':
                if (s[2] >= '0' && s[2] <= '9' && atoi(s+2) > 255) {
                    tcp_port=atoi(s+2);
                } else {
                    port=atoi(s+2);
                }
                break;
            case 'Q':
                ooneuser=1;
                break;
            case 'H':
                no_hangup=1;
                break;
            case 'I':
                show=opp(show);
                break;
            case 'M':
                ok_modem_stuff=0;
                break;
            case 'W':
                splash_pause=1;
                break;
            }
        }
    }

    init(show);
    if(port!=0) syscfg.primaryport=port;

    if (_OvrInitExt(0,0)==0)
        cprintf("\nXMS Memory Found, Will Be Used for Overlay Swapping.\n");
    if(node) cprintf("System is Node %d using port %d",node,syscfg.primaryport);

    if(exist("critical")) {
        pl("8Cirtical Errors have occured!  Read Error.log!");
        sound(1400);
        delay(500);
        nosound();
        unlink("critical");
    }

    if(splash_pause) {
        printf("\nPress any key...");
        fflush(stdout);
        getchar();
    }

    if (restoring_shrink) {
        restoring_shrink=0;
        _setcursortype(2);
        switch(restore_data("exitdata.dom")) {
        case 0: /* WFC */
            _setcursortype(0);
            goto wfc_label;
        case 1: /* main menu */
        case 2:
            read_menu(menuat,0);
            goto main_menu_label;
        }
    } 
    else _setcursortype(0);


    do {
        bbsCRC();
        if (already_on)
            gotcaller(ui, us);
        else
             getcaller();

        if (using_modem>-1) {
                getuser();
        } 
        else {
                using_modem=0;
            checkit=0;
            okmacro=1;
            usernum=1;
            reset_act_sl();
            changedsl();
        }

        bbsCRC();

        if (!hangup) {
            logon();
            if(!menudb_exists(nifty.firstmenu)) {
                pl("8Main Menu is missing!!  System cannot Continue!  If Possible, Inform SysOp!");
                logpr("7!0 MAIN MENU MISSING.  Hanging up on User");
                pausescr();
                hangup=1;
            } 
            else {
                if(actsl<=syscfg.newusersl) readmenu(nifty.newusermenu);
                else readmenu(nifty.firstmenu);
            }
main_menu_label:
            while (!hangup) menuman();
            logoff();
        }

hanging_up:
        if (client_fd >= 0)
            send_terminal_restore(client_fd);
        if (!no_hangup && ok_modem_stuff)
            dtr(0);

        frequent_init();
wfc_label:
        if ((!no_hangup) && ok_modem_stuff)
            dtr(0);
        already_on=0;
        if (sysop_alert && (!kbhitb())) {
            dt=timer();
            nl();
            pl("User Has Logged Off");
            nl();
            while ((!kbhitb()) && (fabs(timer()-dt)<60.0)) {
                setbeep(1);
                wait1(9);
                setbeep(0);
                wait1(18);
            }
            clrscrb();
        }
        sysop_alert=0;
    } 
    while ((!endday) && (!ooneuser));

    bbsCRC();
    outs("\x0c");
    end_bbs(oklevel);

}
