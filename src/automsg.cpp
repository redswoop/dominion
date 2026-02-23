/* automsg.cpp — Automessage system (read/write). */

#include "automsg.h"
#include "platform.h"
#include "fcns.h"
#include "disk.h"
#include "jam_bbs.h"
#include "msgbase.h"
#include "mm1.h"
#include "session.h"
#include "system.h"
#include "sysopf.h"


void read_automessage()
{
    auto& sys = System::instance();
    int i;
    messagerec m;
    FILE *f;
    char s[161];

    sprintf(s,"%sauto.fmt",sys.cfg.gfilesdir);
    f=fopen(s,"rt");
    if (!f) return;
    fgets(s,161,f);
    filter(s,'\n');
    plfmt(s);
    fgets(s,161,f);
    filter(s,'\n');
    plfmt(s);
    printfile("auto.msg");
    fgets(s,161,f);
    filter(s,'\n');
    plfmt(s);
    fclose(f);
}


void write_automessage()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    hdrinfo hdr;
    int save,i;
    long len;
    char *b,s[MAX_PATH_LEN];

    if(sess.user.restrict & restrict_automessage) {
        pl("7You are restricted from changing the AutoMessage");
        pausescr();
        return;
    }

    upload_post();
    b=ninmsg(&hdr,&len,&save,0);
    if(save) {
        sys.status.amsganon=0;
        sys.status.amsguser=sess.usernum;
        logtypes(2,"Changed Automessage");
        sprintf(s,"%sauto.msg",sys.cfg.gfilesdir);
        unlink(s);
        i=open(s,O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
        write(i,b,len);
        close(i);
        free(b);
    }
}
