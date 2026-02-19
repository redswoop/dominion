#include <fcntl.h>
#include <sys\stat.h>
#include "wwiv.h"
#include "vardec.h"


userrec dom;
wuserrec wwiv;

void convertrec(void)
{
    int i;

    strcpy(dom.name,wwiv.name);
    strcpy(dom.realname ,wwiv.realname);
    strcpy(dom.callsign ,wwiv.callsign);
    strcpy(dom.phone    ,wwiv.phone);
    strcpy(dom.pw       ,wwiv.pw);
    strcpy(dom.laston   ,wwiv.laston);
    strcpy(dom.firston  ,wwiv.firston);
    strcpy(dom.note     ,wwiv.note);
    strcpy(dom.comment,"");

    for(i=0;i<3;i++)
        strcpy(dom.macros[i],wwiv.macros[i]);
    dom.macros[3][0]=0;

    dom.sex         =wwiv.sex;
    dom.age         =wwiv.age;
    dom.comp_type   =wwiv.comp_type;
    dom.defprot     =wwiv.defprot;
    dom.defed       =wwiv.defed;
    dom.screenchars =wwiv.screenchars;
    dom.screenlines =wwiv.screenlines;
    dom.sl          =wwiv.sl;
    dom.dsl         =wwiv.dsl;
    dom.msgpost     =wwiv.msgpost;
    dom.emailsent   =wwiv.emailsent;
    dom.feedbacksent=wwiv.feedbacksent;
    dom.uploaded    =wwiv.uploaded;
    dom.downloaded  =wwiv.downloaded;
    dom.lastrate    =wwiv.lastrate;
    dom.logons      =wwiv.logons;
    dom.msgread     =wwiv.msgread;
    dom.uk          =wwiv.uk;
    dom.dk          =wwiv.dk;
    dom.daten       =wwiv.daten;
    dom.sysstatus   =wwiv.sysstatus;
    dom.timeontoday =wwiv.timeontoday;
    dom.extratime   =wwiv.extratime;
    dom.timeon      =wwiv.timeon;
    dom.pos_account =wwiv.pos_account;
    dom.neg_account =wwiv.neg_account;
    dom.ass_pts     =wwiv.ass_pts;
    dom.month       =wwiv.month;
    dom.day         =wwiv.day;
    dom.year        =wwiv.year;
    dom.emailnet    =wwiv.emailnet;
    dom.postnet     =wwiv.postnet;
    dom.num_extended=wwiv.num_extended;
    dom.optional_val=wwiv.optional_val;

    dom.timebank=0;
    dom.fpts=0;

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
    inputdat("WWIV v4.22 Filename to Convert",in,71);
    inputdat("Target Dominion Filename",out,71);

    i=open(in,O_BINARY|O_RDWR);
    if(i<0) {
        nl();
        pl("File Not Found");
        exit(0);
    }

    o=open(out,O_BINARY|O_RDWR|O_TRUNC|O_CREAT,S_IREAD|S_IWRITE);
     write(o,&dom,sizeof(dom));
    while(read(i,&wwiv,sizeof(wwiv))) {
        convertrec();
        write(o,&dom,sizeof(dom));
    }

    close(in);
    close(out);
}

