#include "vars.h"

directoryrec dir;
configrec syscfg;
int numf;

#define SETREC(i)  lseek(dlf,((long) (i))*((long)sizeof(uploadsrec)),SEEK_SET);
int dlf;



char *date()
{
  static char s[9];
  struct date today;

  getdate(&today);
  sprintf(s,"%02d/%02d/%02d",today.da_mon,today.da_day,today.da_year-1900);
  return(s);
}

void dliscan1(int dn)
{
  char s[81];
  int i;
  uploadsrec u;

  closedl();
  sprintf(s,"%sdir\\%s.DIR",syscfg.datadir,dir.filename);
  dlf=open(s,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
  i=filelength(dlf)/sizeof(uploadsrec);
  if (i==0) {
    u.numbytes=0;
    SETREC(0);
    write(dlf,(void *)&u,sizeof(uploadsrec));
  } else {
    SETREC(0);
    read(dlf,(void *)&u,sizeof(uploadsrec));
  }
  numf=u.numbytes;
}




void closedl()
{
  if (dlf>0) {
    close(dlf);
    dlf=-1;
  }
}




void align(char *s)
{
  char f[40],e[40],s1[20],*s2;
  int i,i1,i2;

  i1=0;
  if (s[0]=='.')
    i1=1;
  for (i=0; i<strlen(s); i++)
    if ((s[i]=='\\') || (s[i]=='/') || (s[i]==':') || (s[i]=='<') ||
      (s[i]=='>') || (s[i]=='|'))
      i1=1;
  if (i1) {
    strcpy(s,"        .   ");
    return;
  }
  s2=strchr(s,'.');
  if (s2==NULL) {
    e[0]=0;
  } else {
    strcpy(e,&(s2[1]));
    e[3]=0;
    s2[0]=0;
  }
  strcpy(f,s);

  for (i=strlen(f); i<8; i++)
    f[i]=32;
  f[8]=0;
  i1=0;
  i2=0;
  for (i=0; i<8; i++) {
    if (f[i]=='*')
      i1=1;
    if (f[i]==' ')
      i2=1;
    if (i2)
      f[i]=' ';
    if (i1)
      f[i]='?';
  }

  for (i=strlen(e); i<3; i++)
    e[i]=32;
  e[3]=0;
  i1=0;
  for (i=0; i<3; i++) {
    if (e[i]=='*')
      i1=1;
    if (i1)
      e[i]='?';
  }

  for (i=0; i<12; i++)
    s1[i]=32;
  strcpy(s1,f);
  s1[8]='.';
  strcpy(&(s1[9]),e);
  strcpy(s,s1);
  for (i=0; i<12; i++)
    s[i]=toupper(s[i]);
}

int upload_file2(char *fn, int dn, char *desc)
{
  directoryrec d;
  uploadsrec u,u1;
  int i,i1,i2,ok,f;
  char s[81],s1[81],ff[81];
  long l;
  double ti;

  d=dir;
  strcpy(s,fn);
  align(s);
  strcpy(u.filename,s);
  u.ownerusr=1;
  u.ownersys=0;
  u.numdloads=0;
  u.filetype=0;
  u.mask=0;
  {
    sprintf(ff,"%s%s",d.dpath,s);
    f=open(ff,O_RDONLY | O_BINARY);
    if (f<=0) {
      if (desc && (*desc)) {
        npr("ERR: %s: %s\r\n",fn,desc);
      } else {
        npr("File '%s' doesn't exist.\r\n",fn);
      }
      return(1);
    }
    l=filelength(f);
    u.numbytes=l;
    close(f);
    strcpy(u.upby,"SysOp");
    strcpy(u.date,date());
    u.points=((u.numbytes)+1023)/10240;
    pr("3%s: %4ldk :",u.filename,((u.numbytes)+1023)/1024);
    if ((desc) && (*desc)) {
      strncpy(u.description,desc,39);
      u.description[39]=0;
      pl(u.description);
    } else
      input(u.description,39);
    if (u.description[0]==0)
      return(0);
    time(&l);
    u.daten=l;
    for (i=numf; i>=1; i--) {
      SETREC(i);
      read(dlf,(void *)&u1,sizeof(uploadsrec));
      SETREC(i+1);
      write(dlf,(void *)&u1,sizeof(uploadsrec));
    }
    SETREC(1);
    write(dlf,(void *)&u,sizeof(uploadsrec));
    ++numf;
    SETREC(0);
    read(dlf, &u1, sizeof(uploadsrec));
    u1.numbytes=numf;
    u1.daten=l;
    SETREC(0);
    write(dlf,(void *)&u1,sizeof(uploadsrec));
  }
  return(1);
}

int maybe_upload(char *fn, int dn, char *desc)
{
  char s[81];
  int i,i1=0,ok=1,ocd;
  uploadsrec u;

  strcpy(s,fn);
  align(s);

    if (!upload_file2(s,dn,desc))
      ok=0;
  return(ok);
}

void upload_files(char *fn, int dn, int type)
/* This assumes the file holds listings of files, one per line, to be
 * uploaded.  The first word (delimited by space/tab) must be the filename.
 * after the filename are optional tab/space separated words (such as file
 * size or date/time).  After the optional words is the description, which
 * is from that position to the end of the line.  the "type" parameter gives
 * the number of optional words between the filename and description.
 * the optional words (size, date/time) are ignored completely.
 */
{
  char s[255],*fn1,*desc;
  FILE *f;
  int ok=1,abort=0,next=0,ok1,i;

  dliscan1(dn);

  f=fopen(fn,"r");
  if (!f) {
    npr("\r\nFile '%s' not found.\r\n\r\n",fn);
  } else {
    while (ok && fgets(s,250,f)) {
      if ((s[0]<=32) || (s[0]>127))
        continue;
      ok1=0;
      fn1=strtok(s," \t\n");
      if (fn1) {
        ok1=1;
        for (i=0; ok1 && (i<type); i++)
          if (strtok(NULL," \t\n")==NULL)
            ok1=0;
        if (ok1) {
          desc=strtok(NULL,"\n");
          if (!desc)
            ok1=0;
        }
      }
      if (ok1) {
        while ((*desc==' ') || (*desc=='\t'))
          ++desc;
        ok=maybe_upload(fn1,dn,desc);
        if (abort)
          ok=0;
      }
    }
    fclose(f);
  }

  closedl();
}

char *VERSION={"0Files2Dom v31.1"};

void title(void)
{
    npr("%s 0- Copyright 1993",VERSION);
}

void main(int ac,char **av)
{
    int i;
    char s[161],c;

    if(ac!=3) {
        nl();
        title();
        npr("5Usage: 3File2dom <File Area to Convert> <Filename to Convert>");
        exit(1);
    }

    i=open("config.dat",O_BINARY|O_RDWR);
    if(i<0) {
        npr("7Config.dat not found!  Exiting!");
        nl();
        exit(2);
    }
    read(i,&syscfg,sizeof(syscfg));
    close(i);

    sprintf(s,"%sdirs.dat",syscfg.datadir);
    i=open(s,O_BINARY|O_RDWR);
    lseek(i,sizeof(directoryrec)*atoi(av[1]),SEEK_SET);
    read(i,&dir,sizeof(dir));
    close(i);

    nl();
    pl("1. RBBS format - <filename> <size> <date> <description>");
    pl("2. QBBS format - <filename> <description>");
    nl();
    outstr("5Which format (1,2,Q) ? 0");
    c=onek("Q12");
    nl();
    if (c!='Q') {
      switch(c) {
        case '1': i=2; break;
        case '2': i=0; break;
        default : i=0; break;
      }
      upload_files(av[2],atoi(av[1]),i);
    }
}
