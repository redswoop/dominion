/*
 * jam_stubs.c — Stub implementations of the JAM message base C API
 *
 * The JAM (Joaquim-Andrew-Mats) library was distributed separately.
 * These stubs allow the BBS to link; message base operations will
 * need real implementations for actual use.
 */

#include "platform.h"
#include "jammb.h"

/* ================================================================== */
/*  JAM API RECORD MANAGEMENT                                          */
/* ================================================================== */

int JAMsysInitApiRec(JAMAPIRECptr apirec, CHAR8ptr pFile, UINT32 Size)
{
    if (!apirec || !pFile) return 0;
    memset(apirec, 0, sizeof(JAMAPIREC));
    strncpy(apirec->BaseName, pFile, sizeof(apirec->BaseName) - 1);
    apirec->WorkBuf = malloc(Size);
    apirec->WorkLen = Size;
    apirec->HdrHandle = -1;
    apirec->TxtHandle = -1;
    apirec->IdxHandle = -1;
    apirec->LrdHandle = -1;
    return 1;
}

int JAMsysDeinitApiRec(JAMAPIRECptr apirec)
{
    if (!apirec) return 0;
    if (apirec->WorkBuf) free(apirec->WorkBuf);
    apirec->WorkBuf = NULL;
    apirec->isOpen = 0;
    return 1;
}

/* ================================================================== */
/*  JAM MESSAGE BASE OPERATIONS (stubs)                                */
/* ================================================================== */

int JAMmbOpen(JAMAPIRECptr apirec)
{
    if (!apirec) return 0;
    apirec->isOpen = 1;
    return 1;
}

int JAMmbCreate(JAMAPIRECptr apirec)
{
    if (!apirec) return -1;
    apirec->APImsg = JAMAPIMSG_CANTMKFILE;
    return -1;
}

int JAMmbClose(JAMAPIRECptr apirec)
{
    if (!apirec) return -1;
    apirec->isOpen = 0;
    return 0;
}

int JAMmbFetchMsgHdr(JAMAPIRECptr apirec, UINT32 msgnum, int fetchSubFields)
{
    if (!apirec) return -1;
    apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
    return -1;
}

int JAMmbFetchMsgIdx(JAMAPIRECptr apirec, UINT32 msgnum)
{
    if (!apirec) return -1;
    apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
    return -1;
}

int JAMmbFetchMsgTxt(JAMAPIRECptr apirec, int fetchMode)
{
    if (!apirec) return -1;
    apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
    return -1;
}

int JAMmbStoreMsgHdr(JAMAPIRECptr apirec, UINT32 msgnum)
{
    if (!apirec) return -1;
    apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
    return -1;
}

int JAMmbStoreMsgIdx(JAMAPIRECptr apirec, UINT32 msgnum)
{
    if (!apirec) return -1;
    apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
    return -1;
}

int JAMmbStoreMsgTxtBuf(JAMAPIRECptr apirec, CHAR8ptr buf, INT32 len, int mode)
{
    if (!apirec) return -1;
    apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
    return -1;
}

int JAMmbLockMsgBase(JAMAPIRECptr apirec, int doLock)
{
    if (!apirec) return -1;
    apirec->HaveLock = doLock;
    return 0;
}

int JAMmbUnLockMsgBase(JAMAPIRECptr apirec, int doUnlock)
{
    if (!apirec) return -1;
    apirec->HaveLock = 0;
    return 0;
}

int JAMmbFetchLastRead(JAMAPIRECptr apirec, UINT32 usernum)
{
    if (!apirec) return -1;
    memset(&apirec->LastRead, 0, sizeof(JAMLREAD));
    return 0;
}

int JAMmbStoreLastRead(JAMAPIRECptr apirec, int mode)
{
    if (!apirec) return -1;
    return 0;
}

int JAMmbAddField(JAMAPIRECptr apirec, UINT32 fldType, int fldLen,
                  unsigned int fldAttr, UINT32ptr pPos, CHAR8ptr pData)
{
    if (!apirec) return -1;
    return 0;
}

int JAMmbUpdateHeaderInfo(JAMAPIRECptr apirec, int mode)
{
    if (!apirec) return -1;
    return 0;
}

/* ================================================================== */
/*  JAM SYSTEM I/O FUNCTIONS                                           */
/* ================================================================== */

INT32 JAMsysWrite(JAMAPIRECptr apirec, FHANDLE fh, VOIDptr pBuf, INT32 Len)
{
    return write(fh, pBuf, Len);
}

/* ================================================================== */
/*  JAM CRC-32                                                         */
/* ================================================================== */

/* Standard CRC-32 (same polynomial as zlib/PNG) */
static UINT32 crc32_tab[256];
static int crc32_inited = 0;

static void init_crc32(void)
{
    UINT32 c;
    int n, k;
    for (n = 0; n < 256; n++) {
        c = (UINT32)n;
        for (k = 0; k < 8; k++) {
            if (c & 1)
                c = 0xEDB88320UL ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc32_tab[n] = c;
    }
    crc32_inited = 1;
}

UINT32 JAMsysCrc32(void *pBuf, unsigned int len, UINT32 crc)
{
    unsigned char *buf = (unsigned char *)pBuf;
    unsigned int i;

    if (!crc32_inited) init_crc32();

    crc = crc ^ 0xFFFFFFFFUL;
    for (i = 0; i < len; i++) {
        crc = crc32_tab[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFUL;
}

/* ================================================================== */
/*  JAM TIME FUNCTIONS                                                 */
/* ================================================================== */

UINT32 JAMsysTime(UINT32ptr pTime)
{
    UINT32 t = (UINT32)time(NULL);
    if (pTime) *pTime = t;
    return t;
}

UINT32 JAMsysMkTime(JAMTMptr pTm)
{
    struct tm tm;
    tm.tm_sec = pTm->tm_sec;
    tm.tm_min = pTm->tm_min;
    tm.tm_hour = pTm->tm_hour;
    tm.tm_mday = pTm->tm_mday;
    tm.tm_mon = pTm->tm_mon;
    tm.tm_year = pTm->tm_year;
    tm.tm_isdst = -1;
    return (UINT32)mktime(&tm);
}

JAMTMptr JAMsysLocalTime(UINT32ptr pt)
{
    static JAMTM jtm;
    time_t t = (time_t)*pt;
    struct tm *lt = localtime(&t);
    jtm.tm_sec = lt->tm_sec;
    jtm.tm_min = lt->tm_min;
    jtm.tm_hour = lt->tm_hour;
    jtm.tm_mday = lt->tm_mday;
    jtm.tm_mon = lt->tm_mon;
    jtm.tm_year = lt->tm_year;
    jtm.tm_wday = lt->tm_wday;
    jtm.tm_yday = lt->tm_yday;
    jtm.tm_isdst = lt->tm_isdst;
    return &jtm;
}
