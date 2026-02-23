#include "io_ncurses.h"  /* MUST come before vars.h */
#include "vars.h"
#include "terminal_bridge.h"

#pragma hdrstop

#include "swap.h"
#include "cp437.h"

/* menuat now in vars.h (Phase B0) */

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
    lines_listed=0;
}


void bs()
{
    term_bs();
}


void out1chx(unsigned char ch)
{
    term_out1chx(ch);
}


/* doinghelp moved to vars.h (Phase B0) */


void out1ch(unsigned char ch)
{
    if(doinghelp) return;

    if (x_only) {
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
                setbeep(1);
                wait1(4);
                setbeep(0);
            }
}


void outs(char *s)
{
    int i;
    char ch;

    for (i=0; s[i]!=0; i++) {
        ch=s[i];

        if (change_color) {
            change_color = 0;
            if ((ch >= '0') && (ch <= '9'))
                ansic(ch - '0');
            break;
        }

        if (ch == 3) {
            change_color = 1;
            break;
        }

        if (change_ecolor) {
            change_ecolor = 0;
            if ((ch >= '0') && (ch <= '9'))
                ansic(ch - '0'+10);
            break;
        }

        if (ch == 14) {
            change_ecolor = 1;
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
    if (l!=topline) {
        if (l>topline) {
            if ((wherey()+topline-l) < 0) {
                /* Scroll down to make room for topscreen */
                SCROLL_UP(topline, screenbottom, l-topline);
                movecsr(wherex(),wherey()+l-topline);
            }
            else {
                oldy += (topline-l);
            }
        }
        else {
            SCROLL_UP(l,topline-1,0);
            oldy += (topline-l);
        }
    }
    topline=l;
    if (using_modem)
        screenlinest=thisuser.screenlines;
    else
        screenlinest=defscreenbottom+1-topline;
}



void savescreen(screentype *s)
{
    if (scrn && s->scrn1)
        memmove(s->scrn1,scrn,screenlen);
    s->x1=wherex();
    s->y1=wherey();
    s->topline1=topline;
    s->curatr1=curatr;
}


void restorescreen(screentype *s)
{
    if (scrn && s->scrn1)
        memmove(scrn,s->scrn1,screenlen);
    topline=s->topline1;
    curatr=s->curatr1;

    /* Redraw screen from scrn buffer via Terminal */
    if (scrn) {
        term_render_scrn(0, screenbottom + 1);
        reset_attr_cache();
    }
    movecsr(s->x1,s->y1);
}




void temp_cmd(char *s, int ccc)
{
    int i;

    pr_wait(1);
    savescreen(&screensave);
    i=topline;
    topline=0;
    curatr=0x07;
    clrscrb();
    runprog(s,ccc);
    restorescreen(&screensave);
    topline=i;
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
    char ch1;
    char *ss, *ss1,s[MAX_PATH_LEN],cmd[128],txt[10],tx[12];
    int f,l,i,type,i1;

    cmd[0]=0;
    ch1=scan_to_char(ch,txt);
    if (ch1) {
        sprintf(s,"%skbdef.dat",syscfg.gfilesdir);
        f=open(s,O_RDONLY | O_BINARY);
        if (f>0) {
            l=filelength(f);
            ss=(char *)farmalloc(l+10);
            if (ss) {
                read(f,ss,l);
                close(f);

                ss[l]=0;
                ss1=strtok(ss,"\r\n");
                while (ss1) {
                    f=1;
                    for(i=0;i<strlen(ss1)&&f;i++) {
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
                farfree(ss);
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
    int i,i1,type;
    char s[MAX_PATH_LEN];

    type=alt_key(ch);
    if(!type) return;

    if (((syscfg.sysconfig & sysconfig_no_local) ==0) && (okskey)) {
        if ((ch>=104) && (ch<=113)) {
            set_autoval(ch-104);
            read_menu(menuat,0);
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
                printf("\nFree Stack: %u",stackavail());
                break;
            case 94:
            case 59: /* F1 */
                if(doinghelp) doinghelp=0;
                val_cur_user(ch==94?0:1);
                read_menu(menuat,0);
                break;
            case 60: /* F2 */
                if(topdata==0) topdata=1;
                else {
                    sprintf(s,"%stops%d.dat",syscfg.gfilesdir,topdata+1);
                    if(exist(s))
                        topdata++;
                    else
                        topdata=0;
                }
                topscreen();
                break;
            case 85: /* Shift f2 */
                topdata=0;
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
                chatcall=0;
                chatreason[0]=0;
                topscreen();
                break;
            case 63: /* F5 */
                hangup=1;
                dtr(0);
                break;
            case 64: /* F6 */
                sysop_alert=!sysop_alert;
                topscreen();
                break;
            case 65: /* F7 */
                thisuser.extratime-=5.0*60.0;
                tleft(0);
                break;
            case 66: /* F8 */
                thisuser.extratime+=5.0*60.0;
                tleft(0);
                break;
            case 67: /* F9 */
                if (thisuser.sl!=255) {
                    if (actsl!=255) {
                        actsl=255;
                        logpr("7!! 0Temp SysOp Access given at 4%s",times());
                    }
                    else {
                        logpr("7! 0Temp SysOp Access Removed");
                        reset_act_sl();
                    }
                    changedsl();
                    read_menu(menuat,0);
                    tleft(0);
                }
                break;
            case 68: /* lF10 */
                if(doinghelp) doinghelp=0;
                if (chatting==0)
                    chat1("",syscfg.sysconfig & sysconfig_2_way);
                else
                    chatting=0;
                break;
            case 46: /* HOME */
                if (chatting) {
                    if (chat_file)
                        chat_file=0;
                    else
                        chat_file=1;
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
                    hangup=1;
                    dtr(0);
                }
                break;

            case 98: /* Ctrl-F5 */
                nl();
                pl("Call back later when you are there.");
                nl();
                hangup=1;
                dtr(0);
                break;
            case 103: /* Ctrl-F10 */
                if (chatting==0)
                    chat1("",!(syscfg.sysconfig & sysconfig_2_way));
                else
                    chatting=0;
                break;
            case 84: /* Shift-F1 */
                set_global_handle(!global_handle);
                topscreen();
                break;
            case 93: /* Shift-F10 */
                temp_cmd(getenv("COMSPEC"),1);
                break;
            case 45:
                if(doinghelp) doinghelp=0;
                savescreen(&screensave);
                textattr(15);
                clrscr();
                cprintf("Exit to Dos? ");
                if(toupper(getche()) == 'Y') {
                    cprintf("\r\nSave Online data? ");
                    if(toupper(getche())=='Y')
                        save_state("exitdata.dom",1);
                    sl1(1,"");
                    userdb_save(usernum,&thisuser);
                    sysoplog("7SysOp BBS Exit");
                    pr_wait(1);
                    if (ok_modem_stuff)
                        closeport();
                    exit(10);
                }
                restorescreen(&screensave);
                break;
            case 72:
                strcpy(charbuffer,";[A");
                charbufferpointer=1;
                break;
            case 80:
                strcpy(charbuffer,";[B");
                charbufferpointer=1;
                break;
            case 75:
                strcpy(charbuffer,";[D");
                charbufferpointer=1;
                break;
            case 77:
                strcpy(charbuffer,";[C");
                charbufferpointer=1;
                break;
            case 71:
                strcpy(charbuffer,";[H");
                charbufferpointer=1;
                break;
            case 79:
                strcpy(charbuffer,";[K");
                charbufferpointer=1;
                break;
            case 35:
                if(!doinghelp) {
                    doinghelp=1;
                    savescreen(&screensave);
                    fastscreen("syshelp.bin");
                }
                else {
                    doinghelp=0;
                    restorescreen(&screensave);
                }
                break;
            case 44:
                save_state("exitdata.dom",1);
                sl1(1,"");
                userdb_save(usernum,&thisuser);
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
    int cx,cy,ctl,cc,i;
    double nsln;
    char tl[80],s[MAX_PATH_LEN],*p,arg[30],oarg[30],oattr;
    FILE *f;
    int x,y,attr,type;

    cx=wherex();
    cy=wherey();
    ctl=topline;
    topline=0;
    cc=curatr;
    nsln=nsl();

    if (topdata) {
        sprintf(s,"%stops%d.tl",syscfg.gfilesdir,topdata);

        f=fopen(s,"rt");

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
                if ((actsl==255) && (thisuser.sl!=255) && !backdoor)
                    strcpy(s,arg);
                break;

            case 3:
                if (global_handle)
                    strcpy(s,arg);
                break;

            case 4:
                if (sysop_alert)
                    strcpy(s,arg);
                break;

            case 5:
                break;

            case 6:
                if (sysop2())
                    strcpy(s,arg);
                break;

            case 7:
                if(useron)
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
        topline=ctl;
        curatr=cc;
        movecsr(cx,cy);
        if (f) fclose(f);
    }

    reset_attr_cache();

    if ((dot) && (useron))
        if ((nsln==0.0) && (thisuser.sl!=255)) {
            if(thisuser.timebank) {
                bank2(60);
                nsln=nsl();
                if(nsln>0.0) return;
            }
            printfile("outtime");
            hangup=1;
        }
}



void topscreen(void)
{
    int cc,cx,cy,ctl,i,x,y,ff,linelen,attr,type,wz=0;
    char ar[17],dar[17],restrict[17],rst[17],lo[6],s[MAX_PATH_LEN],*b,*p;
    char *screen;
    zlogrec z[3];
    FILE *f;
    userrec u;

    status.net_edit_stuff=topdata;

    if(topdata==0) {
/* asm: ax,0x1003 */
/* asm: bl,0x1 */
/* asm: int 0x10 */
        set_protect(0);
        return;
    }

    sprintf(s,"%stops%d.dat",syscfg.gfilesdir,topdata);

    f=fopen(s,"rt");
    if (!f) { set_protect(0); return; }

    fgets(s,81,f);
    linelen=atoi(s);
    topline=linelen;

    set_protect(linelen);

    fgets(s,81,f);

    cx=wherex();
    cy=wherey();
    ctl=topline;
    cc=curatr;
    topline=0;
    movecsr(0,3);

/* asm: ax,0x1003 */
/* asm: bl,0x0 */
/* asm: int 0x10 */

    /* Read topscreen binary into scrn buffer and render via Terminal */
    sprintf(s,"%stops%d.bin",syscfg.gfilesdir,topdata);
    i=open(s,O_RDWR|O_BINARY);
    if (i >= 0) {
        b=(char *)malloca(160*linelen);
        if (b) {
            read(i,b,linelen*160);
            close(i);
            memmove(&scrn[0],b,linelen*160);
            farfree(b);
            term_render_scrn(0, linelen);
        } else {
            close(i);
        }
    } else {
        i = -1; /* file not found, skip */
    }

    sprintf(s,"%shistory.dat",syscfg.datadir);
    ff=open(s,O_RDWR|O_BINARY);
    lseek(ff,0L,SEEK_SET);
    read(ff,(void *)&z[0],sizeof(zlogrec)*3);
    close(ff);

    strcpy(rst,restrict_string);
    for (i=0; i<=15; i++) {
        if (thisuser.ar & (1 << i))
            ar[i]='A'+i;
        else
            ar[i]='a'+i;
        if (thisuser.dar & (1 << i))
            dar[i]='A'+i;
        else
            dar[i]='a'+i;
        if (thisuser.restrict & (1 << i))
            restrict[i]=rst[i];
        else
            restrict[i]=32;
    }
    dar[16]=0;
    ar[16]=0;
    restrict[16]=0;

    if(thisuser.exempt & exempt_ratio) lo[0]='R';
    else lo[0]=32;
    if(thisuser.exempt & exempt_time)  lo[1]='T';
    else lo[1]=32;
    if(thisuser.exempt & exempt_userlist)  lo[2]='U';
    else lo[2]=32;
    if(thisuser.exempt & exempt_post)  lo[3]='P';
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
            strcpy(s,nam(&thisuser,usernum));
            break;
        case 1:
            sprintf(s,"%d",modem_speed);
            break;
        case 3:
            strcpy(s,thisuser.street);
            break;
        case 4:
            strcpy(s,thisuser.city);
            break;
        case 5:
            strcpy(s,thisuser.note);
            break;
        case 6:
            sprintf(s,"%d",status.msgposttoday);
            break;
        case 7:
            sprintf(s,"%d ",status.emailtoday);
            break;
        case 8:
            userdb_load(1,&u);
            sprintf(s,"%d",numwaiting(&u));
            break;
        case 9:
            sprintf(s,"%d",status.fbacktoday);
        case 11:
            sprintf(s,"%d",status.uptoday);
            break;
        case 12:
            sprintf(s,"%d%%",10*status.activetoday/144);
            break;
        case 13:
            sprintf(s,"%d",status.activetoday);
            break;
        case 14:
            sprintf(s,"%d",status.callstoday);
            break;
        case 15:
            strcpy(s,thisuser.laston);
            break;
        case 16:
            sprintf(s,"%s",chatreason[0]? "On":"Off");
            break;
        case 18:
            sprintf(s,thisuser.realname);
            break;
        case 19:
            strcpy(s,thisuser.comment);
            break;
        case 21:
            sprintf(s,"%d",thisuser.sl);
            break;
        case 22:
            sprintf(s,"%d",thisuser.msgpost);
            break;
        case 23:
            sprintf(s,"%d",thisuser.uploaded);
            break;
        case 24:
            sprintf(s,"%d",thisuser.fpts);
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
            sprintf(s,"%d",thisuser.dsl);
            break;
        case 30:
            sprintf(s,"%d",thisuser.logons);
            break;
        case 31:
            sprintf(s,"%d",thisuser.downloaded);
            break;
        case 32:
            sprintf(s,"%d",thisuser.timebank);
            break;
        case 37:
            strcpy(s,thisuser.phone);
            break;
        case 38:
            sprintf(s,"%d",thisuser.age);
            break;
        case 39:
            sprintf(s,"%c",thisuser.sex);
            break;
        case 40:
            strcpy(s,ctype(thisuser.comp_type));
            break;
        case 41:
            sprintf(s,"%s    %4d    %4d    %4d     %3d     %3d    %3d  %3d%%",z[wz].date,z[wz].calls,z[wz].active,z[wz].posts,z[wz].email,z[wz].fback,z[wz].up,10*z[wz].active/144);
            wz++;
            if(wz==3) wz=0;
            break;
        case 42:
            sprintf(s,"%ld",thisuser.uk);
            break;
        case 43:
            sprintf(s,"%ld",thisuser.dk);
            break;
        case 44:
            strcpy(s,thisuser.pw);
            break;
        case 45:
            strcpy(s,status.lastuser);
            break;
        default:
            strcpy(s,"");
            break;
        }
        cprintf(s);
    }

    fclose(f);

    topline=ctl;
    movecsr(cx,cy);
    curatr=cc;
    tleft(0);
    reset_attr_cache();
}

