/*
 * dosconv.c - Convert DOS Dominion BBS data files to macOS format
 *
 * DOS (Borland C++ 3.1, 16-bit): int=2, long=4, no struct padding
 * macOS (arm64, gnu89):           int=4, long=8, natural alignment
 *
 * Usage: ./dosconv <dos_dir> <macos_dir>
 *   Reads DOS binary .dat files from dos_dir, writes macOS versions to macos_dir.
 *   Text files (.MSG, .ANS, .FMT, .MNU, etc.) are copied as-is.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "vardec.h"

/* ================================================================
 * DOS binary reading helpers — read little-endian values from buffer
 * ================================================================ */

static unsigned short rd_u16(const unsigned char *p)
{
    return p[0] | (p[1] << 8);
}

static unsigned long rd_u32(const unsigned char *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned long)p[3] << 24);
}

static float rd_float(const unsigned char *p)
{
    float f;
    memcpy(&f, p, 4);  /* same IEEE 754 format, same endianness on arm64 */
    return f;
}

static void rd_str(char *dst, const unsigned char *p, int len)
{
    memcpy(dst, p, len);
}

/* ================================================================
 * Read entire file into buffer
 * ================================================================ */

static unsigned char *read_file(const char *path, long *out_len)
{
    FILE *f = fopen(path, "rb");
    unsigned char *buf;
    long len;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc(len);
    if (buf) fread(buf, 1, len, f);
    fclose(f);
    if (out_len) *out_len = len;
    return buf;
}

static void copy_file(const char *src, const char *dst)
{
    FILE *in, *out;
    char buf[4096];
    size_t n;
    in = fopen(src, "rb");
    if (!in) return;
    out = fopen(dst, "wb");
    if (!out) { fclose(in); return; }
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        fwrite(buf, 1, n, out);
    fclose(in);
    fclose(out);
}

static void mkd(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
        mkdir(path, 0755);
}

/* ================================================================
 * DOS struct sizes (packed, no alignment, int=2, long=4)
 * ================================================================ */

#define DOS_SLREC_SIZE      14
#define DOS_VALREC_SIZE     8
#define DOS_ARCREC_SIZE     100
#define DOS_CONFIGREC_SIZE  5660
#define DOS_NIFTYREC_SIZE   450
#define DOS_STATUSREC_SIZE  182
#define DOS_SUBBOARDREC_SIZE 165
#define DOS_DIRECTORYREC_SIZE 243
#define DOS_PROTOCOLREC_SIZE 416
#define DOS_RESULTREC_SIZE  80
#define DOS_CONFREC_SIZE    100
#define DOS_XARCREC_SIZE    176
#define DOS_EXARC_SIZE      64
#define DOS_ADDRESSREC_SIZE 8
#define DOS_SMALREC_SIZE    33  /* name[31] + number(2) */

/* ================================================================
 * Convert slrec: DOS(14 bytes) -> macOS(24 bytes)
 * ================================================================ */

static void conv_slrec(slrec *dst, const unsigned char *p)
{
    dst->time_per_day   = rd_u16(p + 0);
    dst->time_per_logon = rd_u16(p + 2);
    dst->maxcalls       = rd_u16(p + 4);
    dst->emails         = rd_u16(p + 6);
    dst->posts          = rd_u16(p + 8);
    dst->ability        = rd_u32(p + 10);
}

/* ================================================================
 * Convert configrec: DOS(5660 bytes) -> macOS
 * ================================================================ */

static void conv_configrec(configrec *c, const unsigned char *p)
{
    int i, off;
    memset(c, 0, sizeof(configrec));

    /* char fields — identical layout through offset 937 */
    rd_str(c->newuserpw,       p + 0, 21);
    rd_str(c->systempw,        p + 21, 21);
    rd_str(c->msgsdir,         p + 42, 81);
    rd_str(c->gfilesdir,       p + 123, 81);
    rd_str(c->datadir,         p + 204, 81);
    rd_str(c->dloadsdir,       p + 285, 81);
    c->ramdrive              = p[366];
    rd_str(c->tempdir,         p + 367, 81);
    rd_str(c->resx,            p + 448, 84);
    rd_str(c->bbs_init_modem,  p + 532, 51);
    rd_str(c->answer,          p + 583, 21);
    rd_str(c->menudir,         p + 604, 105);
    rd_str(c->no_carrier,      p + 709, 21);
    rd_str(c->ring,            p + 730, 21);
    rd_str(c->terminal,        p + 751, 21);
    rd_str(c->systemname,      p + 772, 51);
    rd_str(c->systemphone,     p + 823, 13);
    rd_str(c->sysopname,       p + 836, 51);
    rd_str(c->executestr,      p + 887, 51);

    /* unsigned char fields */
    c->newusersl    = p[938];
    c->newuserdsl   = p[939];
    c->maxwaiting   = p[940];
    memcpy(c->comport, p + 941, 5);
    memcpy(c->com_ISR, p + 946, 5);
    c->primaryport  = p[951];
    c->newuploads   = p[952];
    c->closedsystem = p[953];

    /* unsigned short fields */
    c->systemnumber    = rd_u16(p + 954);
    for (i = 0; i < 5; i++) c->baudrate[i] = rd_u16(p + 956 + i*2);
    for (i = 0; i < 5; i++) c->com_base[i] = rd_u16(p + 966 + i*2);
    c->maxusers        = rd_u16(p + 976);
    c->newuser_restrict= rd_u16(p + 978);
    c->sysconfig       = rd_u16(p + 980);
    c->sysoplowtime    = rd_u16(p + 982);
    c->sysophightime   = rd_u16(p + 984);
    c->executetime     = rd_u16(p + 986);

    /* floats */
    c->req_ratio    = rd_float(p + 988);
    c->newusergold  = rd_float(p + 992);

    /* slrec[256] — DOS offset 996, each 14 bytes */
    for (i = 0; i < 256; i++)
        conv_slrec(&c->sl[i], p + 996 + i * DOS_SLREC_SIZE);

    /* valrec[10] — DOS offset 4580, each 8 bytes (identical layout) */
    off = 4580;
    for (i = 0; i < 10; i++) {
        c->autoval[i].sl  = p[off];
        c->autoval[i].dsl = p[off + 1];
        c->autoval[i].ar  = rd_u16(p + off + 2);
        c->autoval[i].dar = rd_u16(p + off + 4);
        /* restrict is a keyword in C99+ but a field in gnu89 */
        memcpy((char *)&c->autoval[i] + 6, p + off + 6, 2);
        off += DOS_VALREC_SIZE;
    }

    /* char fields */
    rd_str(c->hangupphone, p + 4660, 21);
    rd_str(c->pickupphone, p + 4681, 21);

    /* unsigned int (DOS: 2 bytes) */
    c->netlowtime  = rd_u16(p + 4702);
    c->nethightime = rd_u16(p + 4704);

    rd_str(c->connect_300_a, p + 4706, 105);

    /* arcrec[4] — DOS offset 4811, each 100 bytes (all char, identical) */
    for (i = 0; i < 4; i++)
        memcpy(&c->arcs[i], p + 4811 + i * DOS_ARCREC_SIZE, DOS_ARCREC_SIZE);

    rd_str(c->beginday_c, p + 5211, 51);
    rd_str(c->logon_c, p + 5262, 51);

    /* int fields (DOS: 2 bytes) */
    c->userreclen     = (int)(short)rd_u16(p + 5313);
    c->waitingoffset  = (int)(short)rd_u16(p + 5315);
    c->inactoffset    = (int)(short)rd_u16(p + 5317);

    rd_str(c->newuser_c, p + 5319, 51);

    c->wwiv_reg_number = rd_u32(p + 5370);

    rd_str(c->dial_prefix, p + 5374, 21);

    c->post_call_ratio = rd_float(p + 5395);

    rd_str(c->upload_c, p + 5399, 51);
    rd_str(c->dszbatchdl, p + 5450, 81);
    rd_str(c->modem_type, p + 5531, 9);
    rd_str(c->batchdir, p + 5540, 81);

    c->sysstatusoffset = (int)(short)rd_u16(p + 5621);
    c->network_type    = p[5623];
    rd_str(c->res, p + 5624, 36);
}

/* ================================================================
 * Convert niftyrec: DOS(450 bytes) -> macOS
 * ================================================================ */

static void conv_niftyrec(niftyrec *n, const unsigned char *p)
{
    int i;
    memset(n, 0, sizeof(niftyrec));

    n->chatcolor   = p[0];
    n->echochar    = p[1];
    rd_str(n->firstmenu, p + 2, 15);
    n->matrixtype  = p[17];
    n->fcom        = p[18];
    n->res         = p[19];
    rd_str(n->newusermenu, p + 20, 15);
    n->systemtype  = p[35];
    rd_str(n->menudir, p + 36, 30);
    n->fptsratio   = p[66];

    n->lockoutrate = rd_u32(p + 67);
    n->nifstatus   = rd_u32(p + 71);

    rd_str(n->rotate, p + 75, 5);

    /* exarc[4] — all char, identical */
    for (i = 0; i < 4; i++)
        memcpy(&n->arc[i], p + 80 + i * DOS_EXARC_SIZE, DOS_EXARC_SIZE);

    rd_str(n->matrix, p + 336, 31);
    rd_str(n->lockoutpw, p + 367, 31);

    n->nuvbad      = p[398];
    n->nuvyes      = p[399];
    n->nuvlevel    = p[400];
    rd_str(n->nuvsl, p + 401, 10);
    rd_str(n->nuvinf, p + 411, 8);
    rd_str(n->nuinf, p + 419, 8);
    n->nulevel     = p[427];
    rd_str(n->defaultcol, p + 428, 20);
    n->nuvaction   = p[448];
    n->nuvbadlevel = p[449];
}

/* ================================================================
 * Convert statusrec: DOS(182 bytes) -> macOS
 * ================================================================ */

static void conv_statusrec(statusrec *s, const unsigned char *p)
{
    memset(s, 0, sizeof(statusrec));

    rd_str(s->date1, p + 0, 9);
    rd_str(s->date2, p + 9, 9);
    rd_str(s->date3, p + 18, 9);
    rd_str(s->log1, p + 27, 13);
    rd_str(s->log2, p + 40, 13);
    s->dltoday      = p[53];
    rd_str(s->log5, p + 54, 13);
    rd_str(s->resx, p + 67, 4);

    s->users        = rd_u16(p + 71);
    s->callernum    = rd_u16(p + 73);
    s->callstoday   = rd_u16(p + 75);
    s->msgposttoday = rd_u16(p + 77);
    s->emailtoday   = rd_u16(p + 79);
    s->fbacktoday   = rd_u16(p + 81);
    s->uptoday      = rd_u16(p + 83);
    s->activetoday  = rd_u16(p + 85);

    s->qscanptr     = rd_u32(p + 87);
    s->amsganon     = p[91];
    s->amsguser     = rd_u16(p + 92);
    s->callernum1   = rd_u32(p + 94);

    s->net_edit_stuff = rd_u16(p + 98);
    s->wwiv_version   = rd_u16(p + 100);
    s->net_version    = rd_u16(p + 102);

    s->net_bias     = rd_float(p + 104);
    s->last_connect = (long)(int)rd_u32(p + 108);
    s->last_bbslist = (long)(int)rd_u32(p + 112);
    s->net_req_free = rd_float(p + 116);

    rd_str(s->log3, p + 120, 18);
    rd_str(s->log4, p + 138, 13);
    rd_str(s->lastuser, p + 151, 31);
}

/* ================================================================
 * Convert subboardrec: DOS(165 bytes) -> macOS
 * ================================================================ */

static void conv_subboardrec(subboardrec *s, const unsigned char *p)
{
    memset(s, 0, sizeof(subboardrec));

    rd_str(s->name, p + 0, 41);
    rd_str(s->filename, p + 41, 9);
    rd_str(s->nmpath, p + 50, 51);
    s->conf = p[101];
    memcpy(s->readacs, p + 102, 21);
    memcpy(s->postacs, p + 123, 21);
    s->anony = p[144];
    s->age   = p[145];
    s->attr  = rd_u32(p + 146);
    s->maxmsgs      = rd_u16(p + 150);
    s->ar           = rd_u16(p + 152);
    s->storage_type = rd_u16(p + 154);
    /* addressrec: 4 ints, each 2 bytes on DOS */
    s->add.zone  = (int)(short)rd_u16(p + 156);
    s->add.net   = (int)(short)rd_u16(p + 158);
    s->add.node  = (int)(short)rd_u16(p + 160);
    s->add.point = (int)(short)rd_u16(p + 162);
    s->origin    = p[164];
}

/* ================================================================
 * Convert directoryrec: DOS(243 bytes) -> macOS
 * ================================================================ */

static void conv_directoryrec(directoryrec *d, const unsigned char *p)
{
    memset(d, 0, sizeof(directoryrec));

    rd_str(d->name, p + 0, 41);
    rd_str(d->filename, p + 41, 9);
    rd_str(d->dpath, p + 50, 81);
    rd_str(d->upath, p + 131, 12);
    rd_str(d->vacs, p + 143, 21);
    rd_str(d->res, p + 164, 48);
    rd_str(d->acs, p + 212, 21);
    d->dar      = rd_u16(p + 233);
    d->maxfiles = rd_u16(p + 235);
    d->mask     = rd_u16(p + 237);
    d->type     = rd_u16(p + 239);
    d->confnum  = rd_u16(p + 241);
}

/* ================================================================
 * Convert protocolrec: DOS(416 bytes) -> macOS
 * ================================================================ */

static void conv_protocolrec(protocolrec *pr, const unsigned char *p)
{
    memset(pr, 0, sizeof(protocolrec));

    rd_str(pr->description, p + 0, 81);
    rd_str(pr->receivefn, p + 81, 81);
    rd_str(pr->sendfn, p + 162, 81);
    rd_str(pr->sendbatch, p + 243, 81);
    rd_str(pr->receivebatch, p + 324, 81);
    pr->singleok = (int)(short)rd_u16(p + 405);
    pr->ok1  = rd_u16(p + 407);
    pr->nok1 = rd_u16(p + 409);
    pr->ok2  = rd_u16(p + 411);
    pr->nok2 = rd_u16(p + 413);
    pr->key  = p[415];
}

/* ================================================================
 * Convert xarcrec: DOS(176 bytes) -> macOS
 * ================================================================ */

static void conv_xarcrec(xarcrec *x, const unsigned char *p)
{
    memset(x, 0, sizeof(xarcrec));

    rd_str(x->extension, p + 0, 4);
    rd_str(x->arct, p + 4, 32);
    rd_str(x->arcc, p + 36, 32);
    rd_str(x->arca, p + 68, 32);
    rd_str(x->arce, p + 100, 32);
    rd_str(x->arcl, p + 132, 32);
    x->attr = (long)(int)rd_u32(p + 164);
    x->ok1  = (int)(short)rd_u16(p + 168);
    x->ok2  = (int)(short)rd_u16(p + 170);
    x->nk1  = (int)(short)rd_u16(p + 172);
    x->nk2  = (int)(short)rd_u16(p + 174);
}

/* ================================================================
 * Convert userrec - need to figure out DOS layout too
 * ================================================================ */

/* DOS userrec size: let's compute from the struct definition.
 * All char/unsigned char fields are the same. The differences are:
 *   unsigned short fields: same (2 bytes)
 *   int fpts: 2 on DOS, 4 on macOS
 *   unsigned long fields: 4 on DOS, 8 on macOS
 *   float fields: 4 on both
 *   long resl[29]: 4*29=116 on DOS, 8*29=232 on macOS
 *   int resi[29]: 2*29=58 on DOS, 4*29=116 on macOS
 *   float resf[29]: same (4*29=116)
 *   long qscn[200]: 4*200=800 on DOS, 8*200=1600 on macOS
 *   long nscn[200]: 4*200=800 on DOS, 8*200=1600 on macOS
 */

/* DOS userrec field offsets (computed manually, no padding): */
/* char block: name[31]+realname[21]+callsign[7]+phone[21]+dphone[21]+
   pw[21]+laston[9]+firston[9]+note[41]+comment[41]+street[41]+city[41]+
   macros[4][MAX_PATH_LEN]+sex = 31+21+7+21+21+21+9+9+41+41+41+41+324+1 = 629 */
/* unsigned char block: age+inact+comp_type+defprot+defed+flisttype+
   mlisttype+helplevel+lastsub+lastdir+lastconf+screenchars+screenlines+
   sl+dsl+exempt+colors[20]+votes[20]+illegal+waiting+subop+ontoday = 34 */
/* -> offset 663 */
/* unsigned short: forwardusr+msgpost+emailsent+feedbacksent+posttoday+
   etoday+ar+dar+restrict+month+day+year = 12*2 = 24 -> offset 687 */
/* int fpts: 2 -> offset 689 */
/* unsigned short: uploaded+downloaded+logons+fsenttoday1+emailnet+postnet = 6*2=12 -> 701 */
/* unsigned long: msgread+uk+dk+daten+sysstatus+lastrate+nuv+timebank = 8*4=32 -> 733 */
/* float: timeontoday+extratime+timeon+pcr+ratio+pos_account+neg_account = 7*4=28 -> 761 */
/* char res[29] = 29 -> 790 */
/* long resl[29] = 29*4=116 -> 906 */
/* int resi[29] = 29*2=58 -> 964 */
/* float resf[29] = 29*4=116 -> 1080 */
/* long qscn[200] = 200*4=800 -> 1880 */
/* long nscn[200] = 200*4=800 -> 2680 */

#define DOS_USERREC_SIZE 2680

static void conv_userrec(userrec *u, const unsigned char *p)
{
    int i, off;
    memset(u, 0, sizeof(userrec));

    /* char block: 629 bytes */
    rd_str(u->name, p + 0, 31);
    rd_str(u->realname, p + 31, 21);
    rd_str(u->callsign, p + 52, 7);
    rd_str(u->phone, p + 59, 21);
    rd_str(u->dphone, p + 80, 21);
    rd_str(u->pw, p + 101, 21);
    rd_str(u->laston, p + 122, 9);
    rd_str(u->firston, p + 131, 9);
    rd_str(u->note, p + 140, 41);
    rd_str(u->comment, p + 181, 41);
    rd_str(u->street, p + 222, 41);
    rd_str(u->city, p + 263, 41);
    for (i = 0; i < 4; i++)
        rd_str(u->macros[i], p + 304 + i*81, 81);
    u->sex = p[628];

    /* unsigned char block at 629 */
    off = 629;
    u->age          = p[off++];
    u->inact        = p[off++];
    u->comp_type    = p[off++];
    u->defprot      = p[off++];
    u->defed        = p[off++];
    u->flisttype    = p[off++];
    u->mlisttype    = p[off++];
    u->helplevel    = p[off++];
    u->lastsub      = p[off++];
    u->lastdir      = p[off++];
    u->lastconf     = p[off++];
    u->screenchars  = p[off++];
    u->screenlines  = p[off++];
    u->sl           = p[off++];
    u->dsl          = p[off++];
    u->exempt       = p[off++];
    memcpy(u->colors, p + off, 20); off += 20;
    memcpy(u->votes, p + off, 20); off += 20;
    u->illegal      = p[off++];
    u->waiting      = p[off++];
    u->subop        = p[off++];
    u->ontoday      = p[off++];
    /* off should be 663 */

    /* unsigned short block */
    u->forwardusr    = rd_u16(p + off); off += 2;
    u->msgpost       = rd_u16(p + off); off += 2;
    u->emailsent     = rd_u16(p + off); off += 2;
    u->feedbacksent  = rd_u16(p + off); off += 2;
    u->posttoday     = rd_u16(p + off); off += 2;
    u->etoday        = rd_u16(p + off); off += 2;
    u->ar            = rd_u16(p + off); off += 2;
    u->dar           = rd_u16(p + off); off += 2;
    /* restrict field — use memcpy to avoid keyword issue */
    {
        unsigned short tmp = rd_u16(p + off); off += 2;
        memcpy((char *)u + ((char *)&u->ar - (char *)u) + 4, &tmp, 2);
    }
    /* month, day, year — these follow restrict in the struct */
    {
        unsigned short mo = rd_u16(p + off); off += 2;
        unsigned short dy = rd_u16(p + off); off += 2;
        unsigned short yr = rd_u16(p + off); off += 2;
        /* Copy by offset: restrict is at ar+2, month at ar+4, day at ar+6, year at ar+8 */
        memcpy((char *)&u->ar + 6, &mo, 2);
        memcpy((char *)&u->ar + 8, &dy, 2);
        memcpy((char *)&u->ar + 10, &yr, 2);
    }
    /* off should be 687 */

    /* int fpts (DOS: 2 bytes) */
    u->fpts = (int)(short)rd_u16(p + off); off += 2;
    /* off = 689 */

    /* unsigned short block */
    u->uploaded     = rd_u16(p + off); off += 2;
    u->downloaded   = rd_u16(p + off); off += 2;
    u->logons       = rd_u16(p + off); off += 2;
    u->fsenttoday1  = rd_u16(p + off); off += 2;
    u->emailnet     = rd_u16(p + off); off += 2;
    u->postnet      = rd_u16(p + off); off += 2;
    /* off = 701 */

    /* unsigned long block (DOS: 4 bytes each) */
    u->msgread   = rd_u32(p + off); off += 4;
    u->uk        = rd_u32(p + off); off += 4;
    u->dk        = rd_u32(p + off); off += 4;
    u->daten     = rd_u32(p + off); off += 4;
    u->sysstatus = rd_u32(p + off); off += 4;
    u->lastrate  = rd_u32(p + off); off += 4;
    u->nuv       = rd_u32(p + off); off += 4;
    u->timebank  = rd_u32(p + off); off += 4;
    /* off = 733 */

    /* float block */
    u->timeontoday = rd_float(p + off); off += 4;
    u->extratime   = rd_float(p + off); off += 4;
    u->timeon      = rd_float(p + off); off += 4;
    u->pcr         = rd_float(p + off); off += 4;
    u->ratio       = rd_float(p + off); off += 4;
    u->pos_account = rd_float(p + off); off += 4;
    u->neg_account = rd_float(p + off); off += 4;
    /* off = 761 */

    /* reserved blocks */
    rd_str(u->res, p + off, 29); off += 29;
    /* long resl[29] — DOS: 4 bytes each */
    for (i = 0; i < 29; i++) {
        u->resl[i] = (long)(int)rd_u32(p + off); off += 4;
    }
    /* int resi[29] — DOS: 2 bytes each */
    for (i = 0; i < 29; i++) {
        u->resi[i] = (int)(short)rd_u16(p + off); off += 2;
    }
    /* float resf[29] */
    for (i = 0; i < 29; i++) {
        u->resf[i] = rd_float(p + off); off += 4;
    }
    /* long qscn[200] — DOS: 4 bytes each */
    for (i = 0; i < 200; i++) {
        u->qscn[i] = (long)(int)rd_u32(p + off); off += 4;
    }
    /* long nscn[200] — DOS: 4 bytes each */
    for (i = 0; i < 200; i++) {
        u->nscn[i] = (long)(int)rd_u32(p + off); off += 4;
    }
    /* off should be 2680 = DOS_USERREC_SIZE */
}

/* DOS smalrec: name[31] + unsigned short number = 33 bytes */
static void conv_smalrec(smalrec *s, const unsigned char *p)
{
    memset(s, 0, sizeof(smalrec));
    rd_str(s->name, p + 0, 31);
    s->number = rd_u16(p + 31);
}

/* ================================================================
 * Path fixup: convert DOS backslashes to forward slashes,
 * and optionally rebase paths
 * ================================================================ */

static void fixup_path(char *path, const char *newbase)
{
    int i;
    char *bs;

    /* Convert backslashes to forward slashes */
    for (i = 0; path[i]; i++)
        if (path[i] == '\\') path[i] = '/';

    /* If path starts with a drive letter (C:), strip it and rebase */
    if (path[1] == ':') {
        char tmp[256];
        /* Skip drive letter and colon */
        if (newbase) {
            snprintf(tmp, sizeof(tmp), "%s%s", newbase, path + 2);
        } else {
            snprintf(tmp, sizeof(tmp), ".%s", path + 2);
        }
        strcpy(path, tmp);
    }
}

/* ================================================================
 * MAIN
 * ================================================================ */

int main(int argc, char **argv)
{
    char dos_dir[512], mac_dir[512];
    char src[512], dst[512];
    unsigned char *buf;
    long len;
    FILE *f;
    int i, n;

    if (argc < 3) {
        printf("Usage: %s <dos_dir> <macos_dir>\n", argv[0]);
        printf("  Converts DOS Dominion BBS data files to macOS format.\n");
        return 1;
    }

    snprintf(dos_dir, sizeof(dos_dir), "%s", argv[1]);
    snprintf(mac_dir, sizeof(mac_dir), "%s", argv[2]);

    /* Ensure trailing slash */
    if (dos_dir[strlen(dos_dir)-1] != '/') strcat(dos_dir, "/");
    if (mac_dir[strlen(mac_dir)-1] != '/') strcat(mac_dir, "/");

    /* Create output directories */
    mkd(mac_dir);
    snprintf(dst, sizeof(dst), "%sdata", mac_dir); mkd(dst);
    snprintf(dst, sizeof(dst), "%sdata/dir", mac_dir); mkd(dst);
    snprintf(dst, sizeof(dst), "%safiles", mac_dir); mkd(dst);
    snprintf(dst, sizeof(dst), "%stemp", mac_dir); mkd(dst);
    snprintf(dst, sizeof(dst), "%smsgs", mac_dir); mkd(dst);
    snprintf(dst, sizeof(dst), "%sbatch", mac_dir); mkd(dst);
    snprintf(dst, sizeof(dst), "%smenus", mac_dir); mkd(dst);
    snprintf(dst, sizeof(dst), "%sdls", mac_dir); mkd(dst);
    snprintf(dst, sizeof(dst), "%sscripts", mac_dir); mkd(dst);

    /* ============================================================
     * 1. CONFIG.DAT (configrec + niftyrec)
     * ============================================================ */
    printf("Converting CONFIG.DAT...\n");
    snprintf(src, sizeof(src), "%sCONFIG.DAT", dos_dir);
    buf = read_file(src, &len);
    if (!buf) {
        printf("  ERROR: cannot open %s\n", src);
        return 1;
    }
    printf("  DOS file size: %ld bytes (expected ~%d)\n", len,
           DOS_CONFIGREC_SIZE + DOS_NIFTYREC_SIZE);
    {
        configrec syscfg;
        niftyrec nifty;
        int cfg_size, nif_off;

        /* Handle the 2-byte discrepancy: try actual file size */
        cfg_size = len - DOS_NIFTYREC_SIZE;
        if (cfg_size < DOS_CONFIGREC_SIZE - 4 || cfg_size > DOS_CONFIGREC_SIZE + 4) {
            printf("  WARNING: unexpected configrec size %d (expected %d)\n",
                   cfg_size, DOS_CONFIGREC_SIZE);
            printf("  Trying with calculated DOS_CONFIGREC_SIZE=%d\n", DOS_CONFIGREC_SIZE);
            cfg_size = DOS_CONFIGREC_SIZE;
        }
        nif_off = len - DOS_NIFTYREC_SIZE;

        conv_configrec(&syscfg, buf);

        /* Fix paths to be relative to mac_dir */
        snprintf(syscfg.datadir, sizeof(syscfg.datadir), "%sdata/", mac_dir);
        snprintf(syscfg.gfilesdir, sizeof(syscfg.gfilesdir), "%safiles/", mac_dir);
        snprintf(syscfg.tempdir, sizeof(syscfg.tempdir), "%stemp/", mac_dir);
        snprintf(syscfg.msgsdir, sizeof(syscfg.msgsdir), "%smsgs/", mac_dir);
        snprintf(syscfg.batchdir, sizeof(syscfg.batchdir), "%sbatch/", mac_dir);
        snprintf(syscfg.menudir, sizeof(syscfg.menudir), "%smenus/", mac_dir);
        snprintf(syscfg.dloadsdir, sizeof(syscfg.dloadsdir), "%sdls/", mac_dir);

        /* Set primaryport to 0 — no modem on macOS */
        syscfg.primaryport = 0;

        /* Fix userreclen to macOS size */
        syscfg.userreclen = sizeof(userrec);

        conv_niftyrec(&nifty, buf + nif_off);

        snprintf(dst, sizeof(dst), "%sConfig.dat", mac_dir);
        f = fopen(dst, "wb");
        fwrite(&syscfg, sizeof(syscfg), 1, f);
        fwrite(&nifty, sizeof(nifty), 1, f);
        fclose(f);
        printf("  Wrote %s (%zu bytes)\n", dst, sizeof(syscfg) + sizeof(nifty));
        printf("  System: %s\n", syscfg.systemname);
        printf("  SysOp:  %s\n", syscfg.sysopname);
    }
    free(buf);

    /* ============================================================
     * 2. STATUS.DAT
     * ============================================================ */
    printf("Converting STATUS.DAT...\n");
    snprintf(src, sizeof(src), "%sDATA/STATUS.DAT", dos_dir);
    buf = read_file(src, &len);
    if (buf) {
        statusrec status;
        conv_statusrec(&status, buf);
        /* Fix callernum for macOS */
        if (status.callernum != 65535) {
            status.callernum1 = (long)status.callernum;
            status.callernum = 65535;
        }
        snprintf(dst, sizeof(dst), "%sdata/Status.dat", mac_dir);
        f = fopen(dst, "wb");
        fwrite(&status, sizeof(status), 1, f);
        fclose(f);
        printf("  Wrote %s (%zu bytes), %d users, last: %s\n",
               dst, sizeof(status), status.users, status.lastuser);
        free(buf);
    } else {
        printf("  WARNING: STATUS.DAT not found\n");
    }

    /* ============================================================
     * 3. SUBS.DAT
     * ============================================================ */
    printf("Converting SUBS.DAT...\n");
    snprintf(src, sizeof(src), "%sDATA/SUBS.DAT", dos_dir);
    buf = read_file(src, &len);
    if (buf) {
        n = len / DOS_SUBBOARDREC_SIZE;
        snprintf(dst, sizeof(dst), "%sdata/SUBS.DAT", mac_dir);
        f = fopen(dst, "wb");
        for (i = 0; i < n; i++) {
            subboardrec sub;
            conv_subboardrec(&sub, buf + i * DOS_SUBBOARDREC_SIZE);
            fwrite(&sub, sizeof(sub), 1, f);
            if (i < 5) printf("  Sub %d: %s (%s)\n", i, sub.name, sub.filename);
        }
        fclose(f);
        printf("  Converted %d sub-boards\n", n);
        free(buf);
    }

    /* ============================================================
     * 4. DIRS.DAT
     * ============================================================ */
    printf("Converting DIRS.DAT...\n");
    snprintf(src, sizeof(src), "%sDATA/DIRS.DAT", dos_dir);
    buf = read_file(src, &len);
    if (buf) {
        n = len / DOS_DIRECTORYREC_SIZE;
        snprintf(dst, sizeof(dst), "%sdata/DIRS.DAT", mac_dir);
        f = fopen(dst, "wb");
        for (i = 0; i < n; i++) {
            directoryrec dir;
            conv_directoryrec(&dir, buf + i * DOS_DIRECTORYREC_SIZE);
            fixup_path(dir.dpath, mac_dir);
            fwrite(&dir, sizeof(dir), 1, f);
            if (i < 5) printf("  Dir %d: %s (%s)\n", i, dir.name, dir.filename);
        }
        fclose(f);
        printf("  Converted %d directories\n", n);
        free(buf);
    }

    /* ============================================================
     * 5. PROTOCOL.DAT
     * ============================================================ */
    printf("Converting PROTOCOL.DAT...\n");
    snprintf(src, sizeof(src), "%sDATA/PROTOCOL.DAT", dos_dir);
    buf = read_file(src, &len);
    if (buf) {
        n = len / DOS_PROTOCOLREC_SIZE;
        snprintf(dst, sizeof(dst), "%sdata/protocol.dat", mac_dir);
        f = fopen(dst, "wb");
        for (i = 0; i < n; i++) {
            protocolrec proto;
            conv_protocolrec(&proto, buf + i * DOS_PROTOCOLREC_SIZE);
            fwrite(&proto, sizeof(proto), 1, f);
            if (i < 5) printf("  Proto %d: %s (key=%c)\n", i, proto.description, proto.key);
        }
        fclose(f);
        printf("  Converted %d protocols\n", n);
        free(buf);
    }

    /* ============================================================
     * 6. RESULTS.DAT (resultrec — same size on both platforms!)
     * ============================================================ */
    printf("Copying RESULTS.DAT (no conversion needed)...\n");
    snprintf(src, sizeof(src), "%sDATA/RESULTS.DAT", dos_dir);
    snprintf(dst, sizeof(dst), "%sdata/RESULTS.DAT", mac_dir);
    copy_file(src, dst);

    /* ============================================================
     * 7. CONF.DAT (confrec — same size on both platforms!)
     * ============================================================ */
    printf("Copying CONF.DAT (no conversion needed)...\n");
    snprintf(src, sizeof(src), "%sDATA/CONF.DAT", dos_dir);
    snprintf(dst, sizeof(dst), "%sdata/conf.dat", mac_dir);
    copy_file(src, dst);

    /* ============================================================
     * 8. ARCHIVE.DAT (xarcrec — needs conversion)
     * Note: BBS reads 8 xarcrecs but file may have fewer
     * ============================================================ */
    printf("Converting archive.dat...\n");
    snprintf(src, sizeof(src), "%sDATA/ARCHIVE.DAT", dos_dir);
    if (!access(src, R_OK)) {
        /* Try just copying — if it doesn't exist, not critical */
        buf = read_file(src, &len);
        if (buf) {
            n = len / DOS_XARCREC_SIZE;
            if (n > 8) n = 8;
            snprintf(dst, sizeof(dst), "%sdata/archive.dat", mac_dir);
            f = fopen(dst, "wb");
            for (i = 0; i < 8; i++) {
                xarcrec xarc;
                if (i < n) {
                    conv_xarcrec(&xarc, buf + i * DOS_XARCREC_SIZE);
                } else {
                    memset(&xarc, 0, sizeof(xarc));
                }
                fwrite(&xarc, sizeof(xarc), 1, f);
            }
            fclose(f);
            printf("  Converted %d archive defs\n", n);
            free(buf);
        }
    } else {
        /* Create empty */
        snprintf(dst, sizeof(dst), "%sdata/archive.dat", mac_dir);
        f = fopen(dst, "wb");
        for (i = 0; i < 8; i++) {
            xarcrec xarc;
            memset(&xarc, 0, sizeof(xarc));
            fwrite(&xarc, sizeof(xarc), 1, f);
        }
        fclose(f);
    }

    /* ============================================================
     * 9. USER.LST + USER.IDX (user records)
     * ============================================================ */
    printf("Converting USER.LST...\n");
    snprintf(src, sizeof(src), "%sDATA/USER.LST", dos_dir);
    buf = read_file(src, &len);
    if (buf) {
        n = len / DOS_USERREC_SIZE;
        snprintf(dst, sizeof(dst), "%sdata/user.lst", mac_dir);
        f = fopen(dst, "wb");
        for (i = 0; i < n; i++) {
            userrec user;
            conv_userrec(&user, buf + i * DOS_USERREC_SIZE);
            fwrite(&user, sizeof(user), 1, f);
            printf("  User %d: %-31s SL=%3d DSL=%3d\n",
                   i+1, user.name, user.sl, user.dsl);
        }
        fclose(f);
        printf("  Converted %d users\n", n);
        free(buf);
    }

    printf("Converting USER.IDX...\n");
    snprintf(src, sizeof(src), "%sDATA/USER.IDX", dos_dir);
    buf = read_file(src, &len);
    if (buf) {
        n = len / DOS_SMALREC_SIZE;
        snprintf(dst, sizeof(dst), "%sdata/user.idx", mac_dir);
        f = fopen(dst, "wb");
        for (i = 0; i < n; i++) {
            smalrec idx;
            conv_smalrec(&idx, buf + i * DOS_SMALREC_SIZE);
            fwrite(&idx, sizeof(idx), 1, f);
        }
        fclose(f);
        printf("  Converted %d index entries\n", n);
        free(buf);
    }

    /* ============================================================
     * 10. Copy all text/binary files from AFILES as-is
     * ============================================================ */
    printf("Copying AFILES...\n");
    {
        DIR *d;
        struct dirent *de;
        snprintf(src, sizeof(src), "%sAFILES", dos_dir);
        d = opendir(src);
        if (d) {
            while ((de = readdir(d)) != NULL) {
                if (de->d_name[0] == '.') continue;
                snprintf(src, sizeof(src), "%sAFILES/%s", dos_dir, de->d_name);
                snprintf(dst, sizeof(dst), "%safiles/%s", mac_dir, de->d_name);
                copy_file(src, dst);
                printf("  %s\n", de->d_name);
            }
            closedir(d);
        }
    }

    /* ============================================================
     * 11. Copy MENUS as-is (text files)
     * ============================================================ */
    printf("Copying MENUS...\n");
    {
        DIR *d;
        struct dirent *de;
        snprintf(src, sizeof(src), "%sMENUS", dos_dir);
        d = opendir(src);
        if (d) {
            while ((de = readdir(d)) != NULL) {
                if (de->d_name[0] == '.') continue;
                snprintf(src, sizeof(src), "%sMENUS/%s", dos_dir, de->d_name);
                snprintf(dst, sizeof(dst), "%smenus/%s", mac_dir, de->d_name);
                copy_file(src, dst);
                printf("  %s\n", de->d_name);
            }
            closedir(d);
        }
    }

    /* ============================================================
     * 12. Copy remaining DATA files that don't need conversion
     * ============================================================ */
    printf("Copying other DATA files...\n");
    {
        char *copy_files[] = {
            "MODEM.DAT", "STRINGS.DAT", "SYSSTR.DAT", "HISTORY.DAT",
            "SMW.DAT", "VOTING.DAT", "SDESC.DAT", "RUMOURS.DAT",
            "EMAIL.DAT", "GENERAL.SUB", "NEWS.SUB", NULL
        };
        for (i = 0; copy_files[i]; i++) {
            snprintf(src, sizeof(src), "%sDATA/%s", dos_dir, copy_files[i]);
            snprintf(dst, sizeof(dst), "%sdata/%s", mac_dir, copy_files[i]);
            if (!access(src, R_OK)) {
                copy_file(src, dst);
                printf("  %s\n", copy_files[i]);
            }
        }
    }

    /* ============================================================
     * 13. Copy MSGS directory
     * ============================================================ */
    printf("Copying MSGS...\n");
    {
        DIR *d;
        struct dirent *de;
        snprintf(src, sizeof(src), "%sMSGS", dos_dir);
        d = opendir(src);
        if (d) {
            while ((de = readdir(d)) != NULL) {
                if (de->d_name[0] == '.') continue;
                snprintf(src, sizeof(src), "%sMSGS/%s", dos_dir, de->d_name);
                snprintf(dst, sizeof(dst), "%smsgs/%s", mac_dir, de->d_name);
                copy_file(src, dst);
                printf("  %s\n", de->d_name);
            }
            closedir(d);
        }
    }

    /* ============================================================
     * 14. Copy DOM.KEY if present
     * ============================================================ */
    snprintf(src, sizeof(src), "%sDOM.KEY", dos_dir);
    if (!access(src, R_OK)) {
        snprintf(dst, sizeof(dst), "%sDOM.KEY", mac_dir);
        copy_file(src, dst);
        printf("Copied DOM.KEY\n");
    }

    printf("\nConversion complete! Data written to %s\n", mac_dir);
    printf("Note: DOM.KEY may need regeneration for macOS bp() stub.\n");
    return 0;
}
