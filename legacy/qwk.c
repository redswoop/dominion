#ifdef QWK
/*
 *
 * Basic QWK functions in C
 * note these routines make some QWK<->Fidonet alterations
 * so the two will cooperate better (snort -- yeah, right)...
 * if you have to fool with the index file, #define USEFLOATS
 *
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "qwk.h"

#include "strips.c"

extern char * rstrip  (char *);
extern char * lstrip  (char *);
extern char * stripcr (char *);


size_t qwkreadhdr (FILE *fp,QWKHDR *hdr,int rep) {

    /* read and decode awful QWK message header
     * if rep != 0 a reply packet is being read
     */

    QWKHDRREADER h;
    struct tm    tm;
    char         date[14],p;


    if(fread(&h,1,128,fp) != 128) {
       printf("header truncated\n");
       return 0;
    }
    hdr->status  = h.status;
    hdr->confnum = h.confnum + 1;           /* no longer zero-based */
    hdr->live = (char)QWK_ISALIVE(h.live);
    h.live = 0;
    hdr->numchunks = atol(h.numchunks) - 1L; /* exclude header */
    *h.numchunks = 0;
    hdr->repnum = atol(h.repnum);
    *h.repnum = 0;
    strcpy(hdr->pword,h.pword);
    rstrip(hdr->pword);
    *h.pword = 0;
    strcpy(hdr->subj,h.subj);
    rstrip(hdr->subj);
    *h.subj = 0;
    strcpy(hdr->from,h.from);
    rstrip(hdr->from);
    *h.from = 0;
    strcpy(hdr->to,h.to);
    rstrip(hdr->to);
    *h.to = 0;
    strcpy(date,h.date);
    *h.date = 0;
    if(!rep) hdr->msgnum = atol(h.msgnum);
    else {
        hdr->msgnum = 0;
        hdr->confnum = atoi(h.msgnum) + 1;
    }
    date[2] = 0;
    tm.tm_mon = atoi(date);
    date[5] = 0;
    tm.tm_mday = atoi(&date[3]);
    p = date[8];
    date[8] = 0;
    tm.tm_year = atoi(&date[6]);
    if(tm.tm_year < 89) tm.tm_year += 100;
    date[8] = p;
    date[10] = 0;
    tm.tm_hour = atoi(&date[8]);
    tm.tm_min = atoi(&date[11]);
    tm.tm_sec = 0;
    hdr->date = mktime(&tm);
    return 1;
}



size_t qwkreadblk (FILE *fp,char *blk) {

    /* read text block and convert 0xe3's to cr's
     * note that blk must be at least QWKBLKSIZE + 1 in
     * length to allow for NULL terminator.
     */

    char *p = blk;


    if(fread(blk,1,QWKBLKSIZE,fp) != QWKBLKSIZE) {
       printf("Block Truncated");
       return 0;
    }
    blk[QWKBLKSIZE] = 0;
    while((p = strchr(p,'\xe3')) != NULL) *p = '\r';
    return 1;
}



size_t qwkreadblks (FILE *fp,char *blk,size_t c) {

    /* read several text blocks and convert 0xe3's to cr's
     * note that blk must be at least (QWKBLKSIZE * c) + 1
     * in length to allow for NULL terminator
     */

    char   *p = blk;


    c = fread(blk,QWKBLKSIZE,c,fp);
    if(!c)
       return 0;
    blk[QWKBLKSIZE * c] = 0;
    while((p = strchr(p,'\xe3')) != NULL) *p = '\r';
    return c;
}





#ifdef QWKEXPORT

size_t qwkwritehdr (FILE *fp,QWKHDR *hdr,int rep) {

    /* encode and write awful QWK message header
     * if rep != 0 then header is for reply packet
     */

    QWKHDRREADER h;

    bug("Top of header write");

    h.status  = hdr->status;
    if(!rep) sprintf(h.msgnum,"%-7ld",hdr->msgnum);
    else sprintf(h.msgnum,"%-7d",hdr->confnum - 1); /* zero based again */
    strftime(h.date,13,"%m/%d/%y%h:%m",localtime(&hdr->date));
    sprintf(h.to,"%-25.25s",hdr->to);
    sprintf(h.from,"%-25.25s",hdr->from);
    sprintf(h.subj,"%-25.25s",hdr->subj);
    sprintf(h.pword,"%-12.12s",hdr->pword);
    sprintf(h.repnum,"%-8ld",hdr->repnum);
    sprintf(h.numchunks,"%-6ld",hdr->numchunks + 1); /* add one for header */
    if(hdr->live) h.live = 0xe1;
    else h.live = 0xe2;
    h.confnum = hdr->confnum - 1;   /* zero based again */
    if(fwrite(&h,1,QWKBLKSIZE,fp) != QWKBLKSIZE) return 0;
    bug("Returning from Headerwrite");
    return 1;
}



size_t qwkwriteblk (FILE *fp,char *blk) {

    /* write text block after converting cr's to 0xe3's
     * well, what it actually does is write a C string
     * then pad with spaces if required...
     * returns number of blocks written
     */

    char   *p = blk;
    size_t w,len;


    while((p = strchr(p,'\r')) != NULL) *p = '\xe3';
    len = strlen(blk);
    len = min(len,QWKBLKSIZE);
    w = fwrite(blk,1,len,fp);
    if(!w || w == 65535U) return 0;   /* write error */
    while(w++ < QWKBLKSIZE) {
        fputc(' ',fp);
    }
    return 1;
}



size_t qwkwriteblks (FILE *fp,char *blk) {

    /* write several text blocks after converting cr's to 0xe3's
     * well, what it actually does is write a C string then
     * pad to the nearest block with spaces if required...
     * returns number of blocks written
     */

    char   *p = blk;
    size_t w,len;


    while((p = strchr(p,'\r')) != NULL) *p = '\xe3';
    len = strlen(blk);
    w = fwrite(blk,1,len,fp);
    if(!w || w == 65535U) return 0;  /* write error */
    len = (w / QWKBLKSIZE) + ((w % QWKBLKSIZE) != 0);
    w = (len * QWKBLKSIZE) - w;
    while(w--) {
        fputc(' ',fp);
    }
    return len;
}



#ifdef USEFLOATS

size_t qwkwriteidx (FILE *fp,QWKIDX *i) {

    /* write QWK index record, convert IEEE to nasty MSBIN */

    i->recnum = IEEEToMSBIN(i->recnum);
    if(fwrite(i,1,sizeof(QWKIDX),fp) != sizeof(QWKIDX)) return 0;
    return 1;
}




size_t qwkwriteidxs (FILE *fp,QWKIDX **i,size_t c) {

    /* write several QWK index records, convert IEEE to MSBIN */

    size_t oc;


    oc = c;
    while(c) {
        i[c - 1]->recnum = IEEEToMSBIN(i[c - 1]->recnum);
        c--;
    }
    return fwrite(i[0],sizeof(QWKIDX),oc,fp);
}

#endif

#endif



#ifdef USEFLOATS

 /***  MSBIN conversion routines from QWK.DOC by Jeffrey Foy ***/

union Converter {
    unsigned char uc[10];
    unsigned int  ui[5];
    unsigned long ul[2];
    float          f[2];
    double         d[1];
};

 /* MSBINToIEEE - Converts an MSBIN floating point number */
 /*               to IEEE floating point format           */
 /*                                                       */
 /*  Input: f - floating point number in MSBIN format     */
 /* Output: Same number in IEEE format                    */

float MSBINToIEEE (float f) {

    union Converter t;
    int             sign, exp;       /* sign and exponent */


    t.f[0] = f;

 /* extract the sign & move exponent bias from 0x81 to 0x7f */

    sign = t.uc[2] / 0x80;
    exp  = (t.uc[3] - 0x81 + 0x7f) & 0xff;

 /* reassemble them in IEEE 4 byte real number format */

    t.ui[1] = (t.ui[1] & 0x7f) | (exp << 7) | (sign << 15);
    return t.f[0];
} /* End of MSBINToIEEE */



 /* IEEEToMSBIN - Converts an IEEE floating point number  */
 /*               to MSBIN floating point format          */
 /*                                                       */
 /*  Input: f - floating point number in IEEE format      */
 /* Output: Same number in MSBIN format                   */

float IEEEToMSBIN (float f) {

    union Converter t;
    int             sign, exp;       /* sign and exponent */


    t.f[0] = f;

 /* extract sign & change exponent bias from 0x7f to 0x81 */

    sign = t.uc[3] / 0x80;
    exp  = ((t.ui[1] >> 7) - 0x7f + 0x81) & 0xff;

 /* reassemble them in MSBIN format */

    t.ui[1] = (t.ui[1] & 0x7f) | (sign << 7) | (exp << 8);
    return t.f[0];
} /* End of IEEEToMSBIN */

#endif
#endif

