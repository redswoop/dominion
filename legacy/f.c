#pragma hdrstop

#define _DEFINE_GLOBALS_
#include "vars.h"
#include <math.h>

#define modem_time 3.5
extern unsigned _stklen=30000U;
extern double thing;
extern char menuat[15];
int node=0,SYSTEMDEBUG=0;


void hang_up(void)
{
    int i=0;

    if ((cdet()) && (!no_hangup) && (ok_modem_stuff)) {
        dtr(1);
        while ((i++<2) && (cdet())) {
            wait1(27);
            pr1("+++");
            wait1(54);
            if (modem_i->hang[0])
                pr1(modem_i->hang);
            else
                pr1("ATH{");
            wait1(6);
        }
    }
}


void main(int argc, char *argv[])
{
    char s[81],s1[81],ch;
    int i,i2,v,port,show=0;
    unsigned int ui=0, us=0;
    double dt;
    unsigned short c_s,c_o,dogofer=0,dofilenet=0;

    checkreg();

    if(getenv("DOM")) cd_to(getenv("DOM"));
    if(!exist("Config.dat")) {
        cd_to(searchpath("config.dat"));
    }

    already_on=0;
    endday=0;
    oklevel=0;
    noklevel=255;
    ooneuser=0;
    no_hangup=0;
    ok_modem_stuff=1;
    if (exist("exitdata.dom"))
        restoring_shrink=1;
    else
        restoring_shrink=0;

    port=0;

    textcolor(9);
    cprintf("þ ");
    textcolor(15);
    cprintf("%s, Dominous 1993\n\n\r",wwiv_version);

    for (i=1; i<argc; i++) {
        strcpy(s,argv[i]);
        if ((s[0]=='-') || (s[0]=='/')) {
            ch=toupper(s[1]);
            switch(ch) {
            case '?':
                printf("/B  - someone already logged on at rate (modem speed)\n");
                printf("/S  - used only with /B, indicates com (Locked) speed\n");
                printf("/N  - System Node\n");
                printf("/P  - System Com Port (1-5)\n");
                printf("/Q  - quit the bbs after one user done\n");
                printf("/H  - don't hang up on user when he loggs off\n");
                printf("/M  - don't access modem at all\n");
                printf("/L  - Logon as SysOp Locally\n");
                printf("/C  - Enable flow control\n");
                printf("/I  - Load Quietly(Bypass all loading screens)\n");
                printf("/Z  - Load up like /B, but force ComSpeed to your highest\n");
                printf("/F  - Enable/Disable Mailer Loadup\n");
                printf("/A  - Toss in File Net\n");
                printf("\n");
                exit(0);
            case 'D': 
                SYSTEMDEBUG=1; 
                break;
            case 'A': 
                dofilenet=1; 
                break;
            case 'G':
                show=1;
                dogofer=1; 
                break;
            case 'C': 
                flow_control=1;
                break;
            case 'L': 
                already_on=2; 
                ui=19200; 
                us=19200; 
                break;
            case 'Z':
            case 'B':
                ui=(unsigned int) atol(&(s[2]));
                if ((ui==300)   || (ui==1200) ||  (ui==2400)  || (ui==4800)  ||
                    (ui==7200)  || (ui==9600) ||  (ui==12000) || (ui==14400) ||
                    (ui==19200) || (ui==38400) || (ui==57600) || (ui==16800)) {
                    ultoa((unsigned long) ui,curspeed,10);
                    if (!us&&ch!='Z')
                        us=ui;
                    else if(ch=='Z')
                        us=7000;
                    already_on=1;
                } 
                else {
                    ui=us=0;
                }
                break;
            case 'S':
                us=(unsigned int) atol(&(s[2]));
                if (!((us==300)   || (us==1200)  || (us==2400)  || (us==4800)  ||
                    (us==7200)  || (us==9600)  || (us==12000) || (us==14400) ||
                    (us==19200) || (us==38400) || (us==57600)) || (us==16800)) {
                    us=ui;
                }
                break;
            case 'E':
                oklevel=atoi(&(s[2]));
                break;
            case 'N':
                node=atoi(s+2);
                break;
            case 'P':
                port=atoi(s+2);
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
            }
        }
    }

    init(show);
    if(port!=0) syscfg.primaryport=port;

    if(dofilenet)
        exit(fdnfilenet());

    if(dogofer) {
        gofer();
        exit(0);
    }


    if(us==7000)
        us=syscfg.baudrate[syscfg.primaryport];

    if (_OvrInitExt(0,0)==0)
        printf("\nXMS Memory Found, Will Be Used for Overlay Swapping.\n");
    v = get_dv_version();
    if (running_dv)
        printf("Running under Desqview %d.%02d.\n",v / 256, v % 256);
    if(node) printf("System is Node %d using port %d",node,syscfg.primaryport);

    if(exist("critical")) {
        pl("8Cirtical Errors have occured!  Read Error.log!");
        sound(1400);
        delay(500);
        nosound();
        unlink("critical");
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
            if (!using_modem)
                holdphone(1,0);
            getuser();
        } 
        else {
            holdphone(1,0);
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
            sprintf(s,"%s%s",syscfg.menudir,nifty.firstmenu);
            if(!exist(s)) {
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
            while (!hangup)
                menuman();
            logoff();
        }

        hang_up();
        frequent_init();

wfc_label:
        if (!using_modem)
            holdphone(0,0);
        if ((!no_hangup) && ok_modem_stuff)
            dtr(0);
        already_on=0;
        if (sysop_alert && (!kbhitb())) {
            dtr(1);
            wait1(2);
            holdphone(1,0);
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
            holdphone(0,0);
        }
        sysop_alert=0;
    } 
    while ((!endday) && (!ooneuser));

    bbsCRC();
    outs("\x0c");
    end_bbs(oklevel);

}
