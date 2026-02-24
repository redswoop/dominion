#include "bbsutl2.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "conio.h"
#include "file1.h"
#include "utility.h"
#include "config.h"
#include "stringed.h"
#include "session.h"
#include "system.h"
#include "version.h"
#include "sysopf.h"
#include "lilo.h"
#pragma hdrstop


int begxx,begyy;
/* sess.menuat now in vars.h (Phase B0) */

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


