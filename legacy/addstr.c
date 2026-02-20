#include "vars.h"
#pragma hdrstop

configrec syscfg;

typedef struct {
  unsigned char thestring[161];
} stringrec;

int start;

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

void createdat(char *fn)
{
    FILE *ff;
    char s[161],s1[171];
    int f;

    if(!exist(fn)) return;

    f=open("newstr.dat",O_RDWR|O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);

    ff=fopen(fn,"rt");

    fgets(s1,171,ff);
    start=atoi(s1);

    while(fgets(s1,171,ff)!=NULL) {
        filter(s1,'\n');
        write(f,&s1,sizeof(s));
    }

    close(f);
    fclose(ff);
}

void appenddat()
{
    char s[161],s1[171];
    int f,ff;

    sprintf(s,"%sstrings.dat",syscfg.datadir);

    f=open(s,O_RDWR|O_BINARY);

    if(!exist("newstr.dat")) return;
    ff=open("newstr.dat",O_BINARY|O_RDWR);

    lseek(f,start*sizeof(stringrec),0);

    while(read(ff,&s,sizeof(s)))
        write(f,&s,sizeof(s));

    close(f);
    close(ff);
}


void main(int ac,char **ar)
{
    int i;
    char s[81];

    i=open("config.dat",O_BINARY|O_RDWR);
    read(i,&syscfg,sizeof(syscfg));
    close(i);

    createdat(ar[1]);
    npr("3Using 9%s3 to start updating at string 4%d",ar[1],start);
    appenddat();
}
