#include "bbsutl.h"
#include "platform.h"
#include "bbs_output.h"
#include "sysoplog.h"
#include "session.h"
#pragma hdrstop


void far *malloca(unsigned long nbytes)
{
    void *buf;

    buf=malloc(nbytes+1);
    if (buf==NULL) {
        nl();
        npr("Not enough memory, needed %ld bytes.\r\n",nbytes);
        nl();
        sysoplog("!!! Ran out of memory !!!");
    }
    return(buf);
}

void stuff_in1(char *s, char *s1, char f1[MAX_PATH_LEN],char f2[MAX_PATH_LEN], char f3[MAX_PATH_LEN],char f4[MAX_PATH_LEN], char f5[MAX_PATH_LEN], char f6[MAX_PATH_LEN], char f7[MAX_PATH_LEN],char f8[MAX_PATH_LEN], char f9[MAX_PATH_LEN], char f0[MAX_PATH_LEN])
{
    int r=0,w=0;

    while (s1[r]!=0) {
        if (s1[r]=='%') {
            ++r;
            s[w]=0;
            switch(s1[r]) {
            case '1': 
                strcat(s,f1); 
                break;
            case '2': 
                strcat(s,f2); 
                break;
            case '3': 
                strcat(s,f3); 
                break;
            case '4': 
                strcat(s,f4); 
                break;
            case '5': 
                strcat(s,f5); 
                break;
            case '6': 
                strcat(s,f6); 
                break;
            case '7': 
                strcat(s,f7); 
                break;
            case '8': 
                strcat(s,f8); 
                break;
            case '9': 
                strcat(s,f9); 
                break;
            case '0': 
                strcat(s,f0); 
                break;
            }
            w=strlen(s);
            r++;
        } 
        else
            s[w++]=s1[r++];
    }
    s[w]=0;
}

void stuff_in(char *s, char *s1, char *f1, char *f2, char *f3, char *f4, char *f5)
{
    int r=0,w=0;

    while (s1[r]!=0) {
        if (s1[r]=='%') {
            ++r;
            s[w]=0;
            switch(s1[r]) {
            case '1': 
                strcat(s,f1); 
                break;
            case '2': 
                strcat(s,f2); 
                break;
            case '3': 
                strcat(s,f3); 
                break;
            case '4': 
                strcat(s,f4); 
                break;
            case '5': 
                strcat(s,f5); 
                break;
            }
            w=strlen(s);
            r++;
        } 
        else
            s[w++]=s1[r++];
    }
    s[w]=0;
}






