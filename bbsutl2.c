#include "vars.h"
#pragma hdrstop
#include <conio.h>

#define frequency 500

int begxx,begyy;
extern char menuat[15];

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

    ss=syscfg.sl[actsl];
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

    ss=syscfg.sl[actsl];
    if (cs())
        return(1);
    if (ss.ability & ability_limited_cosysop) {
        if (thisuser.subop==255)
            return(1);
        if (thisuser.subop==usub[cursub].subnum)
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
    unsigned char s[81];

    old=curatr;
    if (xlen>80)
        xlen=80;
    if (ylen>(screenbottom+1-topline))
        ylen=(screenbottom+1-topline);
    if ((x+xlen)>80)
        x=80-xlen;
    if ((y+ylen)>screenbottom+1)
        y=screenbottom+1-ylen;

    xx=wherex();
    yy=wherey();
    directvideo=1;
    textcolor(8);
    for (i=1; i<xlen-1; i++)
        s[i]=196;
    s[0]=194;
    s[xlen-1]=194;
    s[xlen]=0;
    movecsr(x,y);
    cprintf(" ");
    cprintf("Ú");
    cprintf(s);
    cprintf("¿");
    cprintf(" ");
    s[0]=193;
    s[xlen-1]=193;
    movecsr(x,y+ylen-1);
    cprintf(" ");
    cprintf("À");
    cprintf(s);
    cprintf("Ù");
    cprintf(" ");
    movecsr(x+1,y+1);
    for (i=1; i<xlen-1; i++)
        s[i]=32;
    s[0]=179;
    s[xlen-1]=179;
    for (i=1; i<ylen-1; i++) {
        movecsr(x,i+y);
        cprintf(" ");
        cprintf("³");
        cprintf(s);
        cprintf("³");
        cprintf(" ");
    }
    movecsr(xx,yy);
    curatr=old;
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
    char s[81];

    strcpy(s,str);
    movecsr(xcoord-1,ycoord-1);
    editline(s,len,ALL,&rc,"");
    for(i=strlen(s)-1;i>=0 && s[i]==32;i--);
    s[i+1]=0;
    strcpy(str,s);
    curatr=15;
}


int editdig(char *str,int len,int xcoord,int ycoord)
{
    int real;

    movecsr(xcoord-1,ycoord-1);
    editline(str,len,NUM_ONLY,&rc,"");
    real=atoi(str);
    curatr=15;
    return(real);
}


void editline(char *s, int len, int status, int *returncode, char *ss)
{
    int i,j,k,oldatr,cx,cy,pos,ch,done,insert,i1;

    oldatr=curatr;
    cx=wherex();
    cy=wherey();
    for (i=strlen(s); i<len; i++)
        s[i]=32;
    s[len]=0;
    curatr=31;
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
    curatr=oldatr;
    outs(s);
    movecsr(cx,cy);
}


void reprint()
{
    char xl[81], cl[81], atr[81], cc, ansistr_1[81];
    int ansiptr_1;

    ansiptr_1=ansiptr;
    ansiptr=0;
    ansistr[ansiptr_1]=0;
    strcpy(ansistr_1,ansistr);

    savel(cl, atr, xl, &cc);
    nl();
    restorel(cl, atr, xl, &cc);

    strcpy(ansistr,ansistr_1);
    ansiptr=ansiptr_1;
}


void setbeep(int i)
{
    int i1,i2;

    if (i) {
        i1 = 0x34DD / frequency;
        i2 = inportb(0x61);
        if (!(i2 & 0x03)) {
            outportb(0x61, i2 | 0x03);
            outportb(0x43, 0xB6);
        }
        outportb(0x42, i1 & 0x0F);
        outportb(0x42, i1 >> 4);
    } 
    else
        outportb(0x61, inportb(0x61) & 0xFC);
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

    v=syscfg.autoval[n];

    thisuser.sl=v.sl;
    thisuser.dsl=v.dsl;
    thisuser.ar=v.ar;
    thisuser.dar=v.dar;
    thisuser.restrict=v.restrict;
    reset_act_sl();
    changedsl();
}

extern int rc;

void clickat(long byte,long bit,int x, int y)
{
    movecsr(x-1,y-1);
    if(byte & bit) {
        curatr=31;
        outs("û");
    } 
    else {
        curatr=15;
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
            curatr=31;
            outs("û");
        } 
        else {
            curatr=15;
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
    curatr=15;
    return returncode;
}

void val_cur_user(int wait)
{
    char sl[4],dsl[4],ar[17],dar[17],restrict[17],ex[17],delff[3];
    char dk[34],uk[34],ndl[33],nul[33],fpts[34],psts[5],lgns[5],timebank[5];
    int cp,i,done;
    char nsave[41];


    strcpy(nsave,thisuser.name);

    if(wait) pr_wait(1);
    thisuser.sl=actsl;
    savescreen(&screensave);
    curatr=11;
    ansic(3);
    fastscreen("top.bin");
    itoa((int)thisuser.sl,sl,10);
    itoa((int)thisuser.dsl,dsl,10);
    itoa((int)thisuser.dk,dk,10);
    itoa((int)thisuser.uk,uk,10);
    itoa((int)thisuser.downloaded,ndl,10);
    itoa((int)thisuser.uploaded,nul,10);
    itoa((int)thisuser.fpts,fpts,10);
    itoa((int)thisuser.msgpost,psts,10);
    itoa((int)thisuser.logons,lgns,10);
    itoa((int)thisuser.timebank,timebank,10);
    strcpy(ex,"");

    for (i=0; i<=15; i++) {
        if (thisuser.ar & (1 << i))
            ar[i]='A'+i;
        else
            ar[i]=32;
        if (thisuser.dar & (1 << i))
            dar[i]='A'+i;
        else
            dar[i]=32;
        if (thisuser.restrict & (1 << i))
            restrict[i]=restrict_string[i];
        else
            restrict[i]=32;
    }
    dar[16]=0;
    ar[16]=0;
    restrict[16]=0;

    cp=1;
    done=0;

    curatr=15;

    outsat(thisuser.name,16,2);
    outsat(thisuser.realname,53,2);
    outsat(sl,16,3);
    outsat(dsl,53,3);
    outsat(ar,16,4);
    outsat(dar,53,4);
    outsat(thisuser.phone,16,5);
    outsat(restrict,53,5);
    outsat(nul,16,6);
    outsat(ndl,53,6);
    outsat(uk,16,7);
    outsat(dk,53,7);
    outsat(lgns,16,8);
    outsat(psts,53,8);
    outsat(fpts,16,9);
    outsat(timebank,53,9);

    outsat(thisuser.city,16,15);
    outsat(thisuser.street,16,16);
    outsat(thisuser.comment,16,17);
    outsat(thisuser.note,16,18);

    clickat(thisuser.inact,inact_deleted,25,11);
    clickat(thisuser.inact,inact_deleted,41,11);

    clickat(thisuser.sysstatus,sysstatus_ansi,22,12);
    clickat(thisuser.sysstatus,sysstatus_avatar,34,12);
    clickat(thisuser.sysstatus,sysstatus_color,45,12);
    clickat(thisuser.sysstatus,sysstatus_rip,54,12);

    clickat(thisuser.exempt,exempt_time,22,13);
    clickat(thisuser.exempt,exempt_ratio,32,13);
    clickat(thisuser.exempt,exempt_post,41,13);
    clickat(thisuser.exempt,exempt_userlist,54,13);

    while (done==0) {
        switch(cp) {
        case 1: 
            editdata(thisuser.name,25,16,2);
            strupr(thisuser.name);
            break;
        case 2: 
            editdata(thisuser.realname,25,53,2);
            break;
        case 3: 
            thisuser.sl=editdig(sl,3,16,3);
            actsl=thisuser.sl; 
            break;
        case 4: 
            thisuser.dsl=editdig(dsl,3,53,3);
            break;
        case 5: 
            movecsr(16-1,4-1);
            editline(ar,16,SET,&rc,"ABCDEFGHIJKLMNOP");
            thisuser.ar=0;
            for (i=0; i<=15; i++)
                if (ar[i]!=' ') thisuser.ar |= (1 << i);
            break;
        case 6: 
            movecsr(53-1,4-1);
            editline(dar,16,SET,&rc,"ABCDEFGHIJKLMNOP");
            thisuser.dar=0;
            for (i=0; i<=15; i++)
                if (dar[i]!=' ') thisuser.dar |= (1 << i);
            break;
        case 7:
            editdata(thisuser.phone,12,16,5);
            break;
        case 8: 
            movecsr(53-1,5-1);
            editline(restrict,16,SET,&rc,restrict_string);
            thisuser.restrict=0;
            for (i=0; i<=15; i++)
                if (restrict[i]!=' ') thisuser.restrict |= (1 << i);
            break;
        case 9: 
            thisuser.uploaded=editdig(nul,5,16,6);
            break;
        case 10: 
            thisuser.downloaded=editdig(ndl,5,53,6);
            break;
        case 11: 
            thisuser.uk=editdig(uk,5,16,7);
            break;
        case 12: 
            thisuser.dk=editdig(dk,5,53,7);
            break;
        case 13: 
            thisuser.logons=editdig(lgns,5,16,8);
            break;
        case 14: 
            thisuser.msgpost=editdig(psts,5,53,8);
            break;
        case 15: 
            thisuser.fpts=editdig(fpts,5,16,9);
            break;
        case 16: 
            thisuser.timebank=editdig(timebank,3,53,9);
            break;

        case 17:
            rc=click((long *)&thisuser.inact,inact_deleted,25,11);
            break;
        case 18:
            rc=click((long *)&thisuser.inact,inact_deleted,41,11);
            break;

        case 19:
            rc=click((long *)&thisuser.sysstatus,sysstatus_ansi,22,12);
            break;
        case 20:
            rc=click((long *)&thisuser.sysstatus,sysstatus_avatar,34,12);
            break;
        case 21:
            rc=click((long *)&thisuser.sysstatus,sysstatus_color,45,12);
            break;
        case 22:
            rc=click((long *)&thisuser.sysstatus,sysstatus_rip,54,12);
            break;


        case 23:
            rc=click((long *)&thisuser.exempt,exempt_time,22,13);
            break;
        case 24:
            rc=click((long *)&thisuser.exempt,exempt_ratio,32,13);
            break;
        case 25:
            rc=click((long *)&thisuser.exempt,exempt_post,41,13);
            break;
        case 26:
            rc=click((long *)&thisuser.exempt,exempt_userlist,54,13);
            break;

        case 27:
            editdata(thisuser.city,53,16,15);
            break;
        case 28:
            editdata(thisuser.street,53,16,16);
            break;
        case 29:
            editdata(thisuser.comment,53,16,17);
            break;
        case 30:
            editdata(thisuser.note,53,16,18);
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

    restorescreen(&screensave);
    changedsl();

    if(strcmp(nsave,thisuser.name))
        reset_files(0);

    if(wait) pr_wait(0);
}


