/*
 * sysoplog.cpp — Sysop log file I/O
 *
 * Low-level log writing (sl1) and the sysoplog() wrapper.
 * Moved from bbsutl.cpp.
 */

#include "sysoplog.h"
#include "platform.h"
#include "mm1.h"
#include "session.h"
#include "system.h"
#include "file_lock.h"
#include "bbs_path.h"
#pragma hdrstop


void sl1(int cmd,char *s)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    static int midline=0,slf=-1;
    static char f[MAX_PATH_LEN];
    char l[180],ch1;
    int i;

    if(sess.backdoor) return;

    if(sys.cfg.sysconfig & sysconfig_shrink_term)
        strcpy(s,noc2(s));

    switch(cmd) {
    case 0: /* Write line to sysop's log */
        {
        FileLock lk(f);
        if (slf<=0) {
            slf=open(f,O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
            if (filelength(slf)) {
                lseek(slf,-1L,SEEK_END);
                read(slf,((void *)&ch1),1);
                if (ch1==26)
                    lseek(slf,-1L,SEEK_END);
            }
        }
        if (midline) {
            sprintf(l,"\r\n%s",s);
            midline = 0;
        }
        else
            strcpy(l,s);
        if (sys.cfg.sysconfig & sysconfig_printer)
            fprintf(stdprn,"%s\r\n",noc2(s));
        i=strlen(l);
        l[i++]='\r';
        l[i++]='\n';
        l[i]=0;
        write(slf,(void *)l,i);
        close(slf);
        slf=-1;
        }
        break;
    case 1: /* Close sysop's log */
        if (slf>0) {
            close(slf);
            slf=-1;
        }
        break;
    case 2: /* Set filename */
        if (slf>0) {
            close(slf);
            slf=-1;
        }
        {
            char datecode[8];
            datecode[0]=s[6]; datecode[1]=s[7];
            datecode[2]=s[0]; datecode[3]=s[1];
            datecode[4]=s[3]; datecode[5]=s[4];
            datecode[6]=0;
            auto logpath = BbsPath::join(sys.cfg.gfilesdir, std::string(datecode) + ".log");
            strcpy(f, logpath.c_str());
        }
        break;
    case 3: /* Close sysop's log  + return filename */
        if (slf>0) {
            close(slf);
            slf=-1;
        }
        strcpy(s,&f[strlen(sys.cfg.gfilesdir)]);
        break;
    case 4:
        {
        FileLock lk(f);
        if (slf <= 0) {
            slf = open(f, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
            if (filelength(slf)) {
                lseek(slf, -1L, SEEK_END);
                read(slf, ((void *)&ch1), 1);
                if (ch1 == 26)
                    lseek(slf, -1L, SEEK_END);
            }
        }
        if (!midline || ((midline + 2 + strlen(s)) > 78)) {
            strcpy(l, midline ? "\r\n   " : "   ");
            strcat(l, s);
            midline = 3 + strlen(s);
        }
        else {
            strcpy(l, ", ");
            strcat(l, s);
            midline += (2 + strlen(s));
        }
        if (sys.cfg.sysconfig & sysconfig_printer)
            fprintf(stdprn, "%s", l);
        i = strlen(l);
        write(slf, (void *)l, i);
        }
        break;
    }
}


void sysoplog(char s[161])
{
    auto& sess = Session::instance();
    char s1[181];

    if ((sess.actsl!=255)||incom) {
        sprintf(s1,"   %s",s);
        sl1(0,s1);
    }
}
