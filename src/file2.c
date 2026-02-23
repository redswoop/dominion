#include "vars.h"

#pragma hdrstop

#include <time.h>

int printfileinfo(uploadsrec *u, int dn);
#define MAX_LINES 10
#define FSED_OK ((0))

#define SETREC(i)  lseek(dlf,((long) (i))*((long)sizeof(uploadsrec)),SEEK_SET)

void yourinfodl()
{
    char s[MAX_PATH_LEN];

    nl();
    dtitle("Your Transfer Status");
    npr("0Downloads     5: 4%ldk in %d files\r\n",thisuser.dk, thisuser.downloaded);
    npr("0Uploads       5: 4%ldk in %d files\r\n",thisuser.uk, thisuser.uploaded);
    npr("0File Points   5: 4%d\r\n",thisuser.fpts);
    npr("0Your KB Ratio 5: 4%.0f%%\r\n",ratio()*100);
    npr("0Required Ratio5: 4%.0f%%\r\n",syscfg.req_ratio*100);
    nl();
    strcpy(s,"0Special Flags: 7 ");
    if(thisuser.exempt & exempt_ratio)
        npr("%sNo Ratio/File Point Check!\r\n",s);
    if(thisuser.exempt & exempt_post)
        npr("%sNo Post Call Ratio Check!\r\n",s);
    if(thisuser.exempt & exempt_time)
        npr("%sNo Time Check!\r\n",s);
    nl();

    if(thisuser.helplevel==2) pausescr();
}



void setldate()
{
    struct date d;
    struct time t;
    char s[MAX_PATH_LEN];
    int m,dd,y;

    nl();
    unixtodos(nscandate,&d,&t);
    npr("Current limiting date = %02d/%02d/%02d\r\n",d.da_mon,d.da_day,(d.da_year-1900)%100);
    npr("3Enter NewScan Date\r\n5: ");
    mpl(8);
    inputdate(s,0);
    nl();
    m=atoi(s);
    dd=atoi(&(s[3]));
    y=atoi(&(s[6]))+1900;
    if ((strlen(s)==8) && (m>0) && (m<=12) && (dd>0) && (dd<32) && (y>=1980)) {
        t.ti_min=0;
        t.ti_hour=0;
        t.ti_hund=0;
        t.ti_sec=0;
        d.da_year=y;
        d.da_day=dd;
        d.da_mon=m;
        sprintf(s,"Current limiting date = %02d/%02d/%02d",m,dd,(y-1900)%100);
        nl();
        pl(s);
        nl();
        nscandate=dostounix(&d,&t);
    }
}



void unlisteddl(char ms[MAX_PATH_LEN])
{
    char s[MAX_PATH_LEN];
    int abort,sent;

    nl();
    if(!ms[0])
        inputdat("File Name to DL",s,71,0);
    else
        strcpy(s,ms);
    logtypes(4,"DLed Unlisted file 3%s",s);
    sent=open(s,O_RDONLY|O_BINARY);
    close(sent);
    send_file(s,&sent,&abort,stripfn(s));
}



void getfileinfo()
{
    int i,abort,num=0;
    char s[MAX_PATH_LEN];
    uploadsrec u;

    dliscan();
    nl();
    num=file_mask(s);
    if(num<1)
        i=recno(s);
    else i=num;
    abort=0;

    while ((!hangup) && (i>0) && (!abort)) {
        SETREC(i);
        read(dlf,(void *)&u,sizeof(uploadsrec));
        abort=printfileinfo(&u,udir[curdir].subnum);
        if(num<1)
            i=nrecno(s,i);
        else i=0;
    }
    closedl();
}


int printfileinfo(uploadsrec *u, int dn)
{
    char s[161],s1[MAX_PATH_LEN],s2[11],s3[11],s4[11],fstatus[MAX_PATH_LEN];
    userrec us;
    double t;
    int i,abort=0;
    FILE *f;

    sprintf(s,"%sfstat.fmt",syscfg.gfilesdir);
    f=fopen(s,"rt");
    if(modem_speed)
        t=((double) (((u->numbytes)+127)/128)) * (1620.0)/((double) (modem_speed));
    abort=0;

    strcpy(fstatus,"");
    sprintf(s,"%s%s",directories[dn].dpath,u->filename);
    if(!exist(s)) strcpy(fstatus,"0(6File is OffLine0) ");
    if(u->ats[0]==0) strcat(fstatus,"0(8Unvalidated0) ");
    if(u->mask & mask_unavail) strcat(fstatus,"1(8Unavailble1)");

    while((fgets(s1,81,f))!=NULL&&!abort) {
        filter(s1,'\n');

        ltoa(((u->numbytes)+1023)/1024,s3,10);
        sprintf(s4,"%d",u->points);
        sprintf(s2,"%s",ctim(t));

        if(u->ownersys)
            userdb_load(u->ownersys,&us);
        else
            strcpy(us.name,"PUBLIC");
        stuff_in1(s,s1,(u->filename),(u->description),s3,s4,s2,(u->upby),fstatus,(u->date),ctim(t),nam(&us,u->ownersys));

        plfmta(s,&abort);
    }

    nl();

    print_extended(u->filename,&abort,15,1);
    fclose(f);
    return(abort);
}

void displayformat()
{
    char s[161],s1[161],s2[51];
    FILE *f;

    sprintf(s,"%sfile%d.fmt",syscfg.gfilesdir,thisuser.flisttype);
    f=fopen(s,"rt");
    fgets(s,81,f);
    fclose(f);
    filter(s,'\n');
    sprintf(s2,"%-39s","Description");
    stuff_in1(s1,s,"Filename.Ext",s2,"Size","Fpts","###","Uploader    ","UL Date ","#DL","Time","");
    pl(s1);
}

void ascii_send(char *fn, int *sent, double *percent)
{
    char b[2048];
    int i,i1,done,abort,i2,next;
    long pos,max;
    int mcif=mciok;
    mciok=0;

    i=open(fn,O_RDONLY | O_BINARY);
    if (i>0) {
        max=filelength(i);
        if (!max)
            max=1;
        i1=read(i,(void *)b,1024);
        pos=0L;
        abort=0;
        while ((i1) && (!hangup) && (!abort)) {
            i2=0;
            while ((!hangup) && (!abort) && (i2<i1)) {
                checkhangup();
                outchr(b[i2++]);
                checka(&abort,&next,0);
            }
            pos += (long) i2;
            checka(&abort,&next,0);
            i1=read(i,(void *)b,1024);
        }
        close(i);
        if (!abort)
            *sent=1;
        else {
            *sent=0;
            thisuser.dk += ((pos+1023L)/1024L);
        }
        *percent=((double) pos)/((double)max);
    } 
    else {
        nl();
        pl("File not found.");
        nl();
        *sent=0;
        *percent=0.0;
    }
    mciok=mcif;
}

void send_file(char *fn, int *sent, int *abort, char *ft)
{
    int i,i1;
    double percent,t;
    char s[MAX_PATH_LEN];

    ft[0]=0;
    i=get_protocol(0);
    switch(i) {
    case -1:
        *sent=0;
        *abort=1;
        break;
    case -3:
        *sent=0;
        *abort=0;
        break;
    case -2: 
        break;
    default:
        i1=extern_prot(i,fn,1);
        strcpy(ft,(proto[i].description));
        *abort=0;
        if (i1==proto[i].ok1)
            *sent=1;
        else
            *sent=0;
        break;
    }

}


void receive_file(char *fn, int *received, char *ft, int okbatch)
{
    int i,i1;
    char s[MAX_PATH_LEN];

    ft[0]=0;
    i=get_protocol(okbatch);
    switch(i) {
    case -1:
    case -3:
        *received=0;
        break;
    default:
        i1=extern_prot(i,fn,0);
        strcpy(ft,(proto[i].description));
        if (i1==proto[i].ok1) {
            *received=1;
        } 
        else {
            *received=0;
        }
        break;
    }
}

int get_batchprotocol(int dl,int *hang)
{
    char ch;
    int i1,prot;


    prot=thisuser.defprot;
    if(!proto[prot].description[0]||prot<0) prot=get_protocol(1);
    if(dl);
    *hang=0;

top:
    pl(get_string(67));
    pl(get_string(68));
    npr(get_string(69));
    ch=onek("\rHXAB");
    switch(ch) {
    case 13: 
        return(prot);
    case 'X': 
        return(thisuser.defprot=get_protocol(1));
    case 'A': 
        return(-1);
    case 'B': 
        batchdled(0); 
        goto top;
    case 'H': 
        *hang=1; 
        return(prot);
    }
    return(0);
}


int get_protocol(int batch)
{
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],oks[MAX_PATH_LEN],s2[MAX_PATH_LEN],ch;
    int i,i1,i2,maxprot,done;

    strcpy(oks,"Q?");

    maxprot=0;
    done=0;

    for (i1=0; i1<numextrn; i1++) {
        if(batch||(!batch&&proto[i1].singleok)) {
            ++maxprot;
            sprintf(s1,"%c",proto[i1].key);
            strcat(oks,s1);
        }
    }

    strcpy(s,get_string(73));
    strcpy(s1,oks);

    do {
        nl();
        prt(2,s);
        ch=onek(s1);
        if (ch=='?') {
            nl();
            dtitle("Dominion Transfer Protocols");
            pl("5<5Q5>0 Abort");
            for (i1=0; i1<numextrn; i1++) {
                if(batch||(!batch&&proto[i1].singleok))
                    npr("5<5%c5>0 %s\r\n",proto[i1].key,(proto[i1].description));
            }
            nl();
        } 
        else
            done=1;
    } 
    while ((!done) && (!hangup));
    if (ch=='Q')
        return(-1);
    if(ch=='N') return(-3);
    if(ch=='A') return(-4);

    for(i=0;i<numextrn;i++)
        if(ch==proto[i].key) return(i);

    return(-1);
}


int extern_prot(int pn, char *fn1, int sending)
{
    char s[255],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],fn[MAX_PATH_LEN],sx1[21],sx2[21],sx3[21];
    int i,i1;

    i=0;
    for (i1=0; i1<81; i1++) {
        i+=proto[pn].description[i1];
        i+=proto[pn].sendfn[i1];
        i+=proto[pn].receivefn[i1];
    }
    if (sending) {
        nl();
        if(pn>-1) strcpy(s1,(proto[pn].sendfn));
    } 
    else {
        nl();
        if (pn>-1) strcpy(s1,(proto[pn].receivefn));
    }
    strcpy(fn,fn1);
    stripfn1(fn);
    ultoa(com_speed,sx1,10);
    ultoa(modem_speed,sx3,10);
    sx2[0]='0'+syscfg.primaryport;
    sx2[1]=0;
    stuff_in(s,s1,sx1,sx2,fn,sx3,"");
    if (s[0]) {
        set_protect(0);
        clrscr();
        printf("[0;37;1;44m[KCurrent user: %s\n\n[0;1m",nam(&thisuser,usernum));
        outs(s);
        outs("\r\n");
        if (incom) {
            i=runprog(s,0);
            topscreen();
            return(i);
        }
    }
    return(-5);
}


void stuff_in2(char *s, char *s1, char f1[63],int l1,char f2[63],int l2, char f3[63],int l3,char f4[63],int l4, char f5[63],int l5)
{
    int r=0,w=0,cc;
    char s2[63],s3[63];
    char tbl[6][63];
    int tlen[6];

    strcpy(tbl[1],f1);
    strcpy(tbl[2],f2);
    strcpy(tbl[3],f3);
    strcpy(tbl[4],f4);
    strcpy(tbl[5],f5);

    tlen[1]=l1;
    tlen[2]=l2;
    tlen[3]=l3;
    tlen[4]=l4;
    tlen[5]=l5;

    while (s1[r]!=0) {
        if (s1[r]=='%') {
            ++r;
            s[w]=0;

            cc=s1[r]-'1';
            cc++;

            strcpy(s2,tbl[cc]);
            memset(s3,32,60);
            s3[60]=0;
            strcpy(s3,s2);
            s3[strlen(s2)]=32;
            s3[tlen[cc]]=0;
            strcat(s,s3);

            w=strlen(s);
            r++;

        } 
        else
            s[w++]=s1[r++];
    }
    s[w]=0;
}




int dirlist(char type)
{
    FILE *f;
    directoryrec d;
    char s[163],s1[163],s2[5],s3[25],s4[51],s5[5],s6[161];
    int i,i1,abort=0;
    if(type);

    sprintf(s,"%sdirlist.fmt",syscfg.gfilesdir);
    f=fopen(s,"rt");

    fgets(s,163,f); 
    filter(s,'\n');
    plfmta(s,&abort);
    fgets(s,163,f); 
    filter(s,'\n');
    plfmta(s,&abort);


    fgets(s1,163,f); 
    filter(s1,'\n');
    fgets(s6,163,f); 
    filter(s6,'\n');
    for(i=0;i<umaxdirs&&udir[i].subnum!=-1; i++) {
        dliscan1(udir[i].subnum);
        d=directories[udir[i].subnum];
        itoa(numf,s2,10);
        sprintf(s3,"Download%s",(d.mask & mask_no_uploads)?"":"/Upload");

        i1=0;
        if (thisuser.nscn[udir[i].subnum]>=0) i1=1;

        if(i1)
            stuff_in2(s,s1,noc2(d.name),40,udir[i].keys,2,s2,3,s3,15,"",0);
        else
            stuff_in2(s,s6,noc2(d.name),40,udir[i].keys,2,s2,3,s3,15,"",0);
        plfmta(s,&abort);
    }
    fgets(s1,163,f); 
    filter(s1,'\n');
    plfmta(s1,&abort);

    fclose(f);
    return 0;
}


void genner(char *fn,char *spec, int type)
{
    int i,i1,pts,pty,num=0;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],*b,*p;
    uploadsrec u;
    FILE *out;

    out=fopen(fn,"wt");

    fprintf(out,">>>File List of %s\n\n",syscfg.systemname);
    for (i=0;i<num_dirs; i++) {
        pts=1;
        if(type==2)
            if(!(directories[i].mask & mask_PD)) {
                pts=0;
            }

        if (pts) {
            if(type!=2) {
                pts=0;
                for(i1=0;i1<num_dirs&&i1<MAX_DIRS&&usub[i1].subnum!=-1;i1++) {
                    if(udir[i1].subnum==i1)
                        pts=1;
                }

                if(!pts)
                    continue;
            }

            dliscan1(i);
            pty=1;
            for (i1=1; (i1<=numf); i1++) {
                SETREC(i1);
                read(dlf,(void *)&u,sizeof(uploadsrec));

                if(!compare(spec,u.filename))
                    continue;

                if(type==1&&u.daten<nscandate)
                    continue;

                if (pty) {
                    fprintf(out,"\nArea: %s #%d [%d Files]\n\n",directories[i].name,i,numf);
                    pty=0;
                }
                if(u.ats[0]) {
                    strcpy(s1,stripfn(u.filename));
                    sprintf(s2,"%-12s [%4dk] ",s1,(u.numbytes+1023)/1024);
                    fputs(s2,out);
                    fputs(u.description,out);
                    fputs("\n",out);
                    if((u.mask & mask_extended)&&!exist("ass")) {
                        b=read_extended_description(u.filename);
                        p=strtok(b,"\n");
                        while(p!=NULL) {
                            strcpy(s2,p);
                            s2[strlen(s2)-1]=0;
                            fprintf(out,"                    %s \n",s2);
                            p=strtok(NULL,"\n");
                        }
                        farfree(b);
                    }
                }
                num++;
            }
            closedl();
        }
    }


    fprintf(out,"\n\nFiles For %s\n",syscfg.systemname);
    fprintf(out,"Files Total: %d\n",num);

    fclose(out);
}


void gofer(void)
{
    genner("gofer.lst","????????.???",2);
}

void listgen(void)
{
    int isnew,sent=0,i;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN];

    file_mask(s);
    npr("5New Files Only? ");
    isnew=yn();

    genner("filelist.dom",s,isnew);
    sprintf(s,"filelist.dom");
    add_arc("filelist","filelist.dom");
    sprintf(s1,"%s\\filelist.%s",cdir,xarc[ARC_NUMBER].extension);
    i=open(s1,O_RDONLY|O_BINARY);
    close(i);
    send_file(s1,(int *)&sent,(int *)&abort,s);
    if(sent)
        logtypes(4,"Downloaded File List");
    unlink("filelist.dom");
    unlink(s1);
}
