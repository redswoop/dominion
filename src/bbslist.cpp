/* bbslist.cpp — BBS phone directory (add/search). */

#include "bbslist.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "bbsutl.h"
#include "disk.h"
#include "mm1.h"
#include "session.h"
#include "system.h"
#include "sysopf.h"


void addbbs(char *fn)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,i1,i2,f,ok;
    char s[150],s1[150],s2[150],ch,ch1,*ss,s3[40],s4[40],final[150],form[40];
    long l,l1;
    FILE *format;

    if(sess.user.restrict_flags() & restrict_bbslist) {
        pl("7You are restricted from adding to the BBSlist");
        pausescr();
        return;
    }
    nl();
    npr("3Please enter phone number\r\n5: ");
    mpl(12);
    inputfone(s);
    nl();

    if ((s[3]!='-') || (s[7]!='-'))
        s[0]=0;

    if (strlen(s)==12) {
        ok=1;
        sprintf(s1,"%s%s",sys.cfg.gfilesdir,fn);

        f=open(s1,O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
        if (f>0) {
            lseek(f,0L,SEEK_SET);
            l=filelength(f);
            if ((ss=(char *)malloca(l+500L))==NULL) {
                close(f);
                return;
            }
            read(f,ss,(int)l);
            l1=0L;
            while ((l1<l) && (ok)) {
                i=0;
                do {
                    ch=ss[l1++];
                    s1[i]=ch;
                    if (ch==13) s1[i]=0;
                    ++i;
                }
                while ((ch!=10) && (i<120) && (l1<l));
                if (strstr(s1,s)!=NULL)
                    ok=0;
                if (!strncmp(s1,s,12))
                    ok=0;
            }
            free(ss);
            close(f);
        }

        if(!ok) {
            nl();
            pl("7Sorry, but that number is already in the BBS list.");
            nl();
            return;
        }

        if(ok) {
            pl("Number not yet in BBS list.");
            nl();
            inputdat("Enter BBS name and comments",s1,40,1);
            inputdat("Enter maximum speed of the BBS",s2,5,0);
            inputdat("Enter BBS type (Dominion)",s3,8,1);
            nl();

            sprintf(form,"%sbbslist.fmt",sys.cfg.gfilesdir);
            format=fopen(form,"rt");
            fgets(form,41,format);
            fclose(format);
            filter(form,'\n');

            strcpy(final,s1);
            sprintf(s1,"%-40s",final);
            strcpy(final,s2);
            sprintf(s2,"%-5s",final);
            strcpy(final,s3);
            sprintf(s3,"%-8s",final);

            stuff_in(final,form,s,s1,s2,s3,"");
            pl(final);
            nl();
            prt(5,"Is this correct? ");
            if (yn()) {
                logtypes(2,"Added to BBSlist:0 %s, %s",s,s1);
                strcat(final,"\r\n");
                sprintf(s1,"%s%s",sys.cfg.gfilesdir,fn);
                f=open(s1,O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                if (filelength(f)) {
                    lseek(f,-1L,SEEK_END);
                    read(f,((void *)&ch1),1);
                    if (ch1==26)
                        lseek(f,-1L,SEEK_END);
                }
                write(f,(void *)final,strlen(final));
                close(f);
                nl();
                pl("Added to BBS list.");
                nl();
            }
        }
    }
}

void searchbbs(char *fn)
{
    auto& sys = System::instance();
    FILE *f;
    char s[150],s1[20],s2[150];


    nl();
    inputdat("Enter Text to Search For",s1,20,0);
    if(!s1[0]) return;

    sprintf(s,"%s%s",sys.cfg.gfilesdir,fn);
    f=fopen(s,"rt");
    while((fgets(s2,150,f))!=NULL) {
        filter(s2,'\n');
        strcpy(s,noc2(s2));
        strupr(s);
        if(strstr(s,s1)) {
            pl(s2);
            pausescr();
        }
    }
    nl();
    fclose(f);
}
