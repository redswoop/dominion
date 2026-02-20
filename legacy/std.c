#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#pragma hdstop

#include "std.h"

int coltable[]={15,7,13,11,14,9,31,12,12+128,10};
int coltable2[]={143,8,5,3,6,1,128+14,4,128+13,2};

char col=0;

void addto(char *s, int i)
{
    char temp[20];

    if (s[0])
        strcat(s, ";");
    else
        strcpy(s, "\x1B[");
    itoa(i, temp, 10);
    strcat(s, temp);
}

void makeansi(unsigned char attr, char *s)
{
    unsigned char catr;
    char *temp = "04261537";

    catr = 0;
    s[0] = 0;


    if (attr != catr) {
        if ((catr & 0x88) ^ (attr & 0x88)) {
            addto(s, 0);
            addto(s, 30 + temp[attr & 0x07] - '0');
            addto(s, 40 + temp[(attr & 0x70) >> 4] - '0');
            catr = attr & 0x77;
        }
        if ((catr & 0x07) != (attr & 0x07))
            addto(s, 30 + temp[attr & 0x07] - '0');
        if ((catr & 0x70) != (attr & 0x70))
            addto(s, 40 + temp[(attr & 0x70) >> 4] - '0');
        if ((catr & 0x08) ^ (attr & 0x08))
            addto(s, 1);
        if ((catr & 0x80) ^ (attr & 0x80))
            addto(s, 5);
    }
    if (s[0])
        strcat(s, "m");
}


void outchr(unsigned char c)
{
    char s[15];

    if(col==3) {
        col=0;
        makeansi(c,s);
        printf(s);
        return;
    }

    if(col==1) {
        col=0;
        makeansi(coltable[c-'0'],s);
        outstr(s);
         return;
    }

    if(col==2) {
        col=0;
        makeansi(coltable2[c-'0'],s);
        outstr(s);
        return;
    }

    if(c==3) {
        col=1;
        return; 
    }

    if(c==14) {
        col=2; 
        return; 
    }

    if(c=='') {
        col=3; 
        return; 
    }

    if(c==12) { 
        clrscr(); 
        return; 
    }

    printf("%c",c);
}

void SetAttribute(unsigned char c)
{
    char *s;
    makeansi(c,s);
    printf(s);
}

void outstr(char *s)
{
    int x=0;
    while(s[x]) outchr(s[x++]);
}


void nl(void)
{
    printf("\n");
}


void npr(char *fmt, ...)
{
    va_list ap;
    char s[512];

    va_start(ap, fmt);
    vsprintf(s, fmt, ap);
    va_end(ap);
    outstr(s);
    nl();
}

void pl(char *s)
{
    outstr(s);
    nl();
}

void pr(char *fmt, ...)
{
    va_list ap;
    char s[512];

    va_start(ap, fmt);
    vsprintf(s, fmt, ap);
    va_end(ap);
    outstr(s);
}

void pla(char *s)
{
    int i;

    i=0;

    if(strlen(s)<1) return;
    while (s[i]&!kbhit()) outchr(s[i++]);

    if((strstr(s,";"))==NULL) nl();
}

void input1(char *s, int maxlen, int lc, int crend)
{
    int curpos=0, done=0;
    unsigned char ch,s1[20];

    while (!done) {
        ch = getch();
        if (ch > 31) {
            if (curpos < maxlen) {
                if (!lc)
                    ch = toupper(ch);
                s[curpos++] = ch;
                outchr(ch);
            }
        } 
        else
            switch(ch) {
        case 16: 
            outchr(3);
            s[curpos++] = 3; 
            break;
        case 5: 
            outchr(14);
            s[curpos++] = 14; 
            break;
        case 14:
        case 13:
            s[curpos] = 0;
            done =  1;
            break;
        case 8:
            if (curpos) {
                curpos--;
                outstr("\b \b");
                if (s[curpos] == 26)
                    outstr("\b \b");
            }
            break;
        case 21:
        case 24:
            while (curpos) {
                curpos--;

                outstr("\b \b");
                if (s[curpos] == 26)

                    outstr("\b \b");
            }
            break;
        }
    }
    if(crend)
        nl();
}

void input(char *s,int len)
{
   input1(s,len,1,1);
}

int strlenc(char *s)
{
    int len=0,x=0;
    while(s[x]) {
        if(s[x]!=3&&s[x]!=14) {
            len++; 
            x++;
        }
        else if(s[x]==3||s[x]==14) x+=2;
    }
    return len;
}


char onek(char *s)
{
    char ch;

    while(!strchr(s, ch=toupper(getch())) );
    outchr(ch);
    nl();
    return(ch);
}

void pausescr()
{
    outstr("Press Any Key to Continue");
    getch();
    nl();
}

int ny() {
    char c;

    c=onek("YN\r");
    if(c=='N') return 0;
    if(c=='Y'||c=='\r') return 1;
    return 1;
}

int yn() {
    char c;

    c=onek("YN\r");
    if(c=='Y') return 1;
    if(c=='N'||c=='\r') return 0;
    return 1;
}

void InitText()
{
}

void EndText()
{
}

void Gotoxy(int x,int y)
{
    gotoxy(x,y);
}

void Cls()
{
    clrscr();
}

void inputdat(char msg[81],char *s, int len)
{
    pr("3%s: 3",msg);
    input(s,len);
}

void inputq(char msg[81],int *s)
{
    if(s);

    pr("3%s3 ",msg);
    s=yn();
}

void filter(char *s,unsigned char c)
{
    int x=0;

    while(s[x++]!=c);
    s[x-1]=0;

}
 
