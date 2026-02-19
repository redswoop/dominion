#include "vars.h"
#pragma hdrstop

#include <time.h>
#include <dir.h>

extern int INDENTION;
#define MAX_LINES 15
#define FSED_OK (0)
#define CHECK_FOR_EXISTANCE


#define SETREC(i)  lseek(dlf,((long) (i))*((long)sizeof(uploadsrec)),SEEK_SET);

void add_extended_description(char *fn, char *desc)
{
    ext_desc_type ed;

    strcpy(ed.name,fn);
    ed.len=strlen(desc);
    lseek(edlf,0L,SEEK_END);
    write(edlf,&ed,sizeof(ext_desc_type));
    write(edlf,desc,ed.len);
}


void delete_extended_description(char *fn)
{
    ext_desc_type ed;
    long r,w,l1;
    char *ss=NULL;

    r=w=0;
    if ((ss=malloca(10240L))==NULL)
        return;
    l1=filelength(edlf);
    while (r<l1) {
        lseek(edlf,r,SEEK_SET);
        read(edlf,&ed,sizeof(ext_desc_type));
        if (ed.len<10000) {
            read(edlf,ss,ed.len);
            if (strcmp(fn,ed.name)) {
                if (r!=w) {
                    lseek(edlf,w,SEEK_SET);
                    write(edlf,&ed,sizeof(ext_desc_type));
                    write(edlf,ss,ed.len);
                }
                w +=(sizeof(ext_desc_type) + ed.len);
            }
        }
        r += (sizeof(ext_desc_type) + ed.len);
    }
    farfree(ss);
    chsize(edlf,w);
}


char *read_extended_description(char *fn)
{
    ext_desc_type ed;
    long l,l1;
    char *ss=NULL;

    l=0;
    l1=filelength(edlf);
    while (l<l1) {
        lseek(edlf,l,SEEK_SET);
        l += (long) read(edlf,&ed,sizeof(ext_desc_type));
        if (strcmp(fn,ed.name)==0) {
            ss=malloca((long) ed.len+10);
            if (ss) {
                read(edlf,ss,ed.len);
                ss[ed.len]=0;
            }
            return(ss);
        } 
        else
            l += (long) ed.len;
    }
    return(NULL);
}

int print_extended(char *fn, int *abort, unsigned char numlist, int indent)
{
    char *ss;
    int next=0;
    unsigned char numl=0;
    int cpos=0;
    char ch,s[81],s1[10],col=curatr;
    int i,mcih=mciok;

    mciok=0;
    nl();

    ss=read_extended_description(fn);
    if (ss) {
        ch=10;
        while ((ss[cpos]) && (!(*abort)) && (numl<numlist)) {
            if(lines_listed>screenlinest) {
                lines_listed=0;
                pauseline(0,abort);
            }
            if ((ch==10) && (indent)) {
                if (okansi())
                    sprintf(s,"\x1b[%dC",INDENTION);
                else {
                    for (i=0; i<INDENTION; i++)
                        s[i]=32;
                    s[INDENTION]=0;
                }
                makeansi(col,s1,0);
                outstr(s1);
                osan(s,abort,&next);
            }
            outchr(ch=ss[cpos++]);
            checka(abort,&next,0);
            if (ch==10)
                ++numl;
            else
                if ((ch!=13) && (wherex()>=79)) {
                //          osan("\r\n",abort,&next);
                numl++;
            }
        }
        if (wherex())
            nl();
    }
    farfree(ss);
    mciok=mcih;
    return(numl);
}

int count_extended(char *fn)
{
    char *ss;
    unsigned char numl=0;
    char ch;
    int i;
    long cpos=0;

    ss=read_extended_description(fn);
    if (ss) {
        while (ss[cpos]) {
            ch=ss[cpos++];
            if (ch==10)
                ++numl;
        }
    }
    farfree(ss);
    return(numl);
}




void modify_extended_description(char **sss)
{
    char s[161],s1[161];
    int f,ii,i,i1,i2;

    if (*sss)
        ii=1;
    else
        ii=0;
    do {
        if (ii) {
            nl();
            prt(5,"Enter a new extended description? ");
            if (!yn())
                return;
        } 
        else {
            nl();
            prt(5,"Enter an extended description? ");
            if (!yn())
                return;
        }
        {
            if (*sss)
                farfree(*sss);
            if ((*sss=malloca(10240))==NULL)
                return;
            *sss[0]=0;
            i=1;
            nl();
            sprintf(s,"Enter up to %d lines, %d chars each.",MAX_LINES,78-INDENTION);
            pl(s);
            nl();
            s[0]=0;
            i1=thisuser.screenchars;
            if (thisuser.screenchars>(76-INDENTION))
                thisuser.screenchars=76-INDENTION;
            do {
                ansic(2);
                npr("%d: ",i);
                ansic(0);
                s1[0]=0;
                inli(s1,s,90,1);
                i2=strlen(s1);
                if (i2 && (s1[i2-1]==1))
                    s1[i2-1]=0;
                if (s1[0]) {
                    strcat(s1,"\r\n");
                    strcat(*sss,s1);
                }
            } 
            while ((i++<MAX_LINES) && (s1[0]));
            thisuser.screenchars=i1;
            if (*sss[0]==0) {
                farfree(*sss);
                *sss=NULL;
            }
        }
        prt(5,"Is this what you want? ");
        i=!yn();
        if (i) {
            farfree(*sss);
            *sss=NULL;
        }
    } 
    while (i);
}


int getbytes(FILE *dev) {
    int byte1;
    int byte2;
    int result;

    byte1 = getc(dev);
    byte2 = getc(dev);

    result = (byte2 << 8) | byte1;

    return result;

}


void addgif(uploadsrec *u, char *path)
{
    int byte1, bits_per_pix,colors,i;
    unsigned int width;
    unsigned int height;
    FILE *in;
    char s[81],*ss,*ss1,fn[81],s1[81];

    sprintf(fn,"%s%s",path,stripfn(u->filename));
    in = fopen (fn, "rb");
    for (i = 0; (i < 6); i++) s[i] = getc(in);
    s[6] = '\0';
    width = getbytes(in);
    height = getbytes(in);
    byte1 = getc(in);
    fclose(in);

    if((strcmp(s,"GIF87a")==0)||(strcmp(s,"GIF89a")==0));
    else {
        logtypes(3,"%s is not a GIF file, skipping",fn);
        return;
    }
    bits_per_pix = byte1 & 0x07;
    bits_per_pix++;
    colors = 1 << bits_per_pix;

    sprintf(s,"[%dx%dx%d]",width,height,colors);
    if(strlen(s)+strlen(u->description)>39) {
        pl("Adding to Extended description");
        if(u->mask & mask_extended) {
            ss=read_extended_description(fn);
            ss1=malloca(sizeof(ss)+30);
            strcpy(ss1,ss);
            strcat(ss1,"\r");
            strcat(ss1,s);
            strcat(ss1,"\r");
            farfree(ss);
            farfree(ss1);
        } 
        else {
            strcat(s,"\r");
            add_extended_description(u->filename,s);
        }
    } 
    else {
        sprintf(s1,"%s %s",u->description,s);
        strcpy(u->description,s1);
    }
    pl(s1);
}

int finddup(char *fn,int quiet)
{
    int i,i1;
    uploadsrec u;

    if(!quiet) {
        pl(get_string(54));
        nl();
    }
    closedl();
    for (i=0; (i<num_dirs) && (!hangup);i++) {
        dliscan1(i);
        for(i1=1;i1<=numf;i1++) {
            SETREC(i1);
            read(dlf,&u,sizeof(uploadsrec));
            if(stricmp(u.filename,fn)==0)
                return 1;
        }
        closedl();
    }
    return(0);
}

void verify_hangup()
{
    int i;

    npr("7Commencing with Automatic Logoff\r\n0Hit Anykey to abort.\r\n");
    for(i=19;i>0&&!inkey();i--) {
        npr("%2d Seconds till Automatic Disconnection.\r",i);
        delay(1000);
    }
    if(i<=0) {
        nl();
        npr("Automatic Logoff Time Expired, Disconnecting");
        hangup=1;
        logtypes(3,"Automatic Logoff Time Expired, System Disconnected");
    }
}


void listbatch()
{
    char s[161],s1[161];
    int abort,i;

    abort=0;
    nl();
    sprintf(s,"Files - %d, Size - %-1.0fk, Time - %s",
    numbatch,batchsize,ctim(batchtime));
    dtitle(s);
    nl();
    for (i=0; (i<numbatch) && (!abort) && (!hangup); i++) {
        batch.batchdesc[0]=0;
        batrec(1,i);
        if (batch.sending) {
            sprintf(s1,"%5.0fk  %s",batch.size,ctim(batch.time));
            if(batch.dir==-2)
                npr("5<5%d5>0 %-13s: 9%-30s 1[%s]\r\n",i+1,batch.filename,s1,"Flagged Files");
            else
                npr("5<5%d5>0 %-13s: 9%-30s 1[%s]\r\n",i+1,batch.filename,s1,directories[batch.dir].name);
        } 
        else {
            npr("3<3%d3>0 %-13s: 5%-30.30s 1[%s]\r\n",i+1,batch.filename,
            batch.batchdesc,directories[batch.dir].name);
        }
    }
}


void batchdled(int stay)
{
    int i,done=0;
    char s[81],c;

    do {
        nl();
        outstr(get_string(55));
        c=onek("LCR\r");
        switch(c) {
        case '\r': 
            done=1; 
            break;
        case 'L':
            listbatch();
            break;
        case 'R':
            nl();
            if (numbatch==0)
                pl("No files in batch queue.");
            else {
                prt(5,"Remove which? ");
                input(s,2);
                i=atoi(s);
                if ((i>0) && (i<=numbatch))
                    delbatch(i-1);
            }
            break;
        case 'C':
            prt(5,"Clear queue? ");
            if (yn()) {
                while (numbatch>0)
                    delbatch(0);
                nl();
                pl("Queue cleared.");
                batchtime=0.0;
                numbatch=numbatchdl=0;
            }
        }
        if(!stay) done=1;
    } 
    while(!done);

    if(thisuser.helplevel==2) pausescr();
}

void copyupfile(char fn[12],char todir[81],char fdir[81])
{
    int i,d1,d2;
    char *b,s[81],s1[81];
    long l;

    sprintf(s,"%s%s",fdir,fn);
    sprintf(s1,"%s%s",todir,fn);


    if (exist(s)) {
        d2=0;
        if ((s[1]!=':') && (s1[1]!=':'))
            d2=1;
        if ((s[1]==':') && (s1[1]==':') && (s[0]==s1[0]))
            d2=1;
        if (d2) {
            rename(s,s1);
            unlink(s);
        } 
        else {
            if((b=malloca(25*1024))==NULL) return;
            d1=open(s,O_RDONLY | O_BINARY);
            d2=open(s1,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
            l=filelength(d1);
            i=read(d1,(void *)b,25*1024);
            mpl(l/(25*1024));
            while (i>0) {
                write(d2,(void *)b,i);
                i=read(d1,(void *)b,25*1024);
                outchr('o');
            }
            close(d1);
            close(d2);

            farfree(b);
        }
    }
}

char *stripfn(char *fn)
{
    static char ofn[15];
    int i,i1;
    char s[81];

    i1=-1;
    for (i=0; i<strlen(fn); i++)
        if ((fn[i]=='\\') || (fn[i]==':') || (fn[i]=='/'))
            i1=i;
    if (i1!=-1)
        strcpy(s,&(fn[i1+1]));
    else
        strcpy(s,fn);
    for (i=0; i<strlen(s); i++)
        if ((s[i]>='A') && (s[i]<='Z'))
            s[i]=s[i]-'A'+'a';
    i=0;
    while (s[i]!=0) {
        if (s[i]==32)
            strcpy(&s[i],&s[i+1]);
        else
            ++i;
    }
    strcpy(ofn,s);
    return(ofn);
}


void stripfn1(char *fn)
{
    int i,i1;
    char s[81],s1[81];

    i1=0;
    for (i=0; i<strlen(fn); i++)
        if ((fn[i]=='\\') || (fn[i]==':') || (fn[i]=='/'))
            i1=i;
    strcpy(s1,fn);
    if (i1) {
        strcpy(s,&(fn[i1+1]));
        s1[i1+1]=0;
    } 
    else {
        strcpy(s,fn);
        s1[0]=0;
    }

    for (i=0; i<strlen(s); i++)
        if ((s[i]>='A') && (s[i]<='Z'))
            s[i]=s[i]-'A'+'a';
    i=0;
    while (s[i]!=0) {
        if (s[i]==32)
            strcpy(&s[i],&s[i+1]);
        else
            ++i;
    }
    strcat(s1,s);
    strcpy(fn,s1);
}


int upload_file2(char *fn, int dn, char *desc)
{
    directoryrec d;
    uploadsrec u,u1;
    int i,i1,i2,ok,f;
    char s[81],s1[81],ff[81];
    long l;
    double ti;

    d=directories[dn];
    strcpy(s,fn);
    align(s);
    strcpy(u.filename,s);
    u.ownerusr=1;
    u.ownersys=0;
    u.numdloads=0;
    u.filetype=0;
    u.mask=0;
    {
        sprintf(ff,"%s%s",d.dpath,s);
        f=open(ff,O_RDONLY | O_BINARY);
        if (f<=0) {
            if (desc && (*desc)) {
                npr("ERR: %s: %s\r\n",fn,desc);
            } 
            else {
                npr("File '%s' doesn't exist.\r\n",fn);
            }
            return(1);
        }
        l=filelength(f);
        u.numbytes=l;
        close(f);
        strcpy(u.upby,"SysOp");
        strcpy(u.date,date());
        u.points=((u.numbytes)+1023)/10240;
        npr("3%s: %4ldk :",u.filename,((u.numbytes)+1023)/1024);
        if ((desc) && (*desc)) {
            strncpy(u.description,desc,39);
            u.description[39]=0;
            pl(u.description);
        } 
        else {
            strcpy(u.description,"No Description Given at Upload");
        }
        if (u.description[0]==0)
            return(0);
        time(&l);
        u.daten=l;

        if (strstr(u.filename,".GIF"))
            addgif(&u,d.dpath);
        comment_arc(stripfn(u.filename),d.dpath,d.upath);
        strcpy(ff,d.dpath);
        strcat(ff,stripfn(u.filename));
        adddiz(ff,&u);

        for (i=numf; i>=1; i--) {
            SETREC(i);
            read(dlf,(void *)&u1,sizeof(uploadsrec));
            SETREC(i+1);
            write(dlf,(void *)&u1,sizeof(uploadsrec));
        }
        SETREC(1);
        write(dlf,(void *)&u,sizeof(uploadsrec));
        ++numf;
        SETREC(0);
        read(dlf, &u1, sizeof(uploadsrec));
        u1.numbytes=numf;
        u1.daten=l;
        SETREC(0);
        write(dlf,(void *)&u1,sizeof(uploadsrec));
    }
    return(1);
}

int maybe_upload(char *fn, int dn, char *desc)
{
    char s[81];
    int ok=1;
    uploadsrec u;

    strcpy(s,fn);
    align(s);

    if (!upload_file2(s,dn,desc))
        ok=0;
    return(ok);
}

void fdnupload_files(char *fn, int dn, int type)
/* This assumes the file holds listings of files, one per line, to be
 * uploaded.  The first word (delimited by space/tab) must be the filename.
 * after the filename are optional tab/space separated words (such as file
 * size or date/time).  After the optional words is the description, which
 * is from that position to the end of the line.  the "type" parameter gives
 * the number of optional words between the filename and description.
 * the optional words (size, date/time) are ignored completely.
 */
{
    unsigned char s[255];
    char *fn1,*desc;
    FILE *f;
    int ok=1,abort=0,ok1,i;

    dliscan1(dn);

    f=fopen(fn,"r");
    if (!f) {
        npr("\r\nFile '%s' not found.\r\n\r\n",fn);
    } 
    else {
        while (ok && fgets(s,250,f)) {
            if ((s[0]<=32) || (s[0]>127))
                continue;
            ok1=0;
            fn1=strtok(s," \t\n");
            if (fn1) {
                ok1=1;
                for (i=0; ok1 && (i<type); i++)
                    if (strtok(NULL," \t\n")==NULL)
                        ok1=0;
                if (ok1) {
                    desc=strtok(NULL,"\n");
                    if (!desc)
                        ok1=0;
                }
            }
            if (ok1) {
                while ((*desc==' ') || (*desc=='\t'))
                    ++desc;
                ok=maybe_upload(fn1,dn,desc);
                if (abort)
                    ok=0;
            }
        }
        fclose(f);
    }

    closedl();
}


void fdnupload_files(char *fn, int dn, int type);

int fdnfilenet(void)
{
    int i;
    char s[81];

    for(i=0;i<num_dirs;i++) {
        if(directories[i].mask & mask_FDN) {
            npr("Updating %-30.30s [%2d]\r\n", directories[i].name, i);
            sprintf(s,"%sfiles.bbs",directories[i].dpath);
            if(exist(s)) {
                fdnupload_files(s,i,0);
                unlink(s);
            }
        }
    }
    return 0;
}
