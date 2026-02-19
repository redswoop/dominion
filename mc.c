#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <conio.h>
#include <string.h>
#include "vardec.h"
#include <fcntl.h>
#include <sys\stat.h>
#include <dir.h>

char *VERSION={"Dominion Modem Script Compiler v31.1"};

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

void selectfile(char *fn,char path[81])
{
    struct ffblk ff;
    FILE *f;
    int i,y;
    char s[81],*p,f1[81],ff1[81],fff[81],ffff[81];

    pr("1(1Do you want the modem speaker on?1) ");
    y=yn();
    if(y)
        sprintf(s,"%s*.dms",path);
    else
        sprintf(s,"%s*.dmh",path);
    i=findfirst(s,&ff,0);
    if(i) {
        pl("No Scripts Found,  Exiting.");
        exit(0);
    }
    while(!i) {
        strcpy(s,ff.ff_name);
        fnsplit(s,f1,ff1,fff,ffff);
        pr("3%-8s 5-3 ",fff);
        sprintf(s,"%s%s",path,ff.ff_name);
        f=fopen(s,"rt");
        fgets(s,81,f);
        p=strtok(s,":");
        p=strtok(NULL,"");
        pr(p);
        i=findnext(&ff);
    }
    nl();
    pl("3Compile Which?");
    pr("5:0 ");
    input(s,8);
    if(!s[0]) exit(0);
    if(y)
        sprintf(fn,"%s%s.dms",path,s);
    else
        sprintf(fn,"%s%s.dmh",path,s);
}

void main(int argc,char **argv)
{
    FILE *f,*f1;
    char *p,s[161],s1[161],*p1,s2[161],fileline[161],s3[50],fn[40],path[40];
    resultrec ri[100];
    int done=0,i=0,numres=0,numlines=0,append,i3,i1;
    configrec syscfg;
    modem_info mi;
    unsigned int speed=19200;

    nl();
    npr("%s 5þ0 Copyright 1993 Dominous",VERSION);
    nl();

    fn[0]=s2[0]=path[0]=0;

    for(i=1;i<argc;i++) {
        if(argv[i][strlen(argv[i])-1]=='\\'&&!s2[0]) strcpy(s2,argv[i]);
        else if(argv[i][strlen(argv[i])-1]=='\\'&&!path[0]) strcpy(s,argv[i]);
        else strcpy(fn,argv[i]);
    }

    if(!fn[0]&&!s2[0]) selectfile(fn,"scripts\\");
    if(s2[0]&&!fn[0]) selectfile(fn,s2);

    f=fopen(fn,"rt");

    if(f==NULL) {
        npr("File %s not found!",fn);
        return;
    }

    f=fopen(fn,"rt");
    npr("Compiling Configuration File %s",fn);
    done=0;
    while((fgets(s1,161,f)!=NULL)&&!done) {
        if(s1[0]=='#') continue;
        strcpy(fileline,s1);
        numlines++;
        p=strtok(s1,":");
        strcpy(s,p);
        if(!stricmp(s,"END")) done=1;
        else {
        if(!stricmp(s,"NAME")||!stricmp(s,"SETU")||!stricmp(s,"INIT")
        ||!stricmp(s,"ANSR")||!stricmp(s,"PICK")||!stricmp(s,"HANG")
        ||!stricmp(s,"DIAL")||!stricmp(s,"PICK")||!stricmp(s,"SEPR")||!stricmp(s,"SPD")) {
            p=strtok(NULL,"|");
            if(!stricmp(s,"NAME")) strncpy(mi.name,&p[1],81);
            if(!stricmp(s,"SETU")) strncpy(mi.setu,&p[1],161);
            if(!stricmp(s,"DIAL")) strncpy(mi.dial,&p[1],81);
            if(!stricmp(s,"PICK")) strncpy(mi.pick,&p[1],81);
            if(!stricmp(s,"SEPR")) strncpy(mi.sepr,&p[1],10);
            if(!stricmp(s,"INIT")) strncpy(mi.init,&p[1],161);
            if(!stricmp(s,"ANSR")) strncpy(mi.ansr,&p[1],81);
            if(!stricmp(s,"HANG")) strncpy(mi.hang,&p[1],81);
            if(!stricmp(s,"SPD")) speed=atoi(&p[1]);
        }
        }
    }

    while(fgets(s1,161,f)!=NULL) {
            if(s[0]=='#') continue;
            done=0;
            numlines++;
            strcpy(fileline,s1);
            i=0;

            p1=strtok(s1,"|");

            strcpy(s1,p1);
            for(i1=strlen(s1)-1;i1>=0 && s1[i1]==32;i1--);
                s1[i1+1]=0;

            strcpy(ri[numres].return_code,p1);
            p1=strtok(NULL,"|");

            strcpy(s1,p1);
            for(i1=strlen(s1)-1;i1>=0 && s1[i1]==32;i1--);
                s1[i1+1]=0;

            ri[numres].attr=0;
            ri[numres].mode=0;
            ri[numres].modem_speed=0;
            ri[numres].com_speed=0;

            switch(s1[0]) {
                case '+': ri[numres].attr |= flag_append;
                          strcpy(ri[numres].curspeed,&s1[1]);
                          break;
                case '-': ri[numres].curspeed[0]=0; break;
                default: strcpy(ri[numres].curspeed,s1);
            }


            while(!done) {
                p1=strtok(NULL," ");

                if(!p1[0]) {
                    done=1;
                }

                if(strstr(p1,"NORM")) ri[numres].mode=mode_norm;
                if(strstr(p1,"CON"))  ri[numres].mode=mode_con;
                if(strstr(p1,"RING")) ri[numres].mode=mode_ring;
                if(strstr(p1,"ERR"))  ri[numres].mode=mode_err;
                if(strstr(p1,"RINGING")) ri[numres].mode=mode_ringing;
                if(strstr(p1,"NDT"))  ri[numres].mode=mode_ndt;
                if(strstr(p1,"DIS"))  ri[numres].mode=mode_dis;


                if(strstr(p1,"MS")||strstr(p1,"CS")||strstr(p1,"EC")||strstr(p1,"DC")
                ||strstr(p1,"AS")||strstr(p1,"FC")) {
                    i=0;
                    strcpy(s2,p1);
                    strcpy(s,"");
                    while(s2[i]!='=') s[i]=s2[i++];
                    i++;
                    s[2]=0;
                    strcpy(s1,s2+i);
                    if(!stricmp(s,"MS")) ri[numres].modem_speed=atoi(s1);
                    if(!stricmp(s,"CS")) ri[numres].com_speed=atoi(s1);
                    if(!stricmp(s,"EC")) if(s1[0]=='Y') { ri[numres].attr |= flag_ec; }
                    if(!stricmp(s,"AS")) if(s1[0]=='Y') { ri[numres].attr |= flag_as; }
                    if(!stricmp(s,"DC")) if(s1[0]=='Y') { ri[numres].attr |= flag_dc; }
                    if(!stricmp(s,"FC")) if(s1[0]=='Y') { ri[numres].attr |= flag_fc; }
                }
            }
            numres++;
    }

    fclose(f);

    i=open("config.dat",O_RDWR|O_BINARY);
    read(i,&syscfg,sizeof(syscfg));
    syscfg.baudrate[syscfg.primaryport]=speed;
    lseek(i,0L,0);
    write(i,&syscfg,sizeof(syscfg));
    close(i);

    sprintf(s,"%smodem.dat",syscfg.datadir);
    f=fopen(s,"wb");
    fwrite(&mi,sizeof(mi),1,f);
    fclose(f);

    sprintf(s,"%sresults.dat",syscfg.datadir);
    f=fopen(s,"wb");
    fwrite(&ri[0],sizeof(resultrec)*numres+1,1,f);
    fclose(f);

    npr("0Compiled \"5%s0\", %d lines ",mi.name,numlines);
}
