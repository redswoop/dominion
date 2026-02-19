#include "vardec.h"
#include <dir.h>
#include <stdio.h>
#include <sys\stat.h>
#include <fcntl.h>

void filter(char *s,unsigned char c)
{
    int x=0;

    while(s[x++]!=c);
    s[x-1]=0;

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




void main(int argc,char **argv)
{
    FILE *f;
    int i,i1,c,c1;
    char s[81],s1[81];
    directoryrec d[20],ds;
    uploadsrec u;
    long num;
    struct ffblk ff;

    c=open("dirs.dat",O_BINARY|O_RDWR);
    read(c,&d[0],20*sizeof(d));
    close(c);

    for(i1=1;i1<argc;i1++) {
        ds=d[i1-1];
        f=fopen(argv[i1],"rt");
        sprintf(s,"%s.dir",ds.filename);
        c1=open(s,O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
        lseek(c1,sizeof(u),SEEK_SET);
        num=0;
        while(fgets(s,81,f)!=NULL) {
            filter(s,'\n');
            strcpy(s1,s);
            s1[12]=0;
            strcpy(u.filename,s1);
            strcpy(u.description,&s[13]);
            strcpy(u.upby,"Particle Man");
            u.ownerusr=1;
            u.numdloads=0;
            strcpy(u.date,"02/11/93");
            sprintf(s,"%s%s",ds.dpath,stripfn(u.filename));
            findfirst(s,&ff,0);
            u.numbytes=ff.ff_fsize;
            u.ats[0]=0;
            write(c1,&u,sizeof(u));
            num++;
			cprintf("\rReading #%d",num);
        }
        lseek(c1,0L,SEEK_SET);
        u.numbytes=num-1;
        write(c1,&u,sizeof(u));
        close(c1);
        fclose(f);
        cprintf("\n");
    }
}

