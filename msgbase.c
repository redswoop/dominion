#include "vars.h"

#pragma hdrstop

#include <mem.h>
#include <share.h>


int external_edit(char *fn1, char *direc, int numlines)
{
    char s[81],s1[81],fn[81];

    sprintf(fn,"%s%s",direc,fn1);
    sprintf(s1,"%d",numlines);

    stuff_in(s,"quicked %1 %2",fn,s1,"","","");

    runprog(s,0);

    return(exist(fn));
}


int okfsed()
{
    int ok;

    if (!okansi())
        ok=0;
    if (!thisuser.defed)
        ok=0;
    return(ok);
}



void addline(char *b, char *s, long *ll)
{
    strcpy(&(b[*ll]),s);
    *ll +=strlen(s);
    strcpy(&(b[*ll]),"\r");
    *ll += 1;
}





void showmsgheader(char a,char title[81],char name[41],char date[41],char to[41],int reading, int nummsgs,char comment[51],char subnum,int *abort)
{
    FILE *f;
    char s[255],s1[255],s2[10],s3[10],s4[41];
    int mcir=mciok;

    mciok=1;
    if(subboards[usub[subnum].subnum].attr & mattr_fidonet)
        sprintf(s,"%smsgnet%d.fmt",syscfg.gfilesdir,thisuser.mlisttype);
    else
        sprintf(s,"%smsg%d.fmt",syscfg.gfilesdir,thisuser.mlisttype);

    if(!exist(s)) {
        npr("By: %-30s, To:%s\r\n",name,to);
        npr("Title: %s\r\n",title);
        nl();
        return;
    }

    f=fopen(s,"rt");
    fgets(s,255,f);

    while((fgets(s,255,f))!=NULL) {
        filter(s,'\n');

        strcpy(s1,noc2(title));
        strcpy(title,s1);

        if(a==1) {
            strcpy(name,"Anonymous");
            strcpy(comment,"Unknown!");
        } 
        else if(a==2) {
            strcpy(s1,name);
            strcpy(name,"®");
            strcat(name,s1);
            strcat(name,"¯");
        }

        strcpy(s4,noc2(subboards[usub[subnum].subnum].name));
        sprintf(s2,"%3d",reading);
        sprintf(s3,"%3d",nummsgs);

        stuff_in1(s1,s,title,name,date,to,"",comment,s2,s3,s4,"");
        pla(s1,abort);
    }
    fclose(f);
    mciok=mcir;
}



void osan(char *s, int *abort, int *next)
{
    int i;

    i=0;
    checkhangup();
    if (hangup)
        *abort=1;
    checka(abort,next,0);
    while ((s[i]) && (!(*abort))) {
        outchr(s[i++]);
    }
    checka(abort,next,0);
}


void getorigin(int origin, originrec *or)
{
    int i;
    char s[81];

    sprintf(s,"%sorigin.dat",syscfg.datadir);
    i=open(s,O_BINARY|O_RDWR);
    lseek(i,sizeof(originrec)*origin,0);
    read(i,or,sizeof(originrec));
    close(i);
}

void upload_post()
{
    char s[81],s1[21],ch;
    int i,i1,maxli,f;
    long l;

    if(incom) {
        npr("5Upload Prepared File? ");
        if(!yn()) return;
    } 
    else {
        npr("5Load Local File? ");
        if(yn()) {
            nl();
            inputdat("File Name",s,61,0);
            load_workspace(s,1);
        }
        return;
    } 

    sprintf(s,"%smsgtmp",syscfg.tempdir);

    nl();
    npr("You may now upload a message.");
    nl();
    receive_file(s,&i1,s1,0);
    f=open(s,O_RDWR | O_BINARY);
    if (f>0) {
        close(f);
        use_workspace=1;
        nl();
    } 
    else {
        nl();
        pl("Nothing Recieved, Sorry.");
        nl();
    }
}

//#include <dir.h>


void extract_out(char *b, long len, hdrinfo *hdr)
{
    char s1[81],s2[81],ch=26,ch1;
    int i;

    do {
        inputdat("Filename to save as",s1,12,0);
        if (s1[0]) {
            sprintf(s2,"%s%s",syscfg.gfilesdir,s1);
            if (exist(s2)) {
                nl();
                pl("Filename already in use.");
                nl();
                outstr("5(O)verwrite, (A)ppend, (N)ew name, (Q)uit? ");
                ch1=onek("QOAN");
                switch(ch1) {
                case 'Q':
                    s2[0]=0;
                    s1[0]=0;
                    break;
                case 'N':
                    s1[0]=0;
                    break;
                case 'A':
                    break;
                case 'O':
                    unlink(s2);
                    break;
                }
                nl();
            }
        } 
        else
            s2[0]=0;
    } 
    while ((!hangup) && (s2[0]!=0) && (s1[0]==0));
    for(i=0;i<len;i++)
        if(b[i]==13||b[i]=='') b[i]=10;
    if ((s1[0]) && (!hangup)) {
        i=open(s2,O_RDWR | O_BINARY | O_CREAT , S_IREAD | S_IWRITE);
        if (filelength(i)) {
            lseek(i, -1L, SEEK_END);
            read(i, ((void *)&ch1), 1);
            if (ch1 == 26)
                lseek(i, -1L, SEEK_END);            
        }
        strcpy(s1,"Subject: ");
        write(i,&s1,strlen(s1));
        write(i,&hdr->subject,strlen(hdr->subject));
        write(i,"\r\n",2);

        strcpy(s1,"From: ");
        write(i,&s1,strlen(s1));
        write(i,&hdr->who_from,strlen(hdr->who_from));
        write(i,"\r\n",2);

        strcpy(s1,"To: ");
        write(i,&s1,strlen(s1));
        write(i,&hdr->who_to,strlen(hdr->who_to));
        write(i,"\r\n",2);

        write(i,(void *)b,len);
        write(i,&ch,1);
        close(i);
        npr("Message written to: %s.\r\n",s2);
    }
}


void load_workspace(char *fnx, int no_edit)
{
    int i,i5;
    long l;
    char *b,s[81];

    i5=open(fnx,O_RDONLY | O_BINARY);
    if (i5<1) {
        nl();
        pl("File not found.");
        nl();
        return;
    }
    l=filelength(i5);
    if ((b=malloca(l+1024))==NULL) {
        close(i5);
        return;
    }
    read(i5, (void *) b,l);
    close(i5);
    if (b[l-1]!=26)
        b[l++]=26;
    sprintf(s,"%smsgtmp",syscfg.tempdir);
    i5=open(s,O_RDWR | O_CREAT | O_BINARY,S_IREAD | S_IWRITE);
    write(i5, (void *)b,l);
    close(i5);
    farfree(b);
    if ((no_edit))
        use_workspace=1;
    else
        use_workspace=0;
    nl();
    pl("File loaded into workspace.");
    nl();

}


void yourinfomsg()
{
    nl();
    dtitle("Your Message Status");
    npr("0Total Posts   5: 4%d\r\n",thisuser.msgpost);
    npr("0Posts Today   5: 4%d\r\n",thisuser.posttoday);
    npr("0FeedBack      5: 4%d\r\n",thisuser.feedbacksent);
    npr("0Email Sent    5: 4%d\r\n",thisuser.emailsent);
    npr("0Messages Read 5: 4%d\r\n",thisuser.msgread);
    npr("0Mail Waiting  5: 4%d\r\n",thisuser.waiting);
    npr("0Your PCR      5: 4%.0f%%\r\n",post_ratio()*100);
    npr("0Required PCR  5: 4%.0f%%\r\n",syscfg.post_call_ratio*100);
    nl();
    nl();
}


int sublist(char type)
{
    FILE *f;
    subboardrec d;
    char s[163],s1[163],s2[5],s3[25],s4[161];
    int i,i1,abort=0;
    if(type);

    sprintf(s,"%ssublist.fmt",syscfg.gfilesdir);
    f=fopen(s,"rt");

    fgets(s,163,f); 
    filter(s,'\n');
    plfmta(s,&abort);
    fgets(s,163,f); 
    filter(s,'\n');
    plfmta(s,&abort);


    fgets(s1,163,f); 
    filter(s1,'\n');
    fgets(s4,163,f); 
    filter(s4,'\n');

    for(i=0;i<umaxsubs&&usub[i].subnum!=-1&&!abort; i++) {
        //        iscan(i);
        d=subboards[usub[i].subnum];
        itoa(nummsgs,s2,10);

        i1=0;
        if(inscan(i,&thisuser))
            i1=1;

        if(i1)
            stuff_in2(s,s1,noc2(d.name),40,usub[i].keys,2,s2,3,"",0,"",0);
        else
            stuff_in2(s,s4,noc2(d.name),40,usub[i].keys,2,s2,3,"",0,"",0);
        plfmta(s,&abort);
    }
    fgets(s1,163,f); 
    filter(s1,'\n');
    plfmta(s1,&abort);

    fclose(f);
    return 0;
}

void get_quote()
{
    static char s[141];
    static int i,i1,i2,abort,next,rl;

    rl=1;
    do {
        if (rl) {
            i=1; 
            i2=0; 
            abort=0; 
            next=0;
            do {
                npr("0%2d: ",i++);
                i1=0;
                if (abort) {
                    do {
                        i2++;
                    } 
                    while ((quote[i2]!=13) && (quote[i2]!=0));
                } 
                else {
                    do {
                        s[i1++]=quote[i2++];
                    } 
                    while ((quote[i2]!=13) && (quote[i2]!=0));
                }
                if (quote[i2]) {
                    i2+=2;
                    s[i1]=0;
                }
                pla(s,&abort);
            } 
            while (quote[i2]!=0);
            --i;
        }
        nl();
        i1=0; 
        i2=0; 
        s[0]=0;
        while (!s[0]) {
            sprintf(s,"Quote from line 1-%d? (A=All,?=relist,Q=quit) ",i);
            prt(2,s);
            input(s,3);
        }
        if (s[0]=='A') {
            quoting=0;
            charbufferpointer=0;
            bquote=1;
            equote=i;
            return;
        }
        if (s[0]=='Q')
            rl=0;
        else if (s[0]!='?') {
            i1=atoi(s);
            if (i1==i)
                i2=i1;
            else {
                s[0]=0;
                while (!s[0]) {
                    sprintf(s,"through line %d-%d? (Q=quit) ",i1,i);
                    prt(2,s);
                    input(s,3);
                }
                if (s[0]=='Q')
                    rl=0;
                else if (s[0]!='?')
                    i2=atoi(s);
            }
        }
        if (i2) {
            sprintf(s,"Quote line(s) %d-%d? ",i1,i2);
            prt(5,s);
            if (!ny())
                i2=0;
        }
    } 
    while ((!abort) && (!hangup) && (rl) && (!i2));
    quoting=0;
    charbufferpointer=0;
    if ((i1>0) && (i2>=i1) && (i2<=i) && (i2-i1<50) && (rl)) {
        bquote=i1;
        equote=i2;
    }
}


char *ini(char name[81])
{
    char o[81];
    int i=0,p=0;

    memset(o,0,81);

    for(i=0;name[i];i++) {
        if ((name[i]>='A') && (name[i]<='Z'))
            o[p++]=name[i];
    }

    return(o);
}


void parseadd(char s[81],addressrec *a)
{
    char *p,s1[81];

    strcpy(s1,s);
    p=strtok(s1,":");
    a->zone=atoi(p);
    p=strtok(NULL,"/");
    a->net=atoi(p);
    p=strtok(NULL,".");
    a->node=atoi(p);
    p=strtok(NULL,"");
    a->point=atoi(p);
}

void parse_email_info(char *s, unsigned short *un1)
{
    char *ss;
    unsigned un;
    int i;
    *un1=0;

    un=finduser1(s);
    if (un>0)
        *un1=un;
    else
        pl("Unknown user.");
}

typedef struct {
    UINT32 crc;
    char scan;
} newscanrec;

int inscan(int sb,userrec *u)
{
    char s[81];
    int i,i1;
    newscanrec nws;
    long num;
    UINT32 ucrc;

    sprintf(s,"%s%s.nws",syscfg.msgsdir,subboards[sb].filename);
    i=open(s,O_BINARY|O_RDWR|O_CREAT,S_IREAD|S_IWRITE);
//    i=sopen(s,O_BINARY|O_RDWR|O_CREAT,SH_DENYRW|SH_DENYWR,S_IREAD|S_IWRITE);

    strcpy(s,u->realname);
    strlwr(s);
    ucrc=JAMsysCrc32( s,strlen(s), ( UINT32 ) -1L );
    num=filelength(i)/sizeof(nws);

    for(i1=0;i1<num;i1++) {
        read(i,&nws,sizeof(nws));
        if(nws.crc==ucrc) {
            close(i);
            return(nws.scan);
        }
    }

    nws.crc=ucrc;
    nws.scan=1;
    write(i,&nws,sizeof(nws));
    close(i);
    return 1;
}


void togglenws(int sb,userrec *u,int scan)
{
    char s[81];
    int i,i1;
    newscanrec nws;
    long num;
    UINT32 ucrc;

    sprintf(s,"%s%s.nws",syscfg.msgsdir,subboards[sb].filename);
//    i=sopen(s,O_BINARY|O_RDWR|O_CREAT,SH_DENYRW|SH_DENYWR,S_IREAD|S_IWRITE);
    i=open(s,O_BINARY|O_RDWR|O_CREAT,S_IREAD|S_IWRITE);
    num=filelength(i)/sizeof(nws);

    strcpy(s,u->realname);
    strlwr(s);
    ucrc=JAMsysCrc32( s,strlen(s), ( UINT32 ) -1L );

    for(i1=0;i1<num;i1++) {
        read(i,&nws,sizeof(nws));
        if(nws.crc==ucrc) {
            nws.scan=scan;
            lseek(i,i1*sizeof(nws),SEEK_SET);
            write(i,&nws,sizeof(nws));
            close(i);
            return;
        }
    }

    lseek(i,0,SEEK_END);
    nws.crc=ucrc;
    nws.scan=scan;
    write(i,&nws,sizeof(nws));
    close(i);
}
