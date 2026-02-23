#include "error.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "tcpio.h"
#include "bbsutl.h"
#include "timest.h"
#include "session.h"
#include "system.h"
#pragma hdrstop

char *curtime(void);

void err(int err,char *fn,char msg[80])
{
    long l;
    FILE *f;
    char s[MAX_PATH_LEN];


    pl("An Error Has Occured - Will try to save system.  Sorry for any inconvience");
    switch(err) {
    case 1: 
        sprintf(s,"File %s Not Found",fn); 
        break;
    case 2: 
        strcpy(s,"String File Not Found"); 
        break;
    case 3: 
        strcpy(s,"Not Enough Memory for Buffer"); 
        break;
    case 4: 
        strcpy(s,"File Could Not Be Created"); 
        break;
    case 5: 
        strcpy(s,"Fossil Driver Not Installed"); 
        break;
    case 6: 
        strcpy(s,"Video Error"); 
        break;
    case 7: 
        strcpy(s,"Clock Corrupted"); 
        break;
    }
    logpr("7!! 8System Error at %s.  Check Error.Log 7!!",curtime());
    textattr(128+12);
    cprintf("\r\n!! System Error at %s.  Check Error.Log !!\r\n",curtime());
    sound(1200);
    delay(500);
    nosound();

    f=fopen("error.log","at");
    if (f) {
        fprintf(f,">>!<< %s - Errorlevel %d (%s)\n",s,err,curtime());
        fprintf(f,"[%s]\n",msg);
        fclose(f);
    }

    f=fopen("Critical","wt");
    if (f) fclose(f);

    textattr(15);
    cprintf(">>!<< %s - Errorlevel %d (%s)\r\n",s,err,curtime());
    textattr(9);
    cprintf("[%s]\r\n",msg);
    sl1(1,"");
    if (ok_modem_stuff) closeport();
    dtr(0);
    exit(err);
}
