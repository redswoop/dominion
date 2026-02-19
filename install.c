#include <stdio.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <dir.h>
#include <alloc.h>
#include <dos.h>
#include "vardec.h"
#include "done.h"

long getfree(int drv)
{
    struct dfree d;
    getdfree(drv,&d);
    return((long)d.df_avail*(long)d.df_sclus*(long)d.df_bsec);
}

#define DREQ 1000000
#define NUMDIRS 8
char *dirs[]={"dls","data","data\\dir","afiles","temp","batch","msgs","scripts","menus"};

int exist(char *s)
{
  int i;
  struct ffblk ff;

  i=findfirst(s,&ff,0);
  if (i)
    return(0);
  else
    return(1);
}

void fastscreen(int x)
{
    char far *screen = (char far *)0xB8000000;

    if(x==0)
        memcpy(screen,IMAGEDATA,4000);
    else if(x==1)
        memcpy(screen,DON,4000);

}

void runcm(char path[81])
{
    char s[161],s1[161];

    nl();

    getcwd(s1,MAXPATH);
    strcpy(s,path);
    s[strlen(s)-1]=0;
    cd_to(s);

    sprintf(s,"mc %sscripts\\",path);
    system(s);
    cd_to(s1);
}

void mkdirs(char path[81])
{
    char s[161];
    int i;

    pr("5þ 0Installing System Directories  6û\b");
    for(i=0;i<NUMDIRS;i++) {
        sprintf(s,"%s%s",path,dirs[i]);
        mkdir(s);
    }
    npr("0û");
}

void installdata(char path[81])
{
    char s[161];

    outstr("5þ 0Decompressing System Files     6û\b");
    rename("domdata.pot","domdata.exe");
    sprintf(s,"domdata -d -o %s>nul",path);
    system(s);
    npr("[6;34H0û");
    unlink("domdata.exe");
}

void configdata(char path[81])
{
    configrec syscfg;
    char s[81];
    FILE *f;

    sprintf(s,"%sconfig.dat",path);
    f=fopen(s,"r+b");
    fread(&syscfg,sizeof(syscfg),1,f);
    nl();
    pr("5úþú 3Enter SysOp Name  :1 ");
    input(s,51);
    if(s[0]) strcpy(syscfg.sysopname,s);
    pr("5úþú 3Enter System Name :1 ");
    input(s,51);
    if(s[0]) strcpy(syscfg.systemname,s);
    pr("5úþú 3Which Comport will be used by the system :1 ");
    input(s,2);
    if(s[0]) syscfg.primaryport=atoi(s);
    fseek(f,0L,0);
    fwrite(&syscfg,sizeof(syscfg),1,f);
    fclose(f);
}

void install(void)
{
    char s[81],s1[81];
    int drv=0;

    Cls();
    npr("3Enter Path to Install to, 0[CR]3 for Current Directory");
    pr("5: 0");
    input(s,75);
    if(!s[0]) { strcpy(s,"."); drv=0; }

    if(s[strlen(s)-1]!='\\') {
        s[strlen(s)+1]=0;
        s[strlen(s)]='\\';
    }
    if(s[1]==':') drv=toupper(s[0])-'A';
    nl();
    pr("5Installing to 0%s5, Correct?0 ",s);
    if(ny()) {
        if(getfree(drv)<DREQ) {
            pl("7Sorry, but you do not have enough diskspace to Install.");
            npr("0Currently, Dominion requires 3%ld0 bytes free to Install",DREQ);
            exit(1);
        }
        sprintf(s1,"%snul",s);
        _setcursortype(0);
        if(!exist(s1))
            mkdir(s);
        mkdirs(s);
        installdata(s);
        configdata(s);
        runcm(s);
        fastscreen(1);
        gotoxy(1,24);
        getch();
        _setcursortype(2);
    }
}

void main(void)
{
    char s[81],*ss;

    fastscreen(0);
    gotoxy(1,20);
    outstr("5Continue With Installation?0 ");
    if(ny())
        install();
    exit(0);
}
