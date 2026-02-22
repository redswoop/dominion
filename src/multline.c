#include "vars.h"

#pragma hdrstop

#include "json_io.h"

/* Multi-node file handles — not in globals, local to this module */
static int dlf_w = -1;
static int edlf_w = -1;
static int userfile_w = -1;
static int edlf_dn = 0;
static int nodestatusfile = -1;

/* Forward declarations */
void closedlf_w(void);
void closeedlf_w(void);
void close_user_w(void);

int lockchain=-1, locknetdial=-1, locknetlocal=-1;

int filecopy(char *oldfilename, char *newfilename)
{
  char *buffer;
  int i,f1,f2;
  struct ftime filetime;

  f1=open_r(oldfilename);
  if ((freek1(newfilename)*1024)<(double)filelength(f1)) {
    close(f1);
    return(0);
  }
  if ((buffer=(char *)malloca(16400))==NULL)
    return(0);
  f2=open_w(newfilename,O_TRUNC);
  i=read(f1,(void *)buffer,16384);
  while (i>0) {
    write(f2,(void *)buffer,i);
    i=read(f1,(void *)buffer,16384);
  }
  farfree(buffer);
  getftime(f1, &filetime);
  setftime(f2, &filetime);
  close(f1);
  close(f2);
  return(1);
}


FILE *fopen_r(char *s, char *mode) /* open stream for read only */
{
  return(_fsopen(s,mode,SH_DENYNONE));
}

FILE *fopen_w(char *s, char *mode) /* open stream for write */
{
  FILE *f;

  do
    f=_fsopen(s,mode,SH_DENYWR);
  while (f<0);
  return(f);
}

int open_r(char *s) /* open file for read only */
{
  return(open(s,O_RDONLY | O_BINARY | O_DENYNONE));
}

int open_w(char * s, int access) /* open file for write */
{
  int f;

  do
    f=open(s,O_RDWR | O_BINARY | O_CREAT | O_DENYWRITE | access, S_IREAD | S_IWRITE);
  while (f<0);
  return(f);
}

int open_w_exist(char * s, int access) /* open file for write if it exists */
{
  int f;

  if (exist(s))
    do
      f=open(s,O_RDWR | O_BINARY | O_DENYWRITE | access);
    while (f<0);
  else
    f=-1;
  return(f);
}

void opendlf_w1(int dn) /* open a download file for write */
{
  char s[MAX_PATH_LEN];

  closedlf_w();
  sprintf(s,"%s%s.dir",syscfg.datadir,directories[dn].filename);
  dlf_w=open_w(s,0);
}

void opendlf_w() /* open current download file for write */
{
  opendlf_w1(udir[curdir].subnum);
}

void closedlf_w() /* close download file */
{
  if (dlf_w!=-1)
    close(dlf_w);
  dlf_w=-1;
}

void openedlf_w1(int dn) /* open an extended download file for write */
{
  char s[MAX_PATH_LEN];

  closeedlf_w();
  sprintf(s,"%s%s.ext",syscfg.datadir,directories[dn].filename);
  edlf_w=open_w(s,0);
}

void openedlf_w() /* open current extended download file for write */
{
  openedlf_w1(edlf_dn);
}

void closeedlf_w() /* close extended download file */
{
  if (edlf_w!=-1)
    close(edlf_w);
  edlf_w=-1;
}

void load_status()
{
  char s[MAX_PATH_LEN];
  cJSON *st_root;

  sprintf(s,"%sstatus.json",syscfg.datadir);
  st_root = read_json_file(s);
  if (st_root) {
    json_to_statusrec(st_root, &status);
    cJSON_Delete(st_root);
  }
}

void load_user(unsigned int un, userrec *u)
{
  char path[MAX_PATH_LEN];
  cJSON *root;

  if (un>0) {
    sprintf(path, "%susers/%04d.json", syscfg.datadir, un);
    root = read_json_file(path);
    if (root) {
      json_to_userrec(root, u);
      cJSON_Delete(root);
    } else {
      memset(u, 0, sizeof(userrec));
      u->inact = inact_deleted;
    }
  }
}

int open_user_w()
{
  /* No-op: JSON user files are opened/closed per operation */
  return 0;
}

void close_user_w()
{
  /* No-op: JSON user files are opened/closed per operation */
}

void prnodestatus()
{
  typedef struct {
        char            name[31],                /* node user's name */
                        action[31];              /* node user's current action */
       unsigned short   number;                  /* node user's number */
  } nodestatusrec;
  char s[MAX_PATH_LEN],s1[MAX_PATH_LEN];
  int abort=0, i=1, otherstatusfile;
  nodestatusrec nodestatus;

  nl();
  pla("Who is currently online: ",&abort);
  nl();
  sprintf(s,"%sstatus.1",syscfg.datadir);
  for (i=1; (!abort) && (exist(s)); i++) {
    otherstatusfile=open_r(s);
    read(otherstatusfile, (void *)(&nodestatus), sizeof(nodestatusrec));
    close(otherstatusfile);
    sprintf(s,"%s #%d  ...................",nodestatus.name,nodestatus.number);
    sprintf(s1,"  Node %d  %-.25s  %s",i,(nodestatus.number ? s : ".........................."),nodestatus.action);
    pla(s1,&abort);
    sprintf(s,"%sstatus.%d",syscfg.datadir,i+1);
  }
  nl();
}

void save_nodestatus(char *action)
{
  typedef struct {
        char                 name[31],                /* node user's name */
                        action[31];                /* node user's current action */
       unsigned short        number;                        /* node user's number */
  } nodestatusrec;

  nodestatusrec nodestatus;

  strcpy(nodestatus.name,thisuser.name);
  strcpy(nodestatus.action,action);
  nodestatus.number=usernum;
  lseek(nodestatusfile,0L,SEEK_SET);
  write(nodestatusfile, (void *)(&nodestatus), sizeof(nodestatusrec));
}

