#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#pragma hdrstop

#include "menudb.h"
#include "cmd_registry.h"
#include "acs.h"


#define sysstatus_pause_on_message 0x0400

int slok(char val[31],char menu)
{
    acs_context_t ctx;

    acs_fill_context(&ctx);
    if (menu == 3) menu = 0;
    if (!menu) ctx.can_post = 0;  /* 'C' always denies in execution mode */

    return acs_check(val, &ctx);
}


void msgcommand(char type,char ms[40])
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int c,c1,ok=1,i;
    char s[MAX_PATH_LEN],*p;
    unsigned long l;

    switch(type) {
    case 'Y': 
        yourinfomsg();
        break;
    case 'P': 
        post(sess.cursub); 
        break;
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
            logtypes(1,"NewScaned Message Area 4%s",sys.subboards[sess.usub[sess.cursub].subnum].name);
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
                logtypes(1,"NewScaned Message Area 4%s",sys.subboards[sess.usub[sess.cursub].subnum].name);
                nscan(sess.usub[sess.cursub].subnum,&i); 
                break;
            }
        }
        break;
    case 'S': 
        rscanj();  
        break;
    case 'M': 
        readmailj(atoi(ms),0); 
        break;
    case 'E': 
        sess.cursub=0; 
        smail(ms);
        break;
    case 'U': 
        upload_post(); 
        break;
#ifdef QWK
    case 'W': 
        makeqwk(ms[0]); 
        break;
    case 'Z': 
        qwkreply(); 
        break;
#endif
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
        logpr(0,"Took InfoForm 4%s",ms);
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
            outstr("5Use the FullScreen Editor? ");
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
            npr("5Would you like Hotkey Input? ");
            if(!ny())
                sess.user.sysstatus |= sysstatus_fullline;
            nl();
            break;
        }
    }
}

int ex(char type[2],char ms[MAX_PATH_LEN])
{
    int rc = cmd_exec(type, ms);
    if (rc < 0)
        badcommand(type[0], type[1]);
    return rc < 0 ? 1 : rc;
}

/* menuman() and handleinput() moved to menu_nav.cpp (Phase 5) */
