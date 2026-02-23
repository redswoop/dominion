#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#include "version.h"
#pragma hdrstop



static auto& sys = System::instance();
static auto& sess = Session::instance();

int begxx,begyy;
/* sess.menuat now in vars.h (Phase B0) */

void bbsCRC(void)
{
    int i,i1=0;

    for(i=0;i<strlen(wwiv_version);i++)
        i1+=wwiv_version[i];

    if(i1!=CRCVal) {
        clrscr();
        textattr(128+12);
        cprintf("\n\nExecutable Integrity has been compromised. %d\r\n",i1);
        textattr(15);
        cprintf("Please Report this to the Innocents as soon as possible.\n");
        exit(0);
    }
}

int so()
{
    if (checkacs(9))
        return(1);
    else
        return(0);
}

int cs()
{
    slrec ss;

    ss=sys.cfg.sl[sess.actsl];
    if (so())
        return(1);
    if ((ss.ability & ability_cosysop)||checkacs(8))
        return(1);
    else
        return(0);
}


int lcs()
{
    slrec ss;

    ss=sys.cfg.sl[sess.actsl];
    if (cs())
        return(1);
    if (ss.ability & ability_limited_cosysop) {
        if (sess.user.subop==255)
            return(1);
        if (sess.user.subop==sess.usub[sess.cursub].subnum)
            return(1);
        else
            return(0);
    } 
    else
        return(0);
}

/*
void makewindow(int x, int y, int xlen, int ylen)
{
    int i,xx,yy,old;
    unsigned char s[MAX_PATH_LEN];

    old=io.curatr;
    if (xlen>80)
        xlen=80;
    if (ylen>(io.screenbottom+1-io.topline))
        ylen=(io.screenbottom+1-io.topline);
    if ((x+xlen)>80)
        x=80-xlen;
    if ((y+ylen)>io.screenbottom+1)
        y=io.screenbottom+1-ylen;

    xx=wherex();
    yy=wherey();
    textcolor(8);
    for (i=1; i<xlen-1; i++)
        s[i]=196;
    s[0]=194;
    s[xlen-1]=194;
    s[xlen]=0;
    movecsr(x,y);
    cprintf(" ");
    cprintf("�");
    cprintf(s);
    cprintf("�");
    cprintf(" ");
    s[0]=193;
    s[xlen-1]=193;
    movecsr(x,y+ylen-1);
    cprintf(" ");
    cprintf("�");
    cprintf(s);
    cprintf("�");
    cprintf(" ");
    movecsr(x+1,y+1);
    for (i=1; i<xlen-1; i++)
        s[i]=32;
    s[0]=179;
    s[xlen-1]=179;
    for (i=1; i<ylen-1; i++) {
        movecsr(x,i+y);
        cprintf(" ");
        cprintf("�");
        cprintf(s);
        cprintf("�");
        cprintf(" ");
    }
    movecsr(xx,yy);
    io.curatr=old;
}
*/

int rc;

void outsat(char *s, int x, int y)
{
    movecsr(x-1,y-1);
    outs(s);
}


void editdata(char *str,int len,int xcoord,int ycoord)
{
    int i;
    char s[MAX_PATH_LEN];

    strcpy(s,str);
    movecsr(xcoord-1,ycoord-1);
    editline(s,len,ALL,&rc,"");
    for(i=strlen(s)-1;i>=0 && s[i]==32;i--);
    s[i+1]=0;
    strcpy(str,s);
    io.curatr=15;
}


int editdig(char *str,int len,int xcoord,int ycoord)
{
    int real;

    movecsr(xcoord-1,ycoord-1);
    editline(str,len,NUM_ONLY,&rc,"");
    real=atoi(str);
    io.curatr=15;
    return(real);
}


void editline(char *s, int len, int status, int *returncode, char *ss)
{
    int i,j,k,oldatr,cx,cy,pos,ch,done,insert,i1;

    oldatr=io.curatr;
    cx=wherex();
    cy=wherey();
    for (i=strlen(s); i<len; i++)
        s[i]=32;
    s[len]=0;
    io.curatr=31;
    outs(s);
    movecsr(cx,cy);
    done=0;
    pos=0;
    insert=0;
    do {
        ch=getchd();
        if (ch==0) {
            ch=getchd();
            switch (ch) {
            case 59:
                done=1;
                *returncode=DONE;
                break;
            case 71: 
                pos=0; 
                movecsr(cx,cy); 
                break;
            case 79: 
                pos=len; 
                movecsr(cx+pos,cy); 
                break;
            case 77: 
                if (pos<len) {
                    pos++;
                    movecsr(cx+pos,cy);
                }
                break;
            case 75: 
                if (pos>0) {
                    pos--;
                    movecsr(cx+pos,cy);
                }
                break;
            case 72:
            case 15:
                done=1;
                *returncode=PREV;
                break;
            case 80:
                done=1;
                *returncode=NEXT;
                break;
            case 82:
                if (status!=SET) {
                    if (insert)
                        insert=0;
                    else
                        insert=1;
                }
                break;
            case 83:
                if (status!=SET) {
                    for (i=pos; i<len; i++)
                        s[i]=s[i+1];
                    s[len-1]=32;
                    movecsr(cx,cy);
                    outs(s);
                    movecsr(cx+pos,cy);
                }
                break;
            }
        } 
        else {
            if (ch>31) {
                if (status==UPPER_ONLY)
                    ch=toupper(ch);
                if (status==SET) {
                    ch=toupper(ch);
                    if (ch!=' ') {
                        i1=1;
                        for (i=0; i<len; i++)
                            if ((ch==ss[i]) && (i1)) {
                                i1=0;
                                pos=i;
                                movecsr(cx+pos,cy);
                                if (s[pos]==' ')
                                    ch=ss[pos];
                                else
                                    ch=' ';
                            }
                        if (i1)
                            ch=ss[pos];
                    }
                }
                if ((pos<len)&&((status==ALL) || (status==UPPER_ONLY) || (status==SET) ||
                    ((status==NUM_ONLY) && (((ch>='0') && (ch<='9')) || (ch==' '))))) {
                    if (insert) {
                        for (i=len-1; i>pos; i--)
                            s[i]=s[i-1];
                        s[pos++]=ch;
                        movecsr(cx,cy);
                        outs(s);
                        movecsr(cx+pos,cy);
                    } 
                    else {
                        s[pos++]=ch;
                        out1ch(ch);
                    }
                }
            } 
            else {
                ch=ch;
                switch(ch) {
                case 13:
                case 9:
                    done=1;
                    *returncode=NEXT;
                    break;
                case 27:
                    done=1;
                    *returncode=DONE;
                    break;
                case 8:
                    if (pos>0) {
                        if (insert) {
                            for (i=pos-1; i<len; i++)
                                s[i]=s[i+1];
                            s[len-1]=32;
                            pos--;
                            movecsr(cx,cy);
                            outs(s);
                            movecsr(cx+pos,cy);
                        } 
                        else {
                            pos--;
                            movecsr(cx+pos,cy);
                        }
                    }
                    break;
                }
            }
        }
    } 
    while (done==0);
    movecsr(cx,cy);
    io.curatr=oldatr;
    outs(s);
    movecsr(cx,cy);
}


void reprint()
{
    char xl[MAX_PATH_LEN], cl[MAX_PATH_LEN], atr[MAX_PATH_LEN], cc, ansistr_1[MAX_PATH_LEN];
    int ansiptr_1;

    ansiptr_1=io.ansiptr;
    io.ansiptr=0;
    io.ansistr[ansiptr_1]=0;
    strcpy(ansistr_1,io.ansistr);

    savel(cl, atr, xl, &cc);
    nl();
    restorel(cl, atr, xl, &cc);

    strcpy(io.ansistr,ansistr_1);
    io.ansiptr=ansiptr_1;
}


void setbeep(int i)
{
    (void)i; /* PC speaker port I/O removed — no equivalent on macOS */
}



void pr_wait(int i1)
{
    int i;

    if(i1) {
        outstr(get_string(24)); 
        begxx=wherex(); 
        begyy=wherey();
    }
    else {
        for(i=0;i<strlenc(get_string(24));i++) backspace();
    }
}

void set_autoval(int n)
{
    valrec v;

    v=sys.cfg.autoval[n];

    sess.user.sl=v.sl;
    sess.user.dsl=v.dsl;
    sess.user.ar=v.ar;
    sess.user.dar=v.dar;
    sess.user.restrict=v.restrict;
    reset_act_sl();
    changedsl();
}

extern int rc;

void clickat(long byte,long bit,int x, int y)
{
    movecsr(x-1,y-1);
    if(byte & bit) {
        io.curatr=31;
        outs("�");
    } 
    else {
        io.curatr=15;
        outs(" ");
    }
}

int click(long *byt,long bit,int x, int y)
{
    long byte;
    int done=0,returncode=0;
    char ch;

    byte= *byt;

    while(!done) {
        movecsr(x-1,y-1);
        if(byte & bit) {
            io.curatr=31;
            outs("�");
        } 
        else {
            io.curatr=15;
            outs(" ");
        }

        movecsr(x-1,y-1);

        ch=getchd();
        if(!ch) {
            ch=getchd();
            switch (ch) {
            case 59:
                done=1;
                returncode=DONE;
                break;
            case 72:
                done=1;
                returncode=PREV;
                break;
            case 80:
                done=1;
                returncode=NEXT;
                break;
            }
            break;
        } 
        else
            switch(ch) {
        case 13:
            returncode=NEXT;
            done=1;
            break;
        case 32:
            togglebit(&byte,bit);
            break;
        case 27:
            done=1;
            returncode=DONE;
            break;
        } 
    } 

    *byt= byte;
    io.curatr=15;
    return returncode;
}

void val_cur_user(int wait)
{
    char sl[4],dsl[4],ar[17],dar[17],restrict[17],ex[17],delff[3];
    char dk[34],uk[34],ndl[33],nul[33],fpts[34],psts[5],lgns[5],timebank[5];
    int cp,i,done;
    char nsave[41];


    strcpy(nsave,sess.user.name);

    if(wait) pr_wait(1);
    sess.user.sl=sess.actsl;
    savescreen(&sess.screensave);
    io.curatr=11;
    ansic(3);
    fastscreen("top.bin");
    itoa((int)sess.user.sl,sl,10);
    itoa((int)sess.user.dsl,dsl,10);
    itoa((int)sess.user.dk,dk,10);
    itoa((int)sess.user.uk,uk,10);
    itoa((int)sess.user.downloaded,ndl,10);
    itoa((int)sess.user.uploaded,nul,10);
    itoa((int)sess.user.fpts,fpts,10);
    itoa((int)sess.user.msgpost,psts,10);
    itoa((int)sess.user.logons,lgns,10);
    itoa((int)sess.user.timebank,timebank,10);
    strcpy(ex,"");

    for (i=0; i<=15; i++) {
        if (sess.user.ar & (1 << i))
            ar[i]='A'+i;
        else
            ar[i]=32;
        if (sess.user.dar & (1 << i))
            dar[i]='A'+i;
        else
            dar[i]=32;
        if (sess.user.restrict & (1 << i))
            restrict[i]=restrict_string[i];
        else
            restrict[i]=32;
    }
    dar[16]=0;
    ar[16]=0;
    restrict[16]=0;

    cp=1;
    done=0;

    io.curatr=15;

    outsat(sess.user.name,16,2);
    outsat(sess.user.realname,53,2);
    outsat(sl,16,3);
    outsat(dsl,53,3);
    outsat(ar,16,4);
    outsat(dar,53,4);
    outsat(sess.user.phone,16,5);
    outsat(restrict,53,5);
    outsat(nul,16,6);
    outsat(ndl,53,6);
    outsat(uk,16,7);
    outsat(dk,53,7);
    outsat(lgns,16,8);
    outsat(psts,53,8);
    outsat(fpts,16,9);
    outsat(timebank,53,9);

    outsat(sess.user.city,16,15);
    outsat(sess.user.street,16,16);
    outsat(sess.user.comment,16,17);
    outsat(sess.user.note,16,18);

    clickat(sess.user.inact,inact_deleted,25,11);
    clickat(sess.user.inact,inact_deleted,41,11);

    clickat(sess.user.sysstatus,sysstatus_ansi,22,12);
    clickat(sess.user.sysstatus,sysstatus_avatar,34,12);
    clickat(sess.user.sysstatus,sysstatus_color,45,12);
    clickat(sess.user.sysstatus,sysstatus_rip,54,12);

    clickat(sess.user.exempt,exempt_time,22,13);
    clickat(sess.user.exempt,exempt_ratio,32,13);
    clickat(sess.user.exempt,exempt_post,41,13);
    clickat(sess.user.exempt,exempt_userlist,54,13);

    while (done==0) {
        switch(cp) {
        case 1: 
            editdata(sess.user.name,25,16,2);
            strupr(sess.user.name);
            break;
        case 2: 
            editdata(sess.user.realname,25,53,2);
            break;
        case 3: 
            sess.user.sl=editdig(sl,3,16,3);
            sess.actsl=sess.user.sl; 
            break;
        case 4: 
            sess.user.dsl=editdig(dsl,3,53,3);
            break;
        case 5: 
            movecsr(16-1,4-1);
            editline(ar,16,SET,&rc,"ABCDEFGHIJKLMNOP");
            sess.user.ar=0;
            for (i=0; i<=15; i++)
                if (ar[i]!=' ') sess.user.ar |= (1 << i);
            break;
        case 6: 
            movecsr(53-1,4-1);
            editline(dar,16,SET,&rc,"ABCDEFGHIJKLMNOP");
            sess.user.dar=0;
            for (i=0; i<=15; i++)
                if (dar[i]!=' ') sess.user.dar |= (1 << i);
            break;
        case 7:
            editdata(sess.user.phone,12,16,5);
            break;
        case 8: 
            movecsr(53-1,5-1);
            editline(restrict,16,SET,&rc,restrict_string);
            sess.user.restrict=0;
            for (i=0; i<=15; i++)
                if (restrict[i]!=' ') sess.user.restrict |= (1 << i);
            break;
        case 9: 
            sess.user.uploaded=editdig(nul,5,16,6);
            break;
        case 10: 
            sess.user.downloaded=editdig(ndl,5,53,6);
            break;
        case 11: 
            sess.user.uk=editdig(uk,5,16,7);
            break;
        case 12: 
            sess.user.dk=editdig(dk,5,53,7);
            break;
        case 13: 
            sess.user.logons=editdig(lgns,5,16,8);
            break;
        case 14: 
            sess.user.msgpost=editdig(psts,5,53,8);
            break;
        case 15: 
            sess.user.fpts=editdig(fpts,5,16,9);
            break;
        case 16: 
            sess.user.timebank=editdig(timebank,3,53,9);
            break;

        case 17:
            rc=click((long *)&sess.user.inact,inact_deleted,25,11);
            break;
        case 18:
            rc=click((long *)&sess.user.inact,inact_deleted,41,11);
            break;

        case 19:
            rc=click((long *)&sess.user.sysstatus,sysstatus_ansi,22,12);
            break;
        case 20:
            rc=click((long *)&sess.user.sysstatus,sysstatus_avatar,34,12);
            break;
        case 21:
            rc=click((long *)&sess.user.sysstatus,sysstatus_color,45,12);
            break;
        case 22:
            rc=click((long *)&sess.user.sysstatus,sysstatus_rip,54,12);
            break;


        case 23:
            rc=click((long *)&sess.user.exempt,exempt_time,22,13);
            break;
        case 24:
            rc=click((long *)&sess.user.exempt,exempt_ratio,32,13);
            break;
        case 25:
            rc=click((long *)&sess.user.exempt,exempt_post,41,13);
            break;
        case 26:
            rc=click((long *)&sess.user.exempt,exempt_userlist,54,13);
            break;

        case 27:
            editdata(sess.user.city,53,16,15);
            break;
        case 28:
            editdata(sess.user.street,53,16,16);
            break;
        case 29:
            editdata(sess.user.comment,53,16,17);
            break;
        case 30:
            editdata(sess.user.note,53,16,18);
            break;
        }
        switch(rc) {
        case DONE: 
            done=1; 
            break;
        case NEXT: 
            if (cp<30) cp++;
            else cp=1; 
            break;
        case PREV: 
            if (cp>1) cp--; 
            else cp=30;
            break;

        }
    }

    restorescreen(&sess.screensave);
    changedsl();

    if(strcmp(nsave,sess.user.name))
        reset_files(0);

    if(wait) pr_wait(0);
}


