#include "Vardec.h"
#include <fcntl.h>

void main(int ac, char **ar)
{
    int i;
    statusrec status;
    char s[161];

    printf("QuickLog v1.0 - Doiminion External Log Viewer\n\n");

    if(ac<2) {
        printf("\nUsage: QuickLog <path to datadir> <path to afiles>\n");
        exit(0);
    }

    sprintf(s,"%s\\status.dat",ar[1]);
    puts(s);
    i=open(s,O_BINARY|O_RDWR);
    read(i,&status,sizeof(statusrec));
    close(i);
    strcpy(s,status.date1);
    i=0;
    status.date1[i++]=s[6];
    status.date1[i++]=s[7];
    status.date1[i++]=s[0];
    status.date1[i++]=s[1];
    status.date1[i++]=s[3];
    status.date1[i++]=s[4];
    status.date1[i]=0;
    strcat(status.date1,".LOG");

    sprintf(s,"lst %s\\%s",ar[2],status.date1);
    puts(s);
    system(s);
}
