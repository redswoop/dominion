#include "io_ncurses.h"  /* MUST come before vars.h */
#include "platform.h"
#include "conio.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "tcpio.h"
#include "bbsutl.h"
#include "sysoplog.h"
#include "files/file1.h"
#include "user/uedit.h"
#include "acs.h"
#include "timest.h"
#include "disk.h"
#include "utility.h"
#include "jam_bbs.h"
#include "user/userdb.h"
#include "mm1.h"
#include "stringed.h"
#include "session.h"
#include "system.h"
#include "terminal_bridge.h"
#include "chat.h"
#include "timebank.h"
#include "extrn.h"
#include "misccmd.h"
#include "lilo.h"
#include "bbs_path.h"

#pragma hdrstop

#include "terminal/cp437.h"

/* sess.menuat now in vars.h (Phase B0) */

/* ================================================================== */
/* ANSI terminal console — delegates to Terminal via bridge            */
/* ================================================================== */

/* Reset cached terminal attribute (call after direct ANSI output) */


void reset_attr_cache(void)
{
    term_emit_attr(-1);
}

/* Sync conio cursor position — called by platform_stubs.c clrscr() */
void conio_sync_cursor(int x, int y)
{
    term_set_cursor_pos(x, y);
}


void SCROLL_UP(int t, int b, int l) {
    term_scroll_up(t, b, l);
}

static int wx=0;


void movecsr(int x,int y)
{
    term_move_cursor(x, y);
}


int wherex()
{
    return term_cursor_x();
}


int wherey()
{
    return term_cursor_y();
}


void lf()
{
    term_lf();
}

void cr()
{
    term_cr();
}

void clrscrb()
{
    term_clear_screen();
    io.lines_listed=0;
}


void bs()
{
    term_bs();
}


void out1chx(unsigned char ch)
{
    term_out1chx(ch);
}


/* sess.doinghelp moved to vars.h (Phase B0) */


void out1ch(unsigned char ch)
{
    auto& sess = Session::instance();
    if(sess.doinghelp) return;

    if (io.x_only) {
        if (ch>31) {
            wx=(wx+1)%80;
        }
        else if ((ch==13) || (ch==12)) {
            wx=0;
        }
        else if (ch==8) {
            if (wx)
                wx--;
        }
        return;
    }

    if (ch>31)
        out1chx(ch);
    else
        if (ch==13)
        cr();
    else
        if (ch==10)
            lf();
        else
            if (ch==12)
                clrscrb();
    else
        if (ch==8)
        bs();
    else
        if (ch==7)
            if (outcom==0) {
                wait1(4);
            }
}


void outs(const char *s)
{
    int i;
    char ch;

    for (i=0; s[i]!=0; i++) {
        ch=s[i];

        if (io.change_color) {
            io.change_color = 0;
            if ((ch >= '0') && (ch <= '9'))
                ansic(ch - '0');
            break;
        }

        if (ch == 3) {
            io.change_color = 1;
            break;
        }

        if (io.change_ecolor) {
            io.change_ecolor = 0;
            if ((ch >= '0') && (ch <= '9'))
                ansic(ch - '0'+10);
            break;
        }

        if (ch == 14) {
            io.change_ecolor = 1;
            break;
        }
        out1ch(ch);
    }
}

void copy_line(char *s, char *b, long *ptr, long len)
{
    int i;
    long l;

    if (*ptr>=len) {
        s[0]=0;
        return;
    }
    l=*ptr;
    i=0;
    while ((b[l]!='\r') && (b[l]!='\n') && (l<len)) {
        s[i++]=b[l++];
    }
    s[i]=0;
    if ((b[l]=='\r') && (l<len))
        ++l;
    if ((b[l]=='\n') && (l<len))
        ++l;
    *ptr=l;
}


void set_protect(int l)
{
    auto& sess = Session::instance();
    if (l!=io.topline) {
        if (l>io.topline) {
            if ((wherey()+io.topline-l) < 0) {
                /* Scroll down to make room for topscreen */
                SCROLL_UP(io.topline, io.screenbottom, l-io.topline);
                movecsr(wherex(),wherey()+l-io.topline);
            }
            else {
                io.oldy += (io.topline-l);
            }
        }
        else {
            SCROLL_UP(l,io.topline-1,0);
            io.oldy += (io.topline-l);
        }
    }
    io.topline=l;
    if (using_modem)
        io.screenlinest=sess.user.screenlines();
    else
        io.screenlinest=io.defscreenbottom+1-io.topline;
}


void savescreen(screentype *s)
{
    if (io.scrn && s->scrn1)
        memmove(s->scrn1,io.scrn,io.screenlen);
    s->x1=wherex();
    s->y1=wherey();
    s->topline1=io.topline;
    s->curatr1=io.curatr;
}


void restorescreen(screentype *s)
{
    if (io.scrn && s->scrn1)
        memmove(io.scrn,s->scrn1,io.screenlen);
    io.topline=s->topline1;
    io.curatr=s->curatr1;

    /* Redraw screen from io.scrn buffer via Terminal */
    if (io.scrn) {
        term_render_scrn(0, io.screenbottom + 1);
        reset_attr_cache();
    }
    movecsr(s->x1,s->y1);
}


void temp_cmd(char *s, int ccc)
{
    auto& sess = Session::instance();
    int i;

    pr_wait(1);
    savescreen(&sess.screensave);
    i=io.topline;
    io.topline=0;
    io.curatr=0x07;
    clrscrb();
    runprog(s,ccc);
    restorescreen(&sess.screensave);
    io.topline=i;
    pr_wait(0);
}


char xlate[] = {
    'Q','W','E','R','T','Y','U','I','O','P',0,0,0,0,
    'A','S','D','F','G','H','J','K','L',0,0,0,0,0,
    'Z','X','C','V','B','N','M'
};

char scan_to_char(unsigned char ch,char *s)
{
    if ((ch>=16) && (ch<=50))
        return(xlate[ch-16]);
    else {
        if(ch>58&&ch<69)
            sprintf(s,"F%d",ch-58);
        else if(ch>83&&ch<94)
            sprintf(s,"SF%d",ch-83);
        else if(ch>93&&ch<104)
            sprintf(s,"^F%d",ch-93);
        else if(ch>103&&ch<114)
            sprintf(s,"@F%d",ch-103);
        return(ch);
    }
}

int alt_key(unsigned char ch)
{
    auto& sys = System::instance();
    char ch1;
    char *ss, *ss1,s[MAX_PATH_LEN],cmd[128],txt[10],tx[12];
    int f,l,i,type,i1;

    cmd[0]=0;
    ch1=scan_to_char(ch,txt);
    if (ch1) {
        auto kbpath = BbsPath::join(sys.cfg.gfilesdir, "kbdef.dat");
        f=open(kbpath.c_str(),O_RDONLY | O_BINARY);
        if (f>0) {
            l=filelength(f);
            ss=(char *)malloc(l+10);
            if (ss) {
                read(f,ss,l);
                close(f);

                ss[l]=0;
                ss1=strtok(ss,"\r\n");
                while (ss1) {
                    f=1;
                    for(i=0;i<(int)strlen(ss1)&&f;i++) {
                        if(ss1[i]!=32&&ss1[i]!=',') {
                            tx[i]=ss1[i];
                            tx[i+1]=0;
                        }
                        else {
                            f=0;
                            tx[i]=0;
                        }
                    }

                    i=atoi(ss1);
                    //npr("Txt=%s, ss1=%s, p=%s\r\n",txt,ss1,p);
                    if (toupper(*ss1)==ch1||i==ch1||(!stricmp(txt,tx))) {
                        strtok(ss1," ,");
                        ss1=strtok(NULL,"\r\n");
                        if (ss1)
                            strcpy(cmd,ss1);
                        ss1=NULL;
                    }
                    else
                        ss1=strtok(NULL,"\r\n");
                }
                free(ss);
            }
            else
                close(f);
            if (cmd[0]) {
                if(atoi(cmd))
                    return(atoi(cmd));
                else
                    temp_cmd(cmd,1);
            }
        }
    }
    return(0);
}


void skey(unsigned char ch)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,i1,type;
    char s[MAX_PATH_LEN];

    type=alt_key(ch);
    if(!type) return;

    if (((sys.cfg.sysconfig & sysconfig_no_local) ==0) && (sess.okskey)) {
        if ((ch>=104) && (ch<=113)) {
            set_autoval(ch-104);
            read_menu(sess.menuat,0);
        }
        else
            switch (type) {
            case 120:
            case 121:
            case 122:
            case 123:
            case 124:
            case 125:
            case 126:
            case 127:
            case 128:
                sprintf(s,"WFCBAT%d.bat",ch-119);
                temp_cmd(s,1);
                break;
            case 129:
                temp_cmd("wfcbat0.bat",1);
                break;
            case 81:
                temp_cmd(getenv("DOMDL"),1);
                break;
            case 131:
                break;
            case 94:
            case 59: /* F1 */
                if(sess.doinghelp) sess.doinghelp=0;
                val_cur_user(ch==94?0:1);
                read_menu(sess.menuat,0);
                break;
            case 60: /* F2 */
                if(sess.topdata==0) sess.topdata=1;
                else {
                    char topname[32]; snprintf(topname, sizeof(topname), "tops%d.dat", sess.topdata+1);
                    auto toppath = BbsPath::join(sys.cfg.gfilesdir, topname);
                    if(exist((char*)toppath.c_str()))
                        sess.topdata++;
                    else
                        sess.topdata=0;
                }
                topscreen();
                break;
            case 85: /* Shift f2 */
                sess.topdata=0;
                topscreen();
                break;
            case 61: /* F3 */
                if (using_modem) {
                    incom=opp(incom);
                    dump();
                    topscreen();
                }
                break;
            case 62: /* F4 */
                io.chatcall=0;
                sess.chatreason[0]=0;
                topscreen();
                break;
            case 63: /* F5 */
                io.hangup=1;
                dtr(0);
                break;
            case 64: /* F6 */
                sess.sysop_alert=!sess.sysop_alert;
                topscreen();
                break;
            case 65: /* F7 */
                sess.user.set_extratime(sess.user.extratime() - 5.0*60.0);
                tleft(0);
                break;
            case 66: /* F8 */
                sess.user.set_extratime(sess.user.extratime() + 5.0*60.0);
                tleft(0);
                break;
            case 67: /* F9 */
                if (sess.user.sl()!=255) {
                    if (sess.actsl!=255) {
                        sess.actsl=255;
                        logpr("7!! 0Temp SysOp Access given at 4%s",times());
                    }
                    else {
                        logpr("7! 0Temp SysOp Access Removed");
                        reset_act_sl();
                    }
                    changedsl();
                    read_menu(sess.menuat,0);
                    tleft(0);
                }
                break;
            case 68: /* lF10 */
                if(sess.doinghelp) sess.doinghelp=0;
                if (io.chatting==0)
                    chat1("",sys.cfg.sysconfig & sysconfig_2_way);
                else
                    io.chatting=0;
                break;
            case 46: /* HOME */
                if (io.chatting) {
                    if (io.chat_file)
                        io.chat_file=0;
                    else
                        io.chat_file=1;
                }
                break;
            case 86:
                if(using_modem) outcom=opp(outcom);
                break;
            case 38:
            case 88: /* Shift-F5 */
                i1=(rand() % 20) + 10;
                for (i=0; i<i1; i++)
                    outchr(rand() % 256);
                if(ch==88) {
                    io.hangup=1;
                    dtr(0);
                }
                break;

            case 98: /* Ctrl-F5 */
                nl();
                pl("Call back later when you are there.");
                nl();
                io.hangup=1;
                dtr(0);
                break;
            case 103: /* Ctrl-F10 */
                if (io.chatting==0)
                    chat1("",!(sys.cfg.sysconfig & sysconfig_2_way));
                else
                    io.chatting=0;
                break;
            case 84: /* Shift-F1 */
                set_global_handle(!io.global_handle);
                topscreen();
                break;
            case 93: /* Shift-F10 */
                temp_cmd(getenv("COMSPEC"),1);
                break;
            case 45:
                if(sess.doinghelp) sess.doinghelp=0;
                savescreen(&sess.screensave);
                textattr(15);
                clrscr();
                cprintf("Exit to Dos? ");
                if(toupper(getche()) == 'Y') {
                    cprintf("\r\nSave Online data? ");
                    if(toupper(getche())=='Y')
                        save_state("exitdata.dom",1);
                    sl1(1,"");
                    UserDB::instance().store(sess.usernum, sess.user);
                    sysoplog("7SysOp BBS Exit");
                    pr_wait(1);
                    if (ok_modem_stuff)
                        closeport();
                    exit(10);
                }
                restorescreen(&sess.screensave);
                break;
            case 72:
                strcpy(io.charbuffer,";[A");
                io.charbufferpointer=1;
                break;
            case 80:
                strcpy(io.charbuffer,";[B");
                io.charbufferpointer=1;
                break;
            case 75:
                strcpy(io.charbuffer,";[D");
                io.charbufferpointer=1;
                break;
            case 77:
                strcpy(io.charbuffer,";[C");
                io.charbufferpointer=1;
                break;
            case 71:
                strcpy(io.charbuffer,";[H");
                io.charbufferpointer=1;
                break;
            case 79:
                strcpy(io.charbuffer,";[K");
                io.charbufferpointer=1;
                break;
            case 35:
                if(!sess.doinghelp) {
                    sess.doinghelp=1;
                    savescreen(&sess.screensave);
                    fastscreen("syshelp.bin");
                }
                else {
                    sess.doinghelp=0;
                    restorescreen(&sess.screensave);
                }
                break;
            case 44:
                save_state("exitdata.dom",1);
                sl1(1,"");
                UserDB::instance().store(sess.usernum, sess.user);
                sysoplog("SysOp Quick BBS Exit");
                pr_wait(1);
                if (ok_modem_stuff)
                    closeport();
                exit(10);
                break;
            }
    }
}


void tleft(int dot)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int cx,cy,ctl,cc,i;
    double nsln;
    char tl[80],s[MAX_PATH_LEN],*p,arg[30],oarg[30],oattr;
    FILE *f;
    int x,y,attr,type;

    cx=wherex();
    cy=wherey();
    ctl=io.topline;
    io.topline=0;
    cc=io.curatr;
    nsln=nsl();

    if (sess.topdata) {
        char tlname[32]; snprintf(tlname, sizeof(tlname), "tops%d.tl", sess.topdata);
        auto tlpath = BbsPath::join(sys.cfg.gfilesdir, tlname);

        f=fopen(tlpath.c_str(),"rt");

        if (f) while(fgets(s,81,f)!=NULL) {

            filter(s,'\n');
            p=strtok(s,",");
            x=atoi(p);
            p=strtok(NULL,",");
            y=atoi(p);
            p=strtok(NULL,",");
            attr=atoi(p);
            p=strtok(NULL,",");
            type=atoi(p);
            p=strtok(NULL,",");
            strcpy(arg,p);
            p=strtok(NULL,",");
            oattr=atoi(p);
            p=strtok(NULL,",");
            strcpy(oarg,p);


            movecsr(x-1,y-1);
            textattr(attr);
            strcpy(s,oarg);

            switch(type) {

            case 1:
                if ((using_modem) && (!incom))
                    strcpy(s,arg);
                break;

            case 2:
                if ((sess.actsl==255) && (sess.user.sl()!=255) && !sess.backdoor)
                    strcpy(s,arg);
                break;

            case 3:
                if (io.global_handle)
                    strcpy(s,arg);
                break;

            case 4:
                if (sess.sysop_alert)
                    strcpy(s,arg);
                break;

            case 5:
                break;

            case 6:
                if (sysop2())
                    strcpy(s,arg);
                break;

            case 7:
                if(sess.useron)
                    sprintf(s,"%s",ctim(nsl()));
                else
                    strcpy(s,"Suspended");
                break;
            }
            if(strcmp(s,oarg)==0)
                textattr(oattr);
            cprintf(s);
        }
        textattr(15);
        io.topline=ctl;
        io.curatr=cc;
        movecsr(cx,cy);
        if (f) fclose(f);
    }

    reset_attr_cache();

    if ((dot) && (sess.useron))
        if ((nsln==0.0) && (sess.user.sl()!=255)) {
            if(sess.user.timebank()) {
                bank2(60);
                nsln=nsl();
                if(nsln>0.0) return;
            }
            printfile("outtime");
            io.hangup=1;
        }
}


void topscreen(void)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int cc,cx,cy,ctl,i,x,y,ff,linelen,attr,type,wz=0;
    char ar[17],dar[17],restrict[17],rst[17],lo[6],s[MAX_PATH_LEN],*b,*p;
    char *screen;
    zlogrec z[3];
    FILE *f;
    User u;

    sys.status.net_edit_stuff=sess.topdata;

    if(sess.topdata==0) {
/* asm: ax,0x1003 */
/* asm: bl,0x1 */
/* asm: int 0x10 */
        set_protect(0);
        return;
    }

    {
        char datname[32]; snprintf(datname, sizeof(datname), "tops%d.dat", sess.topdata);
        auto datpath = BbsPath::join(sys.cfg.gfilesdir, datname);
        f=fopen(datpath.c_str(),"rt");
    }
    if (!f) { set_protect(0); return; }

    fgets(s,81,f);
    linelen=atoi(s);
    io.topline=linelen;

    set_protect(linelen);

    fgets(s,81,f);

    cx=wherex();
    cy=wherey();
    ctl=io.topline;
    cc=io.curatr;
    io.topline=0;
    movecsr(0,3);

/* asm: ax,0x1003 */
/* asm: bl,0x0 */
/* asm: int 0x10 */

    /* Read topscreen binary into io.scrn buffer and render via Terminal */
    char binname[32]; snprintf(binname, sizeof(binname), "tops%d.bin", sess.topdata);
    auto binpath = BbsPath::join(sys.cfg.gfilesdir, binname);
    i=open(binpath.c_str(),O_RDWR|O_BINARY);
    if (i >= 0) {
        b=(char *)malloca(160*linelen);
        if (b) {
            read(i,b,linelen*160);
            close(i);
            memmove(&io.scrn[0],b,linelen*160);
            free(b);
            term_render_scrn(0, linelen);
        } else {
            close(i);
        }
    } else {
        i = -1; /* file not found, skip */
    }

    auto histpath = BbsPath::join(sys.cfg.datadir, "history.dat");
    ff=open(histpath.c_str(),O_RDWR|O_BINARY);
    lseek(ff,0L,SEEK_SET);
    read(ff,(void *)&z[0],sizeof(zlogrec)*3);
    close(ff);

    strcpy(rst,restrict_string);
    for (i=0; i<=15; i++) {
        if (sess.user.ar() & (1 << i))
            ar[i]='A'+i;
        else
            ar[i]='a'+i;
        if (sess.user.dar() & (1 << i))
            dar[i]='A'+i;
        else
            dar[i]='a'+i;
        if (sess.user.restrict_flags() & (1 << i))
            restrict[i]=rst[i];
        else
            restrict[i]=32;
    }
    dar[16]=0;
    ar[16]=0;
    restrict[16]=0;

    if(sess.user.exempt() & exempt_ratio) lo[0]='R';
    else lo[0]=32;
    if(sess.user.exempt() & exempt_time)  lo[1]='T';
    else lo[1]=32;
    if(sess.user.exempt() & exempt_userlist)  lo[2]='U';
    else lo[2]=32;
    if(sess.user.exempt() & exempt_post)  lo[3]='P';
    else lo[3]=32;
    lo[4]=0;

    while(fgets(s,81,f)!=NULL) {

        filter(s,'\n');
        p=strtok(s,",");
        x=atoi(p);
        p=strtok(NULL,",");
        y=atoi(p);
        p=strtok(NULL,",");
        attr=atoi(p);
        p=strtok(NULL,",");
        type=atoi(p);

        movecsr(x-1,y-1);
        textattr(attr);

        switch (type) {
        case 0:
            strcpy(s,sess.user.display_name(sess.usernum).c_str());
            break;
        case 1:
            sprintf(s,"%d",sess.modem_speed);
            break;
        case 3:
            strcpy(s,sess.user.street());
            break;
        case 4:
            strcpy(s,sess.user.city());
            break;
        case 5:
            strcpy(s,sess.user.note());
            break;
        case 6:
            sprintf(s,"%d",sys.status.msgposttoday);
            break;
        case 7:
            sprintf(s,"%d ",sys.status.emailtoday);
            break;
        case 8:
            { auto p = UserDB::instance().get(1); if (p) u = *p; }
            sprintf(s,"%d",numwaiting(u));
            break;
        case 9:
            sprintf(s,"%d",sys.status.fbacktoday);
        case 11:
            sprintf(s,"%d",sys.status.uptoday);
            break;
        case 12:
            sprintf(s,"%d%%",10*sys.status.activetoday/144);
            break;
        case 13:
            sprintf(s,"%d",sys.status.activetoday);
            break;
        case 14:
            sprintf(s,"%d",sys.status.callstoday);
            break;
        case 15:
            strcpy(s,sess.user.laston());
            break;
        case 16:
            sprintf(s,"%s",sess.chatreason[0]? "On":"Off");
            break;
        case 18:
            sprintf(s,sess.user.realname());
            break;
        case 19:
            strcpy(s,sess.user.comment());
            break;
        case 21:
            sprintf(s,"%d",sess.user.sl());
            break;
        case 22:
            sprintf(s,"%d",sess.user.msgpost());
            break;
        case 23:
            sprintf(s,"%d",sess.user.uploaded());
            break;
        case 24:
            sprintf(s,"%d",sess.user.fpts());
            break;
        case 25:
            strcpy(s,lo);
            break;
        case 26:
            strcpy(s,restrict);
            break;
        case 27:
            strcpy(s,ar);
            break;
        case 28:
            strcpy(s,dar);
            break;
        case 29:
            sprintf(s,"%d",sess.user.dsl());
            break;
        case 30:
            sprintf(s,"%d",sess.user.logons());
            break;
        case 31:
            sprintf(s,"%d",sess.user.downloaded());
            break;
        case 32:
            sprintf(s,"%d",sess.user.timebank());
            break;
        case 37:
            strcpy(s,sess.user.phone());
            break;
        case 38:
            sprintf(s,"%d",sess.user.age());
            break;
        case 39:
            sprintf(s,"%c",sess.user.sex());
            break;
        case 40:
            strcpy(s,getComputerType(sess.user.comp_type()));
            break;
        case 41:
            sprintf(s,"%s    %4d    %4d    %4d     %3d     %3d    %3d  %3d%%",z[wz].date,z[wz].calls,z[wz].active,z[wz].posts,z[wz].email,z[wz].fback,z[wz].up,10*z[wz].active/144);
            wz++;
            if(wz==3) wz=0;
            break;
        case 42:
            sprintf(s,"%ld",sess.user.uk());
            break;
        case 43:
            sprintf(s,"%ld",sess.user.dk());
            break;
        case 44:
            strcpy(s,sess.user.password());
            break;
        case 45:
            strcpy(s,sys.status.lastuser);
            break;
        default:
            strcpy(s,"");
            break;
        }
        cprintf(s);
    }

    fclose(f);

    io.topline=ctl;
    movecsr(cx,cy);
    io.curatr=cc;
    tleft(0);
    reset_attr_cache();
}

