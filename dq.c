#ifdef QWK
#include "vars.h"
#pragma hdrstop
#include "qwk.h"


void bug(char *s)
{
#ifdef DEBUG
    /*    FILE *f;
        f=fopen("shit","at");
        fputs(s,f);
        fputc('\n',f);
        fclose(f);*/
    puts(s);
#else
    if(s);
#endif
}

void createcontrol(void);

char * write_line (char **text,unsigned int linelen,char ctla);

int xreadqwkmsg (FILE *fp,QWKHDR *qwh,char **hold, long *ll)
{

    QWKHDR    hdr;
    time_t    t;
    struct tm *tm;
    long      size,pos,len;
    int error=0,rep=1;
    char *p;

    *hold = NULL;


    /* get header */

    bug("Doing Qwkreadhdr");

    if(!qwkreadhdr(fp,&hdr,rep)) {
        error = 1;    /* end of the line */
        bug("Error at qwkreadhdr!");
        return 1;
    }


    pos = ftell(fp);

    /* now get text */

    size = hdr.numchunks * (long)QWKBLKSIZE;
    if(!size) {
        error = 1;    /* possible? */
        return error;
    }

    bug("Allocating Memory");

    if(size > 65024L) size = 65024L;
    *hold = malloc((size_t)(size + 1L));
    if(!*hold) {
        error = 1;     /* fatal */
        return error;
    }

    if(!qwkreadblks(fp,*hold,(size_t)(size / (long)QWKBLKSIZE))) {
        bug("Error in qwkreadblks");
        farfree(*hold);
        *hold = NULL;
        fseek(fp,pos + (hdr.numchunks * (long)QWKBLKSIZE),SEEK_SET);
        error = 1;    /* maybe not fatal */
        return 1;
    }
    fseek(fp,pos + (hdr.numchunks * (long)QWKBLKSIZE),SEEK_SET);

    rstrip(*hold);
    len = strlen(*hold);

    /* "treat" the message */

    bug("Treating message");

    p = *hold;
    while (*p) {
        if(*p == '\x8d' || *p == '\n') {
            bug("doing Memmove");
            memmove(p,&p[1],len - ((unsigned int)p - (unsigned int)*hold));
            len--;
            if(!len) break;
            continue;
        }
        else p++;
    }

    if(*(p - 1) != '\r' && p > *hold) {
        *p = '\r';
        p[1] = 0;
    }

    if(len==0) {
        farfree(*hold);
        *hold = NULL;
        error = 1;    /* maybe not fatal */
    }

    *qwh = hdr;
    *ll = len;
    return error;
}


void qwkwmsg(QWKHDR qwk,char *b,long len)
{
    messagerec m;
    postrec p;
    char s[121], s1[41];
    int i, dm, a, f,bn;
    long len1, len2, r, w;
    char *b1;

    bug("Qwkwmsg");

    sprintf(s, "%smsgtmp", syscfg.tempdir);
    i = open(s, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    write(i, b, len);

    f = 26;
    write(i, &f, 1);
    close(i);
    farfree(b);

    m.storage_type = 2;

    use_workspace = 1;
    strcpy(irt, qwk.subj);
    if(!qwk.to[0])
        strcpy(qwk.to,"All");
    strcpy(irt_name, qwk.to);
    strcpy(irt_from, qwk.from);
    fidotoss = 1;
    a = 0;
    bn=qwk.confnum-1;
    fiscan(bn);

    npr("Adding '%s' to %s\r\n",irt,subboards[bn].name);
    inmsg(&m, p.title, &a, 1, (subboards[bn].filename), 0);

    if (m.stored_as != 0xffffffff) {
        p.anony = a;
        p.msg = m;
        p.ownersys = 1;
        p.owneruser = 1;
        p.qscan = status.qscanptr++;
        time((long *) (&p.daten));
        p.status = 0;
        if (nummsgs >= subboards[bn].maxmsgs) {
            i = 1;
            dm = 0;
            while ((dm == 0) && (i <= nummsgs)) {
                if ((msgs[i].status & status_no_delete) == 0)
                    dm = i;
                ++i;
            }
            if (dm == 0)
                dm = 1;
            delete(dm);
        }
        msgs[++nummsgs] = p;
        ++status.msgposttoday;
    }
    fsavebase(bn);
}


void qwkreply(void)
{

    FILE *qwk;
    int area;
    char *hold,s1[81],s[81],s2[81];
    QWKHDR p;
    long len,fl;
    int i;


    if(outcom) {
        remove_from_temp("*.*",syscfg.tempdir,0);
        dtitle("QWK Reply Upload");
        nl();
        sprintf(s1,"%s%s.rep",syscfg.tempdir,nifty.menudir);
        receive_file(s1,&i,s,0);
    } 
    else {
        dtitle("Local QWK Import");
        nl();
        inputdat("Enter full Packet pathname",s1,76,0);
    }

    if(!exist(s1))
        return;

    sprintf(s,"%s.msg",nifty.menudir);

    unarc(s1,s);
    if(!exist(s)) {
        npr("Couldn't find %s!\r\n",s);
        return;
    }

    dtitle("Successfully Received Packet: Processing");

    qwk=flopen(s,"rb",&fl);

    fseek(qwk,128L,SEEK_SET);

    while(!feof(qwk)&&ftell(qwk)<fl) {
        if(xreadqwkmsg(qwk,&p,&hold,&len)==0) {
            qwkwmsg(p,hold,len);
        }
    }

    fclose(qwk);
}


void xwriteqwkmsg (FILE *fp,FILE *idx,long len,char *hold,char name[25],char title[25],char to[25],int msgnum,int area)
{
    QWKHDR hdr;
    long pos,l1;
    char s[81];
    long bpos;
    QWKIDX i;
    float ff;

    hold[len]=0;

    pos = ftell(fp);
    strncpy(hdr.from,name,25);
    strncpy(hdr.to,to,25);
    strncpy(hdr.subj,title,25);
    hdr.subj[25] = hdr.to[25] = hdr.from[25] = 0;
    hdr.status = QWKPUBUNREAD;
    hdr.msgnum = msgnum;
    hdr.confnum = area+1;
    hdr.date = time(NULL);
    *hdr.pword = 0;
    hdr.repnum = 0;
    hdr.numchunks = (long)(len / QWKBLKSIZE) + ((len % QWKBLKSIZE) != 0);
    hdr.live = 1;

    if(!qwkwritehdr(fp,&hdr,0)) {
        bug("Error Writing Header");
        return;
    }

    qwkwriteblks(fp,&hold[0]);

    bug("done write_lining");

    hdr.numchunks = (long)((len / 128) + ((len % 128) != 0));

    bpos = ftell(fp);
    fseek(fp,pos,SEEK_SET);
    qwkwritehdr(fp,&hdr,0);
    fseek(fp,bpos,SEEK_SET);

    ff = (float)((pos / 128.0));
    ff += 1.0;
    i.recnum=IEEEToMSBIN(ff);
    fwrite(&i,sizeof(QWKIDX),1,idx);

    farfree(hold);
    bug("Done almost completely");
}


void writeqwk(char *b, long len, char title[81],FILE *msd,FILE *idx,int msgnum,int area)
{
    char n[41],to[41],d[41];
    int p,p1,p2,cur;

    p=p1=p2=0;
    while ((b[p]!=13) && ((long)p<len) && (p<60))
        n[p]=b[p++];
    n[p]=0;
    ++p;


    while ((b[p+p1]!=13) && ((long)p+p1<len) && (p1<60))
        d[p1]=b[(p1++)+p];
    d[p1]=0;
    p1++;


    while ((b[p+p1+p2]!=13) && ((long)p+p1+p2<len) && (p2<60))
        to[p2]=b[(p2++)+p1+p];
    to[p2]=0;
    cur=p+p1+p2;


    bug("Xwriting Message from before part");
    xwriteqwkmsg(msd,idx,len-cur,&b[cur],n,title,to,msgnum,area);
    bug("done with xwriteqwkmsg");
}

void makeqwk(int type)
{
    int i,c=0,bn,skip=0,abort=0,sw,stop;
#ifdef DEBUG
    int total=0;
#endif
    char s[81],*b,s1[81],s2[81],f,s3[161];
    long len;
    FILE *msd,*idx;


    dtitle("Creating QWK Packet");

    ARC_NUMBER=0;

    sprintf(s,"%s.qwk",nifty.menudir);
    remove_from_temp("*.ndx","",0);
    remove_from_temp("messages.dat","",0);
    remove_from_temp("control.dat","",0);
    remove_from_temp(s,"",0);


    sprintf(s,"messages.dat");
    msd=fopen(s,"wb");

    createcontrol();

    //listgen(1);


    qwkwriteblk(msd,"Not produced by Qmail, no Copyright (c) 1987 and No Rights Reserved.");

    strcpy(s2,conf[curconf].flagstr);
    strcpy(conf[curconf].flagstr,"@");

    changedsl();
    topdata=0;
    topscreen();

    for(c=0;c<200&&usub[c].subnum!=-1&&!abort;c++) {
        bn=usub[c].subnum;
        if (thisuser.qscn[usub[c].subnum]<0)
            continue;
        i=1;
        fiscan(bn);
        while ((i<=nummsgs) && (msgs[i].qscan<=thisuser.qscn[bn]))
            ++i;
        sw=i;
        skip=0;

        if(type=='S') {
            nl();
            npr("%d New Messages on %s\r\n",(nummsgs-sw)+1,subboards[bn].name);
            outstr("5Pack it? 5(3Y3es, 3N3o, 3A3ll,3S3top5) 0");
            f=onek("YNAS\r");
            if(f=='N') skip=1;
            if(f=='A') type=0;
            if(f=='S') { 
                abort=1; 
                skip=1; 
            }
        }

        if(!skip) {
            npr("5Scanning 0%-40s 3(3%3d New3)\r\n",subboards[bn].name,(nummsgs-sw)+1);
            sprintf(s,"%03d.ndx",bn);
            idx=fopen(s,"wb");

            stop=0;
            while((i<=nummsgs) && (!stop)) {
#ifdef DEBUG
                printf("%d, total=%d\n",i,total++);
#endif
                bug("Before file read");
                b=readfile(&msgs[i].msg,subboards[bn].filename,&len);
                bug("Before writeqwk");
                if(b!=NULL)
                    writeqwk(b,len,msgs[i].title,msd,idx,i,bn);
                else
                    stop=1;
                i++;
                bug("After it all");
            }
            fclose(idx);
        }
    }

    fclose(msd);
    nl();
    pl("Done Scanning, Now Compressing");


    if(!abort) {
        ARC_NUMBER=0;
        genner(1,"????????.???");
        add_arc(nifty.menudir,"*.ndx messages.dat control.dat newfiles.dat");

        strcpy(s1,nifty.menudir);
        strcat(s1,".");
        strcat(s1,xarc[ARC_NUMBER].extension);

        strcpy(s,nifty.menudir);
        strcat(s,".qwk");
        rename(s1,s);

        if(outcom) {
            i=open(s,O_BINARY|O_RDWR);
            npr("5Packet is %ld bytes.  Transfer? ",filelength(i));
            close(i);
            if(ny())
                send_file(s,&i,&i,s1);
        } 
        else
            npr("\r\nPacket %s created\r\n",s);
        ex("MQ","");
    }

    remove_from_temp("*.ndx","",0);
    remove_from_temp("messages.dat","",0);
    remove_from_temp("control.dat","",0);
    if(outcom)
        remove_from_temp(s,"",0);

    strcpy(conf[curconf].flagstr,s2);
    changedsl();
}


void putf(char *s,FILE *f)
{
    fputs(s,f); 
    fputs("\n",f);
}

typedef struct {
    char bbsid[8];
} 
qwkcrec;

qwkcrec qwkc;

void createcontrol(void)
{
    FILE *f;
    char *s,s1[81];
    struct time t;
    struct date d;
    int i,max;

    //    sprintf(s1,"%scontrol.dat",syscfg.tempdir);
    sprintf(s1,"control.dat");


    f=fopen(s1,"wt");

    putf(syscfg.systemname,f);
    putf("Dominion Land",f);
    putf(syscfg.systemphone,f);
    putf(syscfg.sysopname,f);

    sprintf(s,"00000,%s",nifty.menudir);
    putf(s,f);

    getdate(&d);
    gettime(&t);
    sprintf(s,"%02d-%02d-%d %02d:%02d:%02d",d.da_mon,d.da_day,d.da_year,t.ti_hour,t.ti_min,t.ti_sec);
    putf(s,f);
    putf(thisuser.realname,f);
    putf("",f);
    putf("0",f);
    putf("0",f);

    max=0;
    for(i=0;i<64&&usub[i].subnum!=-1;i++)
        max++;
    sprintf(s,"%d",max-1);
    putf(s,f);

    for(i=0;i<64&&usub[i].subnum!=-1;i++) {
        sprintf(s,"%d",i);
        putf(s,f);
        putf(subboards[usub[i].subnum].name,f);
    }

    putf("HELLO",f);
    putf("NEWS",f);
    putf("GOODBYE",f);
    fclose(f);
}
#endif
