#include "vardec.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>


configrec syscfg;

void fix(char *fn,mmrec n)
{
    int i,i1;
    mmrec p;
    char s[81];

    pr("3Applying Format To File 3%s - ",fn);

    i=open(fn,O_BINARY|O_RDWR);
    read(i,&p,sizeof(p));


    if(n.prompt[0]) strcpy(p.prompt,n.prompt);
    if(n.prompt2[0]) strcpy(p.prompt2,n.prompt2);
    if(n.title1[0]) strcpy(p.title1,n.title1);
    if(n.title2[0]) strcpy(p.title2,n.title2);
    if(n.altmenu[0]) strcpy(p.altmenu,n.altmenu);
    if(n.format[0]) strcpy(p.format,n.format);
    if(n.helpfile[0]) strcpy(p.helpfile,n.helpfile);
    if(n.columns) p.columns=n.columns;
    if(n.col[0]) p.col[0]=n.col[0];
    if(n.col[1]) p.col[1]=n.col[1];
    if(n.col[2]) p.col[2]=n.col[2];


    lseek(i,0L,0);
    write(i,&p,sizeof(p));

    npr("5Done.");
}

void writeheader(mmrec pp)
{
    FILE *f;
    char s[81];

    inputdat("Filename to Write to",s,12,0);
    if(!s[0]) return;

    f=fopen(s,"wb");
    fputs(pp.prompt,f); fputs("\n",f);
    fputs(pp.prompt2,f); fputs("\n",f);
    fputs(pp.title1,f); fputs("\n",f);
    fputs(pp.title2,f); fputs("\n",f);
    fputs(pp.format,f); fputs("\n",f);
    itoa(pp.columns,s,10);
    fputs(s,f); fputs("\n",f);

    fclose(f);
    npr("Menu Header Extracted to %s\r\n",s);
    pausescr();
}

void readheader(mmrec *p)
{
    FILE *f;
    char s[161];
    mmrec pp;


    inputdat("Filename to Read",s,12,0);
    if(!s[0]) return;
    f=fopen(s,"rt");

    fgets(s,161,f); filter(s,'\n'); strcpy(pp.prompt,s);
    fgets(s,161,f); filter(s,'\n'); strcpy(pp.prompt2,s);
    fgets(s,161,f); filter(s,'\n'); strcpy(pp.title1,s);
    fgets(s,161,f); filter(s,'\n'); strcpy(pp.title2,s);
    fgets(s,161,f); filter(s,'\n'); strcpy(pp.format,s);
    fgets(s,161,f); filter(s,'\n'); pp.columns=atoi(s);

    fclose(f);

    strcpy(pp.slneed,"");
    strcpy(pp.pausefile,"");
    strcpy(pp.altmenu,"");
    pp.col[0]=0;
    pp.col[1]=0;
    pp.col[2]=0;

     pausescr();
    *p=pp;
}


void extractheader(mmrec *p)
{
    char s[161],s1[9];
    mmrec pp;
    int i;


    inputdat("Menu Information to Extract",s1,8,0);
    if(!s1[0]) return;
    sprintf(s,"%s%s.mnu",syscfg.menudir,s1);
    pl(s);
    i=open(s,O_BINARY|O_RDWR);
    read(i,&pp,sizeof(pp));
    close(i);

    pausescr();
    *p=pp;
}



int setfix(mmrec *nn)
{
    mmrec pp;
    int i,d1=0;
    char ch,s[161],s1[161],done=0;


    pp=(*nn);

    do {
        outchr(12);
        npr("1[1Menu Style Applicator v1.0 for Dominion v3.11]");
        nl();
        npr("31. Prompt       :0 %s",pp.prompt);
        npr("32. Titles       :0 %s",pp.title1);
        npr("3                :0 %s",pp.title2);
        npr("33. Ansi Menu    :0 %s",pp.altmenu[0]?pp.altmenu:"\"\"");
        npr("34. Security     :0 %s",pp.slneed[0]?pp.slneed:"None");
        npr("35. Style Editor");
        npr("36. HelpLevel    :0 %d",pp.helplevel-1);
        npr("37. Format File  :0 %s",pp.format);
        npr("38. MCI Name     :0 %s",pp.pausefile);
        npr("39. Extended Help:0 %s",pp.helpfile);
        npr("30. Pldn Prompt  :0 %s",pp.prompt2);
        nl();

        outstr("5Menu Editor (Q=Quit, E=Extract Header, R=Read File, W=Write File, A=Apply):0 ");
        ch=onek("AQ?1234567890RWE\r");
        switch(ch) {
            case 'R': readheader(&pp); break;
            case 'W': writeheader(pp); break;
            case 'E': extractheader(&pp); break;
            case '1': pl("Prompt:");
                      outstr(": "); input(s,99);
                      if(s[0]) strcpy(pp.prompt,s);
                      break;
            case '0': pl("PullDown Prompt:");
                      outstr(": "); input(s,99);
                      if(s[0]) strcpy(pp.prompt2,s);
                      break;
            case '2': pl("Title 1:");
                      outstr(": "); input(s,51);
                      if(s[0]) strcpy(pp.title1,s);
                      pl("Title 2:");
                      outstr(": "); input(s,51);
                      if(s[0]) strcpy(pp.title2,s);  break;
            case '3': inputdat("External Menu",s,8,0);
                      if(s[0]) strcpy(pp.altmenu,s);
                      break;
            case '4': inputdat("Security",pp.slneed,sizeof(pp.slneed),1);
                      break;
            case '7': inputdat("Format File",pp.format,8,0); break;
            case '8': inputdat("Menu's MCI Name",pp.pausefile,9,1); break;
            case '9': inputdat("External Help File .HLP",pp.helpfile,8,0); break;
            case '5': d1=0;
                      do {
                      outchr(12);
                      npr("0Menu Style Editor\r\n"); nl();
                      npr("1. Brackets    : %d\r\n",pp.col[0]);
                      npr("2. Commands    : %d\r\n",pp.col[1]);
                      npr("3. Description : %d\r\n",pp.col[2]);
                      npr("4. Columns     : %d\r\n",pp.columns);
                      nl();
                      outstr("Select: ");
                      ch=onek("1234VQ\r");
                      switch(ch) {
                      case '4':  outstr("Enter Number of Columns (1,2,3,4) ");
                                 input(s,2);
                                 if(s[0]) pp.columns=atoi(s);
                                 break;
                      case '1':  outstr("Bracket Attr: "); input(s,2);
                                 if(s[0]) pp.col[0]=atoi(s);
                                 break;
                      case '2':  outstr("Command Key Attr: "); input(s,2);
                                 if(s[0]) pp.col[1]=atoi(s);
                                 break;
                      case '3':  outstr("Description Attr: "); input(s,2);
                                 if(s[0]) pp.col[2]=atoi(s);
                                 break;
                      case '\r':
                      case 'Q': d1=1; break;
                      }
                   } while(!d1);
                   break;
            case 'Q': done=1; break;
            case 'A': done=2; break;
        }
   } while(!done);

   *nn=pp;
   return (done);
}

void main(void)
{
    struct ffblk f;
    int i;
    mmrec n;
    char s[81];

    i=open("config.dat",O_RDWR|O_BINARY);
    if(i<0) {
        pl("7Config.dat Not found!  Exiting!");
        exit(0);
    }
    read(i,&syscfg,sizeof(syscfg));
    close(i);

    strcpy(n.title1,"");
    strcpy(n.title2,"");
    strcpy(n.prompt,"");
    strcpy(n.prompt2,"");
    strcpy(n.altmenu,"");
    strcpy(n.format,"");
    strcpy(n.slneed,"");
    strcpy(n.pausefile,"");
    n.col[0]=0;
    n.col[1]=0;
    n.col[2]=0;
    n.columns=0;

    if(setfix(&n)==1) exit(0);

    syscfg.menudir[strlen(syscfg.menudir)-1]=0;

    chdir(syscfg.menudir);

    findfirst("*.mnu",&f,0);
    do fix(f.ff_name,n); while(findnext(&f)!=-1);
}
