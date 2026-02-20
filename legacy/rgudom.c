#include <fcntl.h>
#include <sys/stat.h>
#include "rgc.h"
#include "vardec.h"

userrec dom;
user_data rg;

void convertrec(void)
{
    int i;

    strcpy(dom.name,rg.name);
    strcpy(dom.realname,rg.realname);
    strcpy(dom.pw,rg.pw);
    pl(rg.pw);
    dom.pw[20]=0;
    strcpy(dom.phone,rg.ph);
    strcpy(dom.street,rg.street);
    strcpy(dom.city,rg.citystate);
    strcpy(dom.note,rg.note);
    dom.note[40]=0;
    strcpy(dom.firston,rg.firston);
    dom.firston[8]=0;
    strcpy(dom.laston,rg.laston);
    dom.laston[8]=0;

    strcpy(dom.comment,"");
    dom.sex='?';
    dom.uk=rg.uk;
    dom.dk=rg.dk;
    dom.fpts=rg.filepoints;
    dom.uploaded=rg.uploads;
    dom.downloaded=rg.downloads;
    dom.msgpost=rg.msgpost;
    dom.waiting=0;
    dom.emailsent=rg.emailsent;
    dom.logons=rg.loggedon;
    dom.timebank=rg.timebank;
    dom.feedbacksent=rg.feedback;
    dom.posttoday=0;
    dom.etoday=0;
    dom.timeon=0;
    dom.pos_account=0.0;
    dom.neg_account=0.0;
    dom.extratime=0.0;
    dom.timeontoday=0.0;
    dom.fsenttoday1=0;
    dom.num_extended=0;
    dom.ass_pts=0;
    dom.age=0;
    dom.screenchars=rg.linelen;
    dom.screenlines=rg.pagelen;
    dom.sl=rg.sl;
    dom.dsl=rg.dsl;
    dom.illegal=0;
    dom.ontoday=0;
    dom.forwardusr=0;
    dom.ar=dom.dar=dom.exempt=0;
    dom.qscn=0xFFFFFFFF;
    dom.qscn2=0xFFFFFFFF;
    for(i=0;i<32;i++) dom.qscnptr[i]=0;
    for(i=0;i<32;i++) dom.qscnptr2[i]=0;

    dom.nscn1=0xFFFFFFFF;
    dom.nscn2=0xFFFFFFFF;
    dom.daten=0;

    dom.res[3]=99;
    dom.comp_type=99;
    dom.defprot=99;
    dom.res[7]=0;
    dom.year=0;
    dom.inact=0;
    dom.sysstatus=0;
}

void main(int ac,char **ar)
{
    int i,o;
    char in[81],out[81];

    if(ac>1)
        strcpy(in,ar[1]);
    if(ac>2)
        strcpy(out,ar[2]);

    nl();
    inputdat("Renegade Filename to Convert",in,71);
    inputdat("Target Dominion Filename",out,71);

    i=open(in,O_BINARY|O_RDWR);
    if(i<0) {
        nl();
        pl("File Not Found");
        exit(0);
    }

    lseek(i,1568L,0);

    o=open(out,O_BINARY|O_RDWR|O_TRUNC|O_CREAT,S_IREAD|S_IWRITE);
     write(o,&dom,sizeof(dom));
    while(read(i,&rg,sizeof(rg))) {
        convertrec();
        write(o,&dom,sizeof(dom));
    }

    close(in);
    close(out);
}
