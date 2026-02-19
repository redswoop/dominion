#include <dos.h>
#include <stdio.h>

void filter(char *s,unsigned char c)
{
    int x=0;

    while(s[x++]!=c);
    s[x-1]=0;

}


int main(void)
{

    char s[81],topath[81],spec[81];
    struct date d;
    FILE *f;

    f=fopen("Backups.cfg","rt");
    if(f==NULL) {
        printf("Config file not found.\n");
        exit(1);
    }

    fgets(s,81,f);
    filter(s,'\n');
    strcpy(topath,s);

    fgets(s,81,f);
    filter(s,'\n');
    strcpy(spec,s);

    if(topath[strlen(topath)-1]=='\\')
        topath[strlen(topath)-1]=0;

    getdate(&d);
    sprintf(s,"pkzip -ex -u %s\\%02d%02d%02d.zip %s",topath,d.da_mon,d.da_day,d.da_year-1900,spec);
    system(s);

    return 0;
}
