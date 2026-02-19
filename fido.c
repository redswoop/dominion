#include "vars.h"
#pragma hdrstop

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

void fiscan(int b)
{
    int f;
    char s[81];

    sprintf(s, "%s%s.SUB", syscfg.datadir, subboards[b].filename);
    f = open(s, O_BINARY | O_RDWR);
    if (f == -1) {
        f = open(s, O_BINARY | O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
        msgs[0].owneruser = 0;
        write(f, (void *) (&msgs[0]), sizeof(postrec));
    }
    lseek(f, 0L, SEEK_SET);
    nummsgs = (read(f, (void *) (&msgs[0]), 512 * sizeof(postrec)) / sizeof(postrec)) - 1;
    nummsgs = msgs[0].owneruser;
    close(f);
}

void fsavebase(int b)
{
    int f;
    char s[81];

    sprintf(s, "%s%s.SUB", syscfg.datadir, subboards[b].filename);
    f = open(s, O_BINARY | O_RDWR);
    lseek(f, 0L, SEEK_SET);
    msgs[0].owneruser = nummsgs;
    write(f, (void *) (&msgs[0]), ((nummsgs + 1) * sizeof(postrec)));
    close(f);
    if (nummsgs) {
        sub_dates[b] = msgs[nummsgs].qscan;
    } 
    else {
        sub_dates[b] = 1;
    }
}

int checkend(char c)
{
    if (c == '\r' || c == '\n')
        return 1;
    return 0;
}


int tossmsg(int bn, char title[72], char to[36], char from[36], char *b, long len)
{
    messagerec m;
    postrec p,p1;
    char s[121], s1[41];
    int i, dm, a, f,i1;
    long len1, len2, r, w;
    char *b1;
    FILE *log;

    b1 = malloc(len);
    r = w = 0;

    while (b[r]) {
        if (b[r] == 1) {
            while (!checkend(b[r]) && r < len)
                r++;
            b1[w] = 0;
        } 
        else if(b[r]=='') {
            r++;
            b1[w++]=13;
        } 
        else {
            b1[w++] = b[r++];
        }
    }

    b1[w] = 0;
    len = w;

    sprintf(s, "%smsgtmp", syscfg.tempdir);
    i = open(s, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    write(i, b1, len);

    f = 26;
    write(i, &f, 1);
    close(i);
    farfree(b);
    farfree(b1);


    m.storage_type = 2;

    use_workspace = 1;
    strcpy(irt, title);
    if(!irt[0])
        strcpy(irt,"Untitled");
    if(!to[0])
        strcpy(to,"All");
    strcpy(irt_name, rnam(to));
    strcpy(irt_from, rnam(from));
    if(!irt_from[0])
        strcpy(irt_from,"Network");
    fidotoss = 1;
    a = 0;
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
            for(i1=0;i1<5;i1++) {
                i = 1;
                dm = 0;
                while ((dm == 0) && (i <= nummsgs)) {
                    if ((msgs[i].status & status_no_delete) == 0)
                        dm = i;
                    ++i;
                }
                if (dm == 0)
                    dm = 1;
                if ((dm>0) && (dm<=nummsgs)) {
                    p1=msgs[dm];
                    remove_link(&p1.msg,(subboards[bn].filename));
                    for (i=dm; i<nummsgs; i++)
                        msgs[i]=msgs[i+1];
                    nummsgs--;
                }
            }
        }

        msgs[++nummsgs] = p;
        ++status.msgposttoday;
    }
    return 0;
}

int fidotosser(int bn)
{
    int i, os, i2 = 1, i1, total = 0;
    char s[81], s1[81], c;
    long len;
    char *b;
    fmsgrec f;


    if (subboards[bn].attr & mattr_netmail)
        sprintf(s, "%s\\HI-WATER.MRK", subboards[bn].nmpath);
    else
        sprintf(s, "%s%s\\HI-WATER.MRK", syscfg.msgsdir, subboards[bn].filename);
    i = open(s, O_BINARY | O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (i < 0) {
        i = open(s, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
        c = 1;
        write(i, &c, sizeof(c));
        i2 = 2;
    } 
    else {
        read(i, &i2, 1);
    }
    close(i);
    i = 1;

    if (subboards[bn].attr & mattr_netmail)
        sprintf(s, "%s\\%d.msg", subboards[bn].nmpath, i2 + 1);
    else
        sprintf(s, "%s%s\\%d.msg", syscfg.msgsdir, subboards[bn].filename, i2 + 1);
    if (!exist(s))
        i = 0;

    if (i)
        fiscan(bn);

    while (i) {
        if (subboards[bn].attr & mattr_netmail)
            sprintf(s, "%s\\%d.msg", subboards[bn].nmpath, i2 + 1);
        else
            sprintf(s, "%s%s\\%d.msg", syscfg.msgsdir, subboards[bn].filename, i2 + 1);
        i1 = open(s, O_BINARY | O_RDWR);
        if (i1 > -1) {
            read(i1, &f, sizeof(fmsgrec));
            len = filelength(i1) - sizeof(fmsgrec);
            b = malloca(len);
            if (b == NULL) {
                pl("Not Enough Memory");
                return 1;
            }
            read(i1, b, len);
            close(i1);
            tossmsg(bn, f.title, f.to, f.from, b, len);
            total++;
            i2++;
        } 
        else
            i = 0;
    }
    npr("%3d Msgs Tossed", total);

    if (total) {
        fsavebase(bn);
        if (subboards[bn].attr & mattr_netmail)
            sprintf(s, "%s\\HI-WATER.MRK", subboards[bn].nmpath);
        else
            sprintf(s, "%s%s\\HI-WATER.MRK", syscfg.msgsdir, subboards[bn].filename);
        i = open(s, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
        write(i, &i2, 1);
        close(i);
    }
    return 0;
}


void writefmsg(char *b, long len, char *title, char *fn, subboardrec sb)
{
    char s1[161], s2[128], ch1, n[41], to[41], d[41], *pp;
    long p, p1, p2, cur, i1, r, w, p3, attr;
    fmsgrec f;
    FILE *c;
    struct date dd;
    struct time tt;
    char *dats[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", ""        };
    addressrec add, toadd;
    originrec ors;

    getorigin(sb.origin,&ors);




    strcpy(s2, fn);

    p = p1 = p2 = p3 = 0;
    while ((b[p] != 13) && ((long) p < len) && (p < 60))
        n[p] = b[p++];
    n[p] = 0;
    ++p;


    while ((b[p + p1] != 13) && ((long) p + p1 < len) && (p1 < 60))
        d[p1] = b[(p1++) + p];
    d[p1] = 0;
    p1++;


    while ((b[p + p1 + p2] != 13) && ((long) p + p1 + p2 < len) && (p2 < 60))
        to[p2] = b[(p2++) + p1 + p];
    to[p2] = 0;



    if (sb.attr & mattr_netmail) {
        p2++;
        while ((b[p + p1 + p2 + p3] != 13) && ((long) p + p1 + p2 + p3 < len) && (p3 < 60))
            s1[p3] = b[(p3++) + p2 + p1 + p];
        s1[p3] = 0;

        pp = strtok(s1, ":");
        toadd.zone = atoi(pp);
        pp = strtok(NULL, "/");
        toadd.net = atoi(pp);
        pp = strtok(NULL, "");
        toadd.node = atoi(pp);

        attr = 387;
        /* 
                           * attr=0; attr |= 0; attr |= 1; attr |= 0x04; attr |= 0x08; 
                           */

    } 
    else {
        toadd.zone = 0;
        toadd.net = 0;
        toadd.node = 0;
        attr = 256;
    }

    cur = p + p1 + p2 + p3;

    if (sb.add.zone)
        add.zone = sb.add.zone;
    else
        add.zone = ors.add.zone;
    if (sb.add.net)
        add.net = sb.add.net;
    else
        add.net = ors.add.net;
    if (sb.add.node)
        add.node = sb.add.node;
    else
        add.node = ors.add.node;


    strncpy(f.to, to, 36);
    strncpy(f.from, n, 36);
    strncpy(f.title, title, 72);
    getdate(&dd);
    gettime(&tt);
    dd.da_year -= 1900;

    sprintf(s1, "%02d %3s %d  %02d:%02d:%02d", dd.da_day, dats[dd.da_mon - 1], dd.da_year,
    tt.ti_hour, tt.ti_min, tt.ti_sec);
    strncpy(f.date, s1, 20);
    f.timesread = 0;
    f.destnode = toadd.node;
    f.orignode = add.node;
    f.cost = 0;
    f.orignet = add.net;
    f.destnet = toadd.net;
    strcpy(f.res, "");
    f.replyto = 0;
    f.attrib = attr;
    f.nextreply = 0;


    c = fopen(s2, "wb");
    fwrite(&f, sizeof(fmsgrec), 1, c);
    fwrite(&b[cur], len - cur, 1, c);

    sprintf(s1, "\r\r--- %s\r", wwiv_version);
    fputs(s1, c);

    sprintf(s1, " * Origin: %-.60s (%d:%d/%d)\r", ors.origin, add.zone, add.net, add.node);
    fputs(s1, c);

    fclose(c);

    farfree(b);
}



int fidoscan(int bn, int *tot)
{
    int i, os, i1, f, total = 0;
    char s[81], s1[81], c, i2 = 1;
    long len;
    char *b;

    if (subboards[bn].attr & mattr_netmail)
        sprintf(s, "%s\\HI-WATER.MRK", subboards[bn].nmpath);
    else
        sprintf(s, "%s%s\\HI-WATER.MRK", syscfg.msgsdir, subboards[bn].filename);
    f = open(s, O_BINARY | O_RDWR);
    if (f < 0) {
        f = open(s, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
        c = 1;
        write(f, &c, sizeof(c));
        i2 = 1;
    } 
    else
        read(f, &i2, 1);
    close(f);
    i = 1;

    fiscan(bn);

    for (i1 = i; i1 <= nummsgs; i1++) {
        if (msgs[i1].status & status_pending_fido) {
            i2++;
            b = readfile(&msgs[i1].msg, subboards[bn].filename, &len);
            if (subboards[curlsub].attr & mattr_netmail) {
                sprintf(s, "%s\\%d.msg", subboards[bn].nmpath, i2);
            } 
            else
                sprintf(s, "%s%s\\%d.msg", syscfg.msgsdir, subboards[bn].filename, i2);
            writefmsg(b, len, noc2(msgs[i1].title), s, subboards[bn]);
            total++;
            msgs[i1].status ^= status_pending_fido;
        }
    }

    if (total)
        fsavebase(bn);

    npr("%d Msgs Scanned", total);
    if (total)
        *tot = 1;

    sprintf(s, "%s%s\\HI-WATER.MRK", syscfg.msgsdir, subboards[bn].filename);
    f = open(s, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    write(f, &i2, 1);
    close(f);

    return 0;
}

int mailsys(int m)
{
    int i = 0, i1, found = 0;
    char s[81];
    FILE *f;
    long l, l1;

    if(m=='F')
        return(fdnfilenet());

    if(m=='G') {
        gofer();
        return(0);
    }

    for (i = 0; i < MAX_SUBS; i++) {
        lines_listed=0;
        if(!(subboards[i].attr & mattr_deleted)) {
            if ((subboards[i].attr & mattr_fidonet)) {
                if (subboards[i].attr & mattr_netmail)
                    sprintf(s, "%s\\nul", subboards[i].nmpath);
                else
                    sprintf(s, "%s%s\\nul", syscfg.msgsdir, subboards[i].filename);
                if (!exist(s)) {
                    if (subboards[i].attr & mattr_netmail)
                        strcpy(s, subboards[i].nmpath);
                    else
                        sprintf(s, "%s%s", syscfg.msgsdir, subboards[i].filename);
                    if (mkdir(s)) {
                        pl("Error Creating Msg Path!");
                        logpr("7! 0Could not creat FidoNet msgpath 4%s", s);
                        continue;
                    }
                }

                switch (m) {

                case 'S':
                    num_listed = 0;
                    if (subboards[i].attr & mattr_fidonet) {
                        npr("Scanning %-30.30s [%d], ", subboards[i].name, i);
                        time(&l);
                        i1 = fidoscan(i, &found);
                        time(&l1);
                        npr(", %5.1f seconds\r\n", difftime(l1, l));
                        if (i1)
                            return (9);
                    }
                    break;
                case 'T':
                    num_listed = 0;
                    if (subboards[i].attr & mattr_fidonet) {
                        npr("Tosing %-30.30s [%2d], ", subboards[i].name, i);
                        time(&l);
                        i1 = fidotosser(i);
                        time(&l1);
                        npr(", %5.1f seconds\r\n", difftime(l1, l));
                        if (i1)
                            return (8);
                    }
                    break;
                case 'P':
                    num_listed = 0;
                    if (subboards[i].attr & mattr_fidonet) {
                        npr("Purging %s [%d]\r\n", subboards[i].name, i);
                        sprintf(s, "%s%s\\", syscfg.msgsdir, subboards[i].filename);
                        remove_from_temp("*.*", s, 0);
                        sprintf(s, "%s%s\\HI-WATER.MRK", syscfg.msgsdir, subboards[i].filename);
                        f = fopen(s, "wb");
                        fputc(1, f);
                        fclose(f);
                    }
                    break;
                }
            }
        }
    }
    save_status();
    if (m == 'S' && found) {
        i = open("DMRESCAN.NOW", O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        close(i);
    }
    return 0;
}

int runmailer()
{
    int i, ret, done = 0, result;
    fdrrec fdr[15];
    int numfdr;
    char s[81];

    sprintf(s,"%snetfdr.dat",syscfg.datadir);
    i=open(s,O_BINARY|O_RDWR|O_CREAT,S_IREAD|S_IWRITE);
    numfdr=(read(i,&fdr[0], (15*sizeof(fdrrec))))/sizeof(fdrrec);
    close(i);


    do {
        clrscrb();
        pl(get_string2(45));
        ret = runprog(fnet.mailer, 1);
        result = -1;
        if (ret == fnet.nlev)
            end_bbs(oklevel);
        if (ret == fnet.retlev)
            return 0;
        for (i = 0; i < numfdr; i++) {
            if (ret == fdr[i].level)
                result = i;
        }
        if (result >= 0) {
            if (fdr[result].attr & fdr_connect) {
                close_user();
                usernum = 0;
                incom = 1;
                outcom = 1;
                using_modem = 1;
                modem_speed = fdr[result].speed;
                com_speed = syscfg.baudrate[syscfg.primaryport];
                sprintf(curspeed, "%d", fdr[result].speed);
                done = 1;
                okskey = 1;
                clrscr();
                printf("Connection Established at: %s\n", curspeed);
            } 
            else if (fdr[result].attr & fdr_mail)
                runprog(fdr[result].fn, 0);
            else if (fdr[result].attr & fdr_local) {
                read_user(1, &thisuser);
                usernum = 1;
                reset_act_sl();
                com_speed = modem_speed = syscfg.baudrate[syscfg.primaryport];
                if ((syscfg.sysconfig & sysconfig_off_hook) == 0)
                    dtr(0);
                using_modem = 1;
                done = 1;
            } 
            else if (fdr[result].attr & fdr_cmdtype) {
                usernum = 1;
                read_user(1, &thisuser);
                reset_act_sl();
                com_speed = modem_speed = syscfg.baudrate[syscfg.primaryport];
                ex(fdr[result].fn, &fdr[result].fn[3]);
                pausescr();
            }
        } 
        else
            return 11;
    }
    while (!done);
    return 1;
}
