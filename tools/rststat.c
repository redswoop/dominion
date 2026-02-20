#include "vardec.h"
#include <fcntl.h>
#include <sys/stat.h>

void main(void)
{
    int i;
    statusrec ss;
    char s[MAX_PATH_LEN];

    ss.date1[0]=0;
    ss.date2[0]=0;
    ss.date3[0]=0;
    ss.log1[0]=0;
    ss.log2[0]=0;
    ss.log3[0]=0;
    ss.log4[0]=0;
    ss.log5[0]=0;
    ss.dltoday=0;
    ss.users=1;
    ss.callernum=0;
    ss.callstoday=0;
    ss.msgposttoday=0;
    ss.emailtoday=0;
    ss.fbacktoday=0;
    ss.activetoday=0;
    ss.qscanptr=0;
    ss.amsganon=0;
    ss.amsguser=1;
    ss.callernum1=0;
    ss.uptoday=0;
    strcpy(ss.lastuser,"Dominion Development Team");

    i=open("status.dat",O_RDWR|O_BINARY|O_TRUNC|O_CREAT,S_IREAD,S_IWRITE);
    write(i,&ss,sizeof(statusrec));
    close(i);
}
