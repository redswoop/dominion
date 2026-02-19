#include "vars.h"
#pragma hdrstop
#include <dir.h>

long getfree(int drv)
{
    struct dfree d;
    getdfree(drv,&d);
    return((long)d.df_avail*(long)d.df_sclus*(long)d.df_bsec);
}

void deletefile(char *fn)
{
    struct ffblk ff;
    int i;
    char s[81],s1[81],*b;
    char d[10],p[MAXPATH],n[8],ext[4];

    b=strtok(fn," ");
    b=strtok(NULL,"");
    strcpy(s,b);

    fnsplit(s,d,p,n,ext);
    i = findfirst(s,&ff,0);
    while(!i) {
        sprintf(s1,"%s%s%s",d,p,ff.ff_name);
        npr("Want to Delete %s",s1);
        if(yn())
            unlink(s1);
        i=findnext(&ff);
    }
}

void dodir(char spec[81])
{
  struct ffblk ff;
  long avail,total=0;
  char s[41],s1[81],s2[41],ch;
  int done=0,i,i3=0,i4=0,lines,numf=0;
  char *p;


   p=strtok(spec," ");
   p=strtok(NULL," ");
   if(!p) strcpy(spec,"*.*");
   else strcpy(spec,p);

   if(spec[strlen(spec)-1]=='\\') strcat(spec,"*.*");
   else {
        strcpy(s,spec);
        strcat(s,"\\nul");
        if(exist(s)) strcat(spec,"\\*.*");
    }
    nl();
    strcpy(spec,strupr(spec));

   npr("2Directory of 3%s\r\n0",spec);
   done = findfirst(spec,&ff,FA_DIREC|FA_ARCH|FA_HIDDEN|FA_SYSTEM|FA_RDONLY);
   npr("5อออออออออออออออออออัอออออออออออออออออออัอออออออออออออออออออัอออออออออออออออออออ");
   nl();
   do {
      total+=ff.ff_fsize;
      numf++;
      sprintf(s,"0%-12.12s7",ff.ff_name);
      if(ff.ff_attrib & FA_DIREC)
      sprintf(s1," 1<Dir> ");
      else {
          if(ff.ff_fsize>=1024000)
              sprintf(s1,"%6dM",ff.ff_fsize/1024000);
          else sprintf(s1,"%7ld",ff.ff_fsize);
      }
      strcat(s,s1);
      outstr(s);
      i3++;
      if(i3==4) {
         nl();
         i3=0;
         i4++;
         if(i4==20) pausescr();
      } else outstr("5ณ");
      done = findnext(&ff);
      if(!done) lines++;
   } while(!done);
   ansic(5);
   for(i4=i3;i4<4;i4++) {
        for(i=0;i<19;i++) outchr(32);
        npr("%c",i4==4?0:179);
   }
  nl();
  npr("5อออออออออออออออออออฯอออออออออออออออออออฯอออออออออออออออออออฯอออออออออออออออออออ");
  npr("\r\n\t3%ld0 Bytes Used By 3%d0 Files\r\n",total,numf);
  nl();
}

void copyfile(char *fn)
{
  int i,i2,d1,d2;
  char *b,s[81],s1[MAXPATH],s2[81];

  strcpy(s,"");

  b=strtok(fn," ");
  b=strtok(NULL," ");
  strcpy(s,b);
  b=strtok(NULL," ");
  if(b==NULL) strcpy(s2,stripfn(s));
  else
  strcpy(s2,b);

  sprintf(s1,"%s\\nul",s2);
  if(exist(s1)) {
    sprintf(s1,"%s\\%s",s2,s);
    strcpy(s2,s1);
  }

  if(strcmp(s2,s)==0) {
    pl("File Cannot Overwrite itself");
    return;
  }

  if(!exist(s)) {
    npr("File %s not Found",s);
    return;
  }

  if ((b=malloca(16400))==NULL) {
    pl("No Memory");
    return;
  }

  npr("0%s 7=> 0%s\r\n",s,s2);
  d1=open(s,O_RDONLY | O_BINARY);
  d2=open(s2,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
  i=read(d1,(void *)b,16384);
  while (i>0) {
     write(d2,(void *)b,i);
     i=read(d1,(void *)b,16384);
  }
  close(d1);
  close(d2);
  farfree(b);
}


void changedir(char *path)
{
    char s[81],*p,c;

    p=strtok(path," ");
    p=strtok(NULL,"");
    strcpy(s,p);

    if(strchr(s,':')) {
      c=s[0]-'A';
      setdisk(c);
    }

    if(chdir(p)) npr("4Error Changing to %s",p);
}

void minidos(void)
{
  char s[161],s1[81],s2[81],c,cudir[81],prompt[81];
  int i,done=0;
  struct dfree free;
  long avail;

  outchr(12);
  pl("2Dominous MiniDos 5v1.50 - Your Dos Shell and Your Friend.");
  nl();

  cd_to(cdir);
  strcpy(prompt,"0%17>1");
  okskey=1;

  do {
    strcpy(s2,"");
    strcpy(s,"");
    strcpy(s1,"");
    getcwd(cudir,51);
    stuff_in(s,prompt,cudir,"`T","`N","`M","");
    outstr(s);
    input1(s,81,1,1);
    strcpy(s2,strupr(s));
    if(!strcmp(s2,"EXIT")) done=1;
    else
    if(strstr(s2,"CD")) changedir(s2);
    if(!strcmp(s2,"CLS")) outchr(12);
    else if(strstr(s2,"DEL"))
          deletefile(s2);
    else if(strstr(s2,"COPY"))
           copyfile(s2);
    else if(strstr(s2,"DIR"))
           dodir(s2);
    else if(strstr(s2,"EXT "))
           ex("D1",s2+4);
    else if(!strcmp(s2,"HELP"))
           printmenu(20);
    else if(strstr(s2,"TYPE"))
           viewfile(s2);
    else if(strstr(s2,"FREE")) {
        npr("Free Space for Drive %c : %ld\r\n",getdisk()+'A',getfree(0));
        }
    else if(strstr(s2,"PROMPT")) {
           pl("Enter Prompt: ");
           inli(prompt,"",81,1);
        }
  } while(!done&&!hangup);

  cd_to(cdir);
}

//void minidos(void){}
