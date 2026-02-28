/*
 * system.cpp — System-wide state singleton + persistence (Phase C)
 */

#include "system.h"
#include "bbs_path.h"
#include "json_io.h"
#include "file_lock.h"
#include "error.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

System& System::instance()
{
    static System s;
    return s;
}

System::System()
{
    memset(this, 0, sizeof(System));
    ARC_NUMBER = -1;
}


static unsigned long maxul(unsigned long a, unsigned long b) { return a > b ? a : b; }
static unsigned short maxus(unsigned short a, unsigned short b) { return a > b ? a : b; }

void save_status()
{
    auto& sys = System::instance();

    auto spath = BbsPath::join(sys.cfg.datadir, "status.json");

    /* Lock, read fresh from disk, merge monotonic counters, write back. */
    FileLock lk(spath.c_str());

    cJSON *fresh_json = read_json_file(spath.c_str());
    if (fresh_json) {
        statusrec fresh;
        memset(&fresh, 0, sizeof(fresh));
        json_to_statusrec(fresh_json, &fresh);
        cJSON_Delete(fresh_json);

        /* Merge: take max of monotonic counters */
        sys.status.callernum1 = maxul(sys.status.callernum1, fresh.callernum1);
        sys.status.callernum = maxus(sys.status.callernum, fresh.callernum);
        sys.status.callstoday = maxus(sys.status.callstoday, fresh.callstoday);
        sys.status.msgposttoday = maxus(sys.status.msgposttoday, fresh.msgposttoday);
        sys.status.emailtoday = maxus(sys.status.emailtoday, fresh.emailtoday);
        sys.status.fbacktoday = maxus(sys.status.fbacktoday, fresh.fbacktoday);
        sys.status.uptoday = maxus(sys.status.uptoday, fresh.uptoday);
        sys.status.activetoday = maxus(sys.status.activetoday, fresh.activetoday);
        sys.status.users = maxus(sys.status.users, fresh.users);
        sys.status.qscanptr = maxul(sys.status.qscanptr, fresh.qscanptr);
    }

    cJSON *root = statusrec_to_json(&sys.status);
    /* Write directly — we already hold the lock.  Can't call write_json_file()
       because it takes its own FileLock on the same path, and flock(2) on macOS
       self-deadlocks when the same process opens two independent fds. */
    char *str = cJSON_Print(root);
    if (str) {
        FILE *f = fopen(spath.c_str(), "w");
        if (f) {
            fputs(str, f);
            fputc('\n', f);
            fclose(f);
        }
        free(str);
    }
    cJSON_Delete(root);
}


void read_in_file(char *fn, messagerec *m, int maxary)
{
    auto& sys = System::instance();
    int i,which;
    char *buf,s[161];
    long l,l1;

    for (i=0; i<maxary; i++) {
        m[i].stored_as=0L;
        m[i].storage_type=0;
    }

    auto path = BbsPath::join(sys.cfg.gfilesdir, fn);
    i=open(path.c_str(),O_RDWR | O_BINARY);
    if (i<0) {
        err(1,(char*)path.c_str(),"In Read_in_file");
    }
    l=filelength(i);
    buf=(char *) malloc(l);
    lseek(i,0L,SEEK_SET);
    if (buf==NULL) {
        err(3,"","In Read_in_file");
    }
    read(i,(void *) buf,l);
    close(i);
    l1=0;

    while(l1<l) {
        if(buf[l1]=='~') {
            i=0;
            l1++;
            while(buf[l1]!=10&&buf[l1]!=13) {
                s[i++]=buf[l1++];
            }

            s[i]=0;
            which=atoi(s);
            m[which].storage_type=l1;
            m[which].stored_as=0;
        }
        else {
            m[which].stored_as++;
            l1++;
        }
    }

    free((void *) buf);
}
