#include "shortmsg.h"
#include "platform.h"
#include "bbs_output.h"
#include "session.h"
#include "system.h"
#include "userdb.h"
#include "file_lock.h"
#include "bbs_path.h"
#pragma hdrstop


void ssm(unsigned int un, unsigned int sy, char *s)
{
    auto& sys = System::instance();
    int  f,i,i1;
    User u;
    char s1[161];
    shortmsgrec sm;

    if (un==65535)
        return;
    if (sy==0) {
        { auto p = UserDB::instance().get(un); if (p) u = *p; }
        if (!(u.inact() & inact_deleted)) {
            auto smw_path = BbsPath::join(sys.cfg.datadir, "smw.dat");
            FileLock lk(smw_path.c_str());
            f=open(smw_path.c_str(),O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
            i=(int) (filelength(f) / sizeof(shortmsgrec));
            i1=i-1;
            if (i1>=0) {
                lseek(f,((long) (i1)) * sizeof(shortmsgrec), SEEK_SET);
                read(f,(void *)&sm,sizeof(shortmsgrec));
                while ((sm.tosys==0) && (sm.touser==0) && (i1>0)) {
                    --i1;
                    lseek(f,((long) (i1)) * sizeof(shortmsgrec), SEEK_SET);
                    read(f,(void *)&sm,sizeof(shortmsgrec));
                }
                if ((sm.tosys) || (sm.touser))
                    ++i1;
            }
            else
                i1=0;
            sm.tosys=sy;
            sm.touser=un;
            strncpy(sm.message,s,80);
            sm.message[80]=0;
            lseek(f,((long) (i1)) * sizeof(shortmsgrec), SEEK_SET);
            write(f,(void *)&sm,sizeof(shortmsgrec));
            close(f);
            u.set_sysstatus_flag(sysstatus_smw, true);
            UserDB::instance().store(un, u);
        }
    }
}

void rsm(int un, User& u)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    shortmsgrec sm;
    int i,i1,f,any;
    char s1[MAX_PATH_LEN];

    any=0;
    if (u.sysstatus() & sysstatus_smw) {
        auto smw_path = BbsPath::join(sys.cfg.datadir, "smw.dat");
        FileLock lk(smw_path.c_str());
        f=open(smw_path.c_str(),O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        i=(int) (filelength(f) / sizeof(shortmsgrec));
        for (i1=0; i1<i; i1++) {
            lseek(f,((long) (i1)) * sizeof(shortmsgrec),SEEK_SET);
            read(f,(void *)&sm,sizeof(shortmsgrec));
            if ((sm.touser==un) && (sm.tosys==0)) {
                pl(sm.message);
                sm.touser=0;
                sm.tosys=0;
                sm.message[0]=0;
                lseek(f,((long) (i1)) * sizeof(shortmsgrec),SEEK_SET);
                write(f,(void *)&sm,sizeof(shortmsgrec));
                any=1;
            }
        }
        close(f);
        u.set_sysstatus_flag(sysstatus_smw, false);
        sess.smwcheck=1;
    }
    if (any)
        nl();
}
