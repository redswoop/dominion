#include "vars.h"

#pragma hdrstop

#include <dir.h>


void copyupfile(char fn[12],char todir[81],char fdir[81]);
#define SETREC(i)  lseek(dlf,((long) (i))*((long)sizeof(uploadsrec)),SEEK_SET);

void download_temp_arc(char *fn, int xfer)
{
  char s[81],s1[81];
  long numbytes;
  double d;
  int i,f,sent,abort;

  npr("Downloading %s.%s:\r\n\r\n", fn, syscfg.arcs[ARC_NUMBER].extension);
  sprintf(s1,"%s%s.%s",syscfg.tempdir,fn, syscfg.arcs[ARC_NUMBER].extension);
  f=open(s1,O_RDONLY | O_BINARY);
  if (f<0) {
    pl("No such file.");
    nl();
    return;
  }
  numbytes=filelength(f);
  close(f);
  if (numbytes==0L) {
    pl("File has nothing in it.");
    nl();
    return;
  }
  d=((double) (((numbytes)+127)/128)) *
    (1620.0) /
    ((double) (modem_speed));
  if (d<=nsl()) {
    npr("Approx. time: %s\r\n",ctim(d));
    sent=0;
    abort=0;
    sprintf(s,"%s.%s",fn,syscfg.arcs[ARC_NUMBER].extension);
    send_file(s1,&sent,&abort,0);
    if (sent) {
      if (xfer) {
        ++thisuser.downloaded;
        thisuser.dk += ((numbytes+1023)/1024);
        nl();
        nl();
        npr("Your ratio is now: %-6.3f\r\n",ratio());
      }
      sprintf(s1,"Downloaded %ldk of %s",(numbytes+1023)/1024, s);
      sysoplog(s1);
      if (useron)
        topscreen();
    }
  } else {
    nl();
    nl();
    pl("Not enough time left to D/L.");
    nl();
  }
}

void add_arc(char *arc, char *fn)
{
  char s[255], s1[81];

  dtitle("Dominion Demon Tasker - Compressing");
  if(ARC_NUMBER==-1) ARC_NUMBER=0;
  sprintf(s1,"%s.%s",arc, syscfg.arcs[ARC_NUMBER].extension);
  get_arc_cmd(s,s1,2,fn);
  if (s[0]) {
    set_protect(0);
    runprog(s,0);
    topscreen();
    sprintf(s,"Added '%s' to %s",fn, s1);
    sysoplog(s);
  } else
    pl("Sorry, can't add to temp archive.");
}

void add_temp_arc()
{
  char s[81],s1[81],s2[81],c,done;
  int i;

  nl();
  ARC_NUMBER=-1;
  for(i=0;i<4;i++) {
     sprintf(s1,"%sTEMP.%s",syscfg.tempdir,syscfg.arcs[i].extension);
     if(exist(s1)) ARC_NUMBER=i;
  }
  if(ARC_NUMBER==-1) {
    nl();
    do {
        pl("Please Select Format");
        for(i=0;i<4&&syscfg.arcs[i].extension[0];i++)
          npr("4%d2>5%s\r\n",i+1,syscfg.arcs[i].extension);
        nl();
        outstr("Select: ");
        c=onek("1234Q\r");
        switch(c) {
           case '1':
           case '\r': ARC_NUMBER=0; done=1; break;
           case '2':  ARC_NUMBER=1; done=1; break;
           case '3':  ARC_NUMBER=2; done=1; break;
           case '4':  ARC_NUMBER=3; done=1; break;
           case 'Q': return;
        }
    } while(!done);
  }
  npr("3Enter filename to add to temporary archive file\r\n");
/*  mpl(12);
  input(s,12);*/
  inputdat("May contain wildcards",s,12,0);
  if (!okfn(s))
    return;
  if (s[0]==0)
    return;
  if (strchr(s,'.')==NULL)
    strcat(s,".*");
  strcpy(s2,stripfn(s));
  for (i=0; i<strlen(s2); i++)
    if ((s2[i]=='|') || (s2[i]=='>') || (s2[i]=='<') || (s2[i]==';') || (s2[i]==' ') ||
        (s2[i]==':') || (s2[i]=='/') || (s2[i]=='\\'))
      return;
  sprintf(s1,"%s%s",syscfg.tempdir,s2);
  sprintf(s2,"%sTEMP",syscfg.tempdir);
  add_arc(s2, s1);
}

void del_temp()
{
  char s[81],s1[81];
  nl();
/*  npr("3Filename to delete\r\n5: ");
  mpl(12);
  input(s1,12);*/
  inputdat("Files to Delete",s1,12,0);
  if (!okfn(s1))
    return;
  if (s1[0]) {
    if (strchr(s1,'.')==NULL)
      strcat(s1,".*");
      remove_from_temp(s1,syscfg.tempdir,1);
  }
}

void list_temp_dir()
{
  int i,i1,f1,abort=0;
  char s[81],s1[81];
  struct ffblk ff;
  uploadsrec u;

  sprintf(s1,"%s*.*",syscfg.tempdir);
  nl();
  pl("Files in temporary directory:");
  nl();
  i=findfirst(s1,&ff,0);
  while(!i) {
    pla(ff.ff_name,&abort);
    i=findnext(&ff);
  }
  nl();
  if ((!abort) && (!hangup)) {
    sprintf(s,"Free space: %ldk.",(long) freek1(syscfg.tempdir));
    pl(s);
    nl();
  }
}


void temp_extract()
{
  int i,i1,i2,i3,ok,abort,ok1;
  char s[255],s1[255],s2[81],s3[255],s4[129];
  uploadsrec u,u1;

  dliscan();
  dtitle("Extract to temporary directory:");
/*  npr("3Filename\r\n5: ");
  mpl(12);
  input(s,12);*/
  inputdat("Filename",s,12,0);
  if ((!okfn(s)) || (s[0]==0)) {
    closedl();
    return;
  }
  if (strchr(s,'.')==NULL)
    strcat(s,".*");
  align(s);
  i=recno(s);
  ok=1;
  while ((i>0) && (ok) && (!hangup)) {
    SETREC(i);
    read(dlf,(void *)&u,sizeof(uploadsrec));
    sprintf(s2,"%s%s",directories[udir[curdir].subnum].dpath,u.filename);
    get_arc_cmd(s1,s2,1,"");
    sprintf(s2,"%s%s",directories[udir[curdir].subnum].dpath,u.filename);

    if(!s1[0]&&exist(s2)) {
        nl();
        abort=0;
        printinfo(&u,&abort,0);
        nl();
        npr("5Copy file to Temp Directory? ");
        if(yn())
            copyupfile(u.filename,syscfg.tempdir,directories[udir[curdir].subnum].dpath);
    }

    if ((s1[0]) && (exist(s2))) {
      nl();
      nl();
      abort=0;
      printinfo(&u,&abort,0);
      nl();
      cd_to(directories[udir[curdir].subnum].dpath);
      get_dir(s4,1);
      strcat(s4,stripfn(u.filename));
      cd_to(cdir);
      do {
        prt(5,"Extract what (?=list,Q=abort) ? ");
        input(s1,12);
        if (s1[0]==0)
          ok1=0;
        else
          ok1=1;
        if (!okfn(s1))
          ok1=0;
        if (strcmp(s1,"?")==0) {
          list_arc_out(stripfn(u.filename),directories[udir[curdir].subnum].dpath);
          s1[0]=0;
        }
        if (strcmp(s1,"Q")==0) {
          ok=0;
          s1[0]=0;
        }
        i2=0;
        for (i1=0; i1<strlen(s1); i1++)
          if ((s1[i1]=='|') || (s1[i1]=='>') || (s1[i1]=='<') || (s1[i1]==';') || (s1[i1]==' '))
            i2=1;
        if (i2)
          s1[0]=0;
        if (s1[0]) {
          if (strchr(s1,'.')==NULL)
            strcat(s1,".*");
          get_arc_cmd(s3,s4,1,stripfn(s1));
          cd_to(syscfg.tempdir);
          if (!okfn(s1))
            s3[0]=0;
          if (s3[0]) {
            runprog(s3,0);
            sprintf(s2,"Extracted out %s from %s",s1,u.filename);
          } else
            s2[0]=0;
          cd_to(cdir);
          if (s2[0])
            sysoplog(s2);
        }
      } while ((!hangup) && (ok) && (ok1));
    } else
      if (s1[0]) {
        nl();
        pl("That file currently isn't there.");
        nl();
      }
    if (ok)
      i=nrecno(s,i);
  }
  closedl();
}

void list_temp_text()
{
  int i,i1,f1,ok,sent;
  char s[81],s1[81];
  struct ffblk ff;
  uploadsrec u;
  double percent;

  nl();
/*  prt(3,"List what file(s) ?0 ");
  input(s,12);*/
  inputdat("List what Files?",s,12,0);
  if (!okfn(s))
    return;
  if (s[0]) {
    if (strchr(s,'.')==NULL)
      strcat(s,".*");
    sprintf(s1,"%s%s",syscfg.tempdir,stripfn(s));
    f1=findfirst(s1,&ff,0);
    ok=1;
    nl();
    while ((f1==0) && (ok)) {
      sprintf(s,"%s%s",syscfg.tempdir,ff.ff_name);
      nl();
      npr("Listing %s:\r\n",ff.ff_name);
      nl();
      ascii_send(s,&sent,&percent);
      if (sent) {
        sprintf(s,"Temp text D/L '%s'",ff.ff_name);
        sysoplog(s);
      } else {
        sprintf(s,"Temp Tried text D/L '%s' %3.2f%%",ff.ff_name,percent*100.0);
        sysoplog(s);
        ok=0;
      }
      f1=findnext(&ff);
    }
  }
}


void list_temp_arc()
{
  char s1[81],s2[81];

  sprintf(s1,"TEMP.%s",syscfg.arcs[ARC_NUMBER].extension);
  list_arc_out(s1,syscfg.tempdir);
  nl();
}

void copyupfile(char fn[12],char todir[81],char fdir[81])
{   int i,d1,d2;
   char *b,s[81],s1[81];
   long l;

   sprintf(s,"%s%s",fdir,fn);
   sprintf(s1,"%s%s",todir,fn);


   if (exist(s)) {
       d2=0;
       if ((s[1]!=':') && (s1[1]!=':'))
         d2=1;
       if ((s[1]==':') && (s1[1]==':') && (s[0]==s1[0]))
         d2=1;
       if (d2) {
         rename(s,s1);
         unlink(s);
       } else {
           if((b=malloca(25*1024))==NULL) return;
           d1=open(s,O_RDONLY | O_BINARY);
           d2=open(s1,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
           l=filelength(d1);
           i=read(d1,(void *)b,25*1024);
           mpl1(l/(25*1024));
           while (i>0) {
             write(d2,(void *)b,i);
             i=read(d1,(void *)b,25*1024);
             outchr('o');
           }
           close(d1);
           close(d2);

           farfree(b);
       }
   }
}

char *stripfn(char *fn)
{
  static char ofn[15];
  int i,i1;
  char s[81];

  i1=-1;
  for (i=0; i<strlen(fn); i++)
    if ((fn[i]=='\\') || (fn[i]==':') || (fn[i]=='/'))
      i1=i;
  if (i1!=-1)
    strcpy(s,&(fn[i1+1]));
  else
    strcpy(s,fn);
  for (i=0; i<strlen(s); i++)
    if ((s[i]>='A') && (s[i]<='Z'))
      s[i]=s[i]-'A'+'a';
  i=0;
  while (s[i]!=0) {
    if (s[i]==32)
      strcpy(&s[i],&s[i+1]);
    else
      ++i;
  }
  strcpy(ofn,s);
  return(ofn);
}


void stripfn1(char *fn)
{
  int i,i1;
  char s[81],s1[81];

  i1=0;
  for (i=0; i<strlen(fn); i++)
    if ((fn[i]=='\\') || (fn[i]==':') || (fn[i]=='/'))
      i1=i;
  strcpy(s1,fn);
  if (i1) {
    strcpy(s,&(fn[i1+1]));
    s1[i1+1]=0;
  } else {
    strcpy(s,fn);
    s1[0]=0;
  }

  for (i=0; i<strlen(s); i++)
    if ((s[i]>='A') && (s[i]<='Z'))
      s[i]=s[i]-'A'+'a';
  i=0;
  while (s[i]!=0) {
    if (s[i]==32)
      strcpy(&s[i],&s[i+1]);
    else
      ++i;
  }
  strcat(s1,s);
  strcpy(fn,s1);
}


int extern_prot(int pn, char *fn1, int sending)
{
  char s[255],s1[81],s2[81],fn[81],sx1[21],sx2[21],sx3[21];
  int i,i1;

  i=0;
  for (i1=0; i1<81; i1++) {
    i+=proto[pn].description[i1];
    i+=proto[pn].sendfn[i1];
    i+=proto[pn].receivefn[i1];
  }
  if (sending) {
    nl();
    if(pn>-1) strcpy(s1,(proto[pn].sendfn));
  } else {
    nl();
    if (pn>-1) strcpy(s1,(proto[pn].receivefn));
  }
  strcpy(fn,fn1);
  stripfn1(fn);
  ultoa(com_speed,sx1,10);
  ultoa(modem_speed,sx3,10);
  sx2[0]='0'+syscfg.primaryport;
  sx2[1]=0;
  stuff_in(s,s1,sx1,sx2,fn,sx3,"");
  if (s[0]) {
    set_protect(0);
    clrscr();
    printf("[0;37;1;44m[KCurrent user: %s\n\n[0;1m",nam(&thisuser,usernum));
    outs(s);
    outs("\r\n");
    if (incom) {
      i=runprog(s,0);
      topscreen();
      return(i);
    }
  }
  return(-5);
}

int get_batchprotocol(int dl,int *hang)
{
  char ch;
  int i1,prot;


  prot=thisuser.defprot;
  if(!proto[prot].description[0]||prot<0) prot=get_protocol(1);
  if(dl);
  *hang=0;

    top:
    npr(get_string2(12),proto[prot].description);
    nl();
    npr(get_string2(13),proto[prot].description);
    nl();
    npr(get_string2(14),proto[prot].description);
    ch=onek("\rHXAB");
    switch(ch) {
        case 13: return(prot);
        case 'X': return(thisuser.defprot=get_protocol(1));
        case 'A': return(-1);
        case 'B': batchdled(); goto top;
        case 'H': *hang=1; return(prot);
    }
  return(0);
}


int get_protocol(int batch)
{
  char s[81],s1[81],oks[81],s2[81],ch;
  int i,i1,i2,maxprot,done;

  strcpy(oks,"Q?N");
  if(batch) strcat(oks,"B");

  maxprot=0;
  done=0;

  for (i1=0; i1<numextrn; i1++) {
    if(batch||(!batch&&proto[i1].singleok)) {
      ++maxprot;
      sprintf(s1,"%c",proto[i1].key);
      strcat(oks,s1);
    }
  }

  strcpy(s,get_string2(4));
  strcpy(s1,oks);

  do {
    nl();
    prt(2,s);
    ch=onek(s1);
    if (ch=='?') {
      nl();
      dtitle("Dominion Transfer Protocols");
      pl("5<5Q5>0 Abort Transfer(s)");
      pl("5<5N5>0 Next File");
      if(batch)
          pl("5<5B5>0 BiModem");
      for (i1=0; i1<numextrn; i1++) {
        if(batch||(!batch&&proto[i1].singleok))
          npr("5<5%c5>0 %s\r\n",proto[i1].key,(proto[i1].description));
      }
      nl();
    } else
      done=1;
  } while ((!done) && (!hangup));
    if (ch=='Q')
      return(-1);
    if(ch=='B') return(-2);
    if(ch=='N') return(-3);
    if(ch=='A') return(-4);

    for(i=0;i<numextrn;i++)
    if(ch==proto[i].key) return(i);

return(-1);
}

void ascii_send(char *fn, int *sent, double *percent)
{
  char b[2048];
  int i,i1,done,abort,i2,next;
  long pos,max;

  i=open(fn,O_RDONLY | O_BINARY);
  if (i>0) {
    max=filelength(i);
    if (!max)
      max=1;
    i1=read(i,(void *)b,1024);
    pos=0L;
    abort=0;
    while ((i1) && (!hangup) && (!abort)) {
      i2=0;
      while ((!hangup) && (!abort) && (i2<i1)) {
    checkhangup();
    outchr(b[i2++]);
    checka(&abort,&next,0);
      }
      pos += (long) i2;
      checka(&abort,&next,0);
      i1=read(i,(void *)b,1024);
    }
    close(i);
    if (!abort)
      *sent=1;
    else {
      *sent=0;
      thisuser.dk += ((pos+1023L)/1024L);
    }
    *percent=((double) pos)/((double)max);
  } else {
    nl();
    pl("File not found.");
    nl();
    *sent=0;
    *percent=0.0;
  }
}
 
void send_file(char *fn, int *sent, int *abort, char *ft)
{
  int i,i1;
  double percent,t;
  char s[81];

  ft[0]=0;
  i=get_protocol(0);
  percent=0.0;
  switch(i) {
    case -1:
      *sent=0;
      *abort=1;
      break;
    case -3:
      *sent=0;
      *abort=0;
      break;
    case -4:
      *sent=0;
      *abort=0;
      ascii_send(fn,sent,&percent);
      strcpy(ft,"ASCII");
      break;
    case -2: break;
    default:
      i1=extern_prot(i,fn,1);
      strcpy(ft,(proto[i].description));
      *abort=0;
      if (i1==proto[i].ok1)
        *sent=1;
      else
        *sent=0;
      break;
  }

}


void receive_file(char *fn, int *received, char *ft, int okbatch)
{
  int i,i1;
  char s[81];

  ft[0]=0;
  i=get_protocol(okbatch);
  switch(i) {
    case -1:
    case -3:
      *received=0;
      break;
    default:
    i1=extern_prot(i,fn,0);
    strcpy(ft,(proto[i].description));
    if (i1==proto[i].ok1) {
      *received=1;
    } else {
      *received=0;
    }
    break;
  }
}


