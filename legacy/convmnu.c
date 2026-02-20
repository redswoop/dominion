#include "vardec.h"
#include <sys/stat.h>
#include <fcntl.h>


typedef struct {
    char desc[50],
         key[8],
         type[2],
         at,
         line[40],
         ms[80];
    char sl[8];
} oldmenurec;

typedef struct {
    char prompt[100],
         title1[80],
         title2[80],
         altmenu[12],
         res[3],
         fallback[15],
         slneed[10],
         pausefile[10];
     int helplevel;
} oldmmrec;

#define command_hidden 0x0001
#define command_unhidden 0x0002
#define command_title 0x0004
#define command_pulldown 0x0008
#define command_forced 0x0010
#define command_every 0x0020
#define command_default 0x0040

#define menu_3d 0x0001
#define menu_pulldown 0x0002
#define menu_format 0x0004


void fix(char *fn,char *path)
{
    int i,num=0,i1;
    oldmenurec m[50];
    oldmmrec p;
    menurec n[50];
    mmrec o;
    char s[81];

    pr("3Converting File 3%s - ",fn);

    sprintf(s,"%s%s",path,fn);
    i=open(s,O_BINARY|O_RDWR);
    read(i,&p,sizeof(p));
    num=(read(i,&m[0],(50*sizeof(oldmenurec))))/
           sizeof(oldmenurec);

    strcpy(o.prompt,p.prompt);
//    o.prompt[0]=0;
    strcpy(o.title1,p.title1);
    strcpy(o.title2,p.title2);
    strcpy(o.altmenu,p.altmenu);
    strcpy(o.format,"main");
    strcpy(o.slneed,p.slneed);
    o.pausefile[0]=0;
    o.helplevel=0;
    o.columns=p.fallback[3];
    o.boarder=p.fallback[6];
    o.battr=p.fallback[5];
    o.col[0]=p.fallback[0];
    o.col[1]=p.fallback[1];
    o.col[2]=p.fallback[2];
    o.attr=0;
    if(p.fallback[4]) o.attr |= menu_3d;


    for(i1=0;i1<num;i1++) {
        strcpy(n[i1].desc,m[i1].desc);
        strcpy(n[i1].key,m[i1].key);
        strcpy(n[i1].type,m[i1].type);
        strcpy(n[i1].ms,m[i1].ms);
        strcpy(n[i1].line,m[i1].line);
        strcpy(n[i1].sl,m[i1].sl);
        switch(m[i1].at) {
            case 'H': n[i1].attr=command_hidden; break;
            case 'U': n[i1].attr=command_unhidden; break;
            case 'T': n[i1].attr=command_title; break;
            case 'P': n[i1].attr=command_pulldown; break;
            case 'F': n[i1].attr=command_forced; break;
            case 'D': n[i1].attr=command_default; break;
            default: n[i1].attr=0; break;
        }
    }

    lseek(i,0L,0);
    write(i,&o,sizeof(o));
    write(i,&n[0],num*sizeof(n[0]));
    close(i);
    npr("5Done. 3%d 3Commands",num);
}

void main(void)
{
    struct ffblk f;
    int i;
    configrec syscfg;
    char s[81];

    i=open("config.dat",O_RDWR|O_BINARY);
    if(i<0) {
        npr("7Config.dat Not found!");
        exit(1);
    }

    read(i,&syscfg,sizeof(syscfg));
    syscfg.sysconfig ^= sysconfig_flow_control;
    lseek(i,0L,0);
    write(i,&syscfg,sizeof(syscfg));
    close(i);

    sprintf(s,"%s*.mnu",syscfg.menudir);
    i=findfirst(s,&f,0);
    while(!i) {
        fix(f.ff_name,syscfg.menudir);
        i=findnext(&f);
    }
}
