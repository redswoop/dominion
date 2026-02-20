#include "vars.h"
#pragma hdrstop


#define ALLOW_FULLSCREEN 1
#define EMAIL_STORAGE 2
#define MAX_LIST 40

void multimail(mmailrec un[40], int numu)
{
  int i,i1,f,ff,len,an,cv,ok;
  mailrec m,m1;
  char s[MAX_PATH_LEN],t[MAX_PATH_LEN];


  if (freek1(syscfg.msgsdir)<10.0) {
    nl();
    pl("Sorry, not enough disk space left.");
    nl();
    return;
  }
  nl();

  m.msg.storage_type=EMAIL_STORAGE;
  upload_post();
  if (syscfg.sl[actsl].ability & ability_email_anony)
    i=anony_enable_anony;
  else
    i=0;
  if ((i==anony_enable_anony) && (thisuser.restrict & restrict_anony))
    i=0;

  inmsg(&m.msg,t,&i,1,"EMAIL",ALLOW_FULLSCREEN);
  if (m.msg.stored_as==0xffffffff)
    return;
  for (cv=0; cv<numu; cv++)
    sendout_email(t,&m.msg,i,un[cv].un,un[cv].sy,1);

  close_user();
  save_status();
  if (!wfc) topscreen();
}


char *mml_s;
int mml_started;

mmailrec oneuser(int which)
{
  char s[MAX_PATH_LEN],*ss;
  unsigned short un,i,sy;
  userrec u;
  mmailrec mmail;

  if (mml_s) {
    if (mml_started)
      ss=strtok(NULL,"\r\n");
    else
      ss=strtok(mml_s,"\r\n");
    mml_started=1;
    if (ss==NULL) {
      farfree(mml_s);
      mml_s=NULL;
      mmail.un=-1; return(mmail);
    }
    strcpy(s,ss);
    for (i=0; s[i]!=0; i++)
      s[i]=toupper(s[i]);
  } else {
    npr("3Enter User to Send To\r\n5%d: ",which);
    mpl(40);
    input(s,40);
  }
  if(!s[0]) {
    mmail.un=-1; return(mmail);
  }

  parse_email_info(s,&un);


  if (un==0) {
    nl();
    pl("Unknown user.");
    nl();
    mmail.un=0; return(mmail);
  }

  if(!sy) {
      read_user(un,&u);
      if (((u.sl==255) && (u.waiting>(syscfg.maxwaiting * 5))) ||
          ((u.sl!=255) && (u.waiting>syscfg.maxwaiting)) ||
          (u.waiting>200)) {
        nl();
        pl("Mailbox full.");
        nl();
        mmail.un=0; return(mmail);
      }
      if (u.inact & inact_deleted) {
        nl();
        pl("Deleted user.");
        nl();
        mmail.un=0; return(mmail);
      }
      npr("     -> %s\r\n",nam(&u,un));
      mmail.sy=0;
  } else {
      mmail.sy=sy;
      npr("     -> User %d, Node %d\r\n",un,sy);
  }
  mmail.un=un;

  return(mmail);
}


void add_list(mmailrec *un, int *numu, int maxu, int allowdup)
{
  int i,i1,i2, mml,done=0;
  mmailrec mmail;

  mml=(mml_s!=NULL);
  mml_started=0;
  while ((!done) && (!hangup) && (*numu<maxu)) {
    mmail=oneuser(*numu+1);
    if (mml && (!mml_s))
      done=1;
    if (mmail.un==-1)
      done=1;
    else
      if (mmail.un) {
        if (!allowdup)
          for (i1=0; i1<*numu; i1++)
            if (un[i1].un==mmail.un) {
              nl();
              pl("Already in list, not added.");
              nl();
              mmail.un=0;
            }
        if (mmail.un) {
          un[(*numu)].un=mmail.un;
          un[(*numu)++].sy=mmail.sy;
        }
      }
  }
  if (*numu==maxu) {
    nl();
    pl("List full.");
    nl();
  }
}



void maillist()
{
  int numu,i,i1,f1;
  mmailrec un[MAX_LIST];
  char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],ch,*sss;
  slrec ss;
  userrec u;
  struct ffblk ff;

  mml_s=NULL;
  mml_started=0;
  ss=syscfg.sl[thisuser.sl];
  if (freek1(syscfg.msgsdir)<10.0) {
    nl();
    pl("Sorry, not enough disk space left.");
    nl();
    return;
  }
  if (((fsenttoday>=5) || (thisuser.fsenttoday1>=10) ||
    (thisuser.etoday>=ss.emails)) && (!cs())) {
    pl("Too much mail sent today.");
    return;
  }
  if (restrict_email & thisuser.restrict) {
    pl("You can't send mail.");
    pausescr();
    return;
  }
  numu=0;
  nl();
  nl();
  sprintf(s,"%s*.ml",syscfg.datadir);
  f1=findfirst(s,&ff,0);
  if (f1) {
    nl();
    pl("No mailing lists available.");
    nl();
    pausescr();
    return;
  }

  do {
      nl();
      pl("Available mailing lists:");
      nl();
      sprintf(s,"%s*.mml",syscfg.datadir);
      f1=findfirst(s,&ff,0);

      while (f1==0) {
        strcpy(s,ff.ff_name);
        sss=strchr(s,'.');
        if (sss)
          *sss=0;
        pl(s);

        f1=findnext(&ff);
      }

      nl();
      prt(2,"Which? ");
      input(s,8);

      sprintf(s1,"%s%s.mml",syscfg.datadir,s);
      i=open(s1,O_RDONLY | O_BINARY);

      if (i<0&&s[0]) {
        nl();
        pl("Unknown mailing list.");
        nl();
      } else {
        i1=filelength(i);
        mml_s=malloca(i1+10L);
        read(i,mml_s,i1);
        mml_s[i1]='\n';
        mml_s[i1+1]=0;
        close(i);
        mml_started=0;
        add_list(un,&numu,MAX_LIST,so());
        if (mml_s) {
          farfree(mml_s);
          mml_s=NULL;
        }
      }
  } while(s[0]&&!hangup);

  multimail(un,numu);
}

/* smail() removed — defined in jam.c (was duplicate) */

void nmail(int unn,int syy)
{
  int numu,done,i,i1,f1;
  mmailrec un[MAX_LIST];
  char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],ch,*sss;
  slrec ss;
  userrec u;
  struct ffblk ff;

  mml_s=NULL;
  mml_started=0;
  ss=syscfg.sl[thisuser.sl];
  if (freek1(syscfg.msgsdir)<10.0) {
    nl();
    pl("Sorry, not enough disk space left.");
    nl();
    return;
  }
  if (((fsenttoday>=5) || (thisuser.fsenttoday1>=10) ||
    (thisuser.etoday>=ss.emails)) && (!cs())) {
    pl("Too much mail sent today.");
    return;
  }
  if (restrict_email & thisuser.restrict) {
    pl("You can't send mail.");
    return;
  }
  done=0;
  numu=0;

  if(unn) {
    un[0].un=unn;
    un[0].sy=syy;
    numu=1;
    multimail(un,numu);
    return;
  }

  pl("Enter user Name(s)/Number(s)");
  nl();
  mml_s=NULL;
  add_list(un,&numu,MAX_LIST,so());
  if(numu>1) {
    do {
    nl();
    outstr("0[Enter] to Send, <A>dd More, <D>elete, <L>ist 1: ");
    ch=onek("\rADLQ");
    switch(ch) {
        case 'Q': done=1; break;
        case 13 : multimail(un,numu); done=1; break;
        case 'A': add_list(un,&numu,MAX_LIST,so()); break;
        case 'D': if(numu) {
                      nl();
                      prt(2,"Delete which? ");
                      input(s,2);
                      i=atoi(s);
                      if ((i>0) && (i<=numu)) {
                        --numu;
                        for (i1=i-1; i1<numu; i1++)
                          un[i1].un=un[i1+1].un;
                      }
                  }
                  break;
        case 'L': for (i=0; i<numu; i++) {
                      if(!un[i].sy) {
                          read_user(un[i].un,&u);
                          npr("%d. %s\r\n",i+1,nam(&u,un[i].un));
                      } else
                          npr("%d. %d@%d\r\n",i+1,un[i].un,un[i].sy);
                  }
                  break;
        }
    } while(!done&&!hangup);
  } else if(numu) multimail(un,numu);
}

