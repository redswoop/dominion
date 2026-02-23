#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#pragma hdrstop
/* sess.menuat now in vars.h (Phase B0) */
#include <stdarg.h>


void getcmdtype(void)
{
    menurec mm;

    nl();
    inputdat("Type",mm.type,2,0);
    nl();
    inputdat("Parameters",mm.ms,40,1);
    ex(mm.type,mm.ms);
}


void logtypes(char type,char *fmt, ...)
{
    va_list ap;
    char s[512],s1[MAX_PATH_LEN];

    va_start(ap, fmt);
    vsprintf(s, fmt, ap);
    va_end(ap);

    switch(type) {
    case 0: 
        strcpy(s1,"7�7>"); 
        break;
    case 1: 
        strcpy(s1,"5�5�"); 
        break;
    case 2: 
        strcpy(s1,"1�1>"); 
        break;
    case 3: 
        strcpy(s1,"2�2�"); 
        break;
    case 4: 
        strcpy(s1,"3�3>"); 
        break;
    case 5: 
        strcpy(s1,"9#9#9#"); 
        break;
    }

    strcat(s1,"0 ");
    strcat(s1,s);
    if(type==5) sl1(0,s1);
    else
        sysoplog(s1);
}


void badcommand(char onf,char tw)
{
    char s[MAX_PATH_LEN];

    nl();
    sprintf(s,"2�2� 0Invalid Command Type %c%c",onf,tw);
    sysoplog(s);
    pl(s);
    nl();
}


void matrixcmd(char type)
{
    switch(type) {
    default:
        badcommand('W',type);
    }
}

void amsgcommand(char type)
{
    switch(type) {
    default:
        badcommand('J',type);
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
                outstr("5Leave Feedback to SysOp? ");
                if(yn()) {
                    //                    strcpy(irt,"LogOff Feedback.");
                }
                nl();
                if(type=='L') {
                    outstr("5Leave Message to Next User? ");
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


void configpldn(int config)
{
    (void)config;
}

