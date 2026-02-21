/*
 * jam_stubs.c -- JAM message base library implementation for macOS/POSIX
 *
 * Implements the JAM (Joaquim-Andrew-Mats) C API.
 * Originally stubs; now real implementations using POSIX I/O and flock().
 */

#include "platform.h"
#include "jammb.h"
#include <errno.h>
#include <sys/file.h>

/* ================================================================== */
/*  JAM SYSTEM I/O FUNCTIONS                                           */
/* ================================================================== */

FHANDLE JAMsysOpen(JAMAPIRECptr apirec, CHAR8ptr pFileName)
{
    int fd = open(pFileName, O_RDWR);
    if (fd < 0 && apirec) apirec->Errno = errno;
    return fd;
}

FHANDLE JAMsysCreate(JAMAPIRECptr apirec, CHAR8ptr pFileName)
{
    int fd = open(pFileName, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0 && apirec) apirec->Errno = errno;
    return fd;
}

int JAMsysClose(JAMAPIRECptr apirec, FHANDLE fh)
{
    if (fh >= 0) close(fh);
    return 1;
}

INT32 JAMsysRead(JAMAPIRECptr apirec, FHANDLE fh, VOIDptr pBuf, INT32 Len)
{
    INT32 n = read(fh, pBuf, Len);
    if (n < 0 && apirec) apirec->Errno = errno;
    return n;
}

INT32 JAMsysWrite(JAMAPIRECptr apirec, FHANDLE fh, VOIDptr pBuf, INT32 Len)
{
    return write(fh, pBuf, Len);
}

INT32 JAMsysSeek(JAMAPIRECptr apirec, FHANDLE fh, int FromWhere, INT32 Offset)
{
    INT32 pos = lseek(fh, Offset, FromWhere);
    if (pos < 0 && apirec) apirec->Errno = errno;
    return pos;
}

int JAMsysLock(JAMAPIRECptr apirec, int DoLock)
{
    if (!apirec || apirec->HdrHandle < 0) return 0;
    if (DoLock)
        return flock(apirec->HdrHandle, LOCK_EX | LOCK_NB) == 0;
    else
        return flock(apirec->HdrHandle, LOCK_UN) == 0;
}

int JAMsysUnlink(JAMAPIRECptr apirec, CHAR8ptr pFileName)
{
    return unlink(pFileName) == 0;
}

FHANDLE JAMsysSopen(JAMAPIRECptr apirec, CHAR8ptr pFileName, int AccessMode, int ShareMode)
{
    int fd = open(pFileName, AccessMode);
    if (fd < 0 && apirec) apirec->Errno = errno;
    return fd;
}

/* ================================================================== */
/*  JAM API RECORD MANAGEMENT                                          */
/* ================================================================== */

int JAMsysInitApiRec(JAMAPIRECptr apirec, CHAR8ptr pFile, UINT32 Size)
{
    if (!apirec || !pFile) return 0;
    memset(apirec, 0, sizeof(JAMAPIREC));
    strncpy(apirec->BaseName, pFile, sizeof(apirec->BaseName) - 1);
    apirec->WorkBuf = malloc(Size);
    if (!apirec->WorkBuf) return 0;
    memset(apirec->WorkBuf, 0, Size);
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
    if (apirec->HdrHandle >= 0) { close(apirec->HdrHandle); apirec->HdrHandle = -1; }
    if (apirec->TxtHandle >= 0) { close(apirec->TxtHandle); apirec->TxtHandle = -1; }
    if (apirec->IdxHandle >= 0) { close(apirec->IdxHandle); apirec->IdxHandle = -1; }
    if (apirec->LrdHandle >= 0) { close(apirec->LrdHandle); apirec->LrdHandle = -1; }
    if (apirec->WorkBuf) { free(apirec->WorkBuf); apirec->WorkBuf = NULL; }
    apirec->isOpen = 0;
    apirec->HaveLock = 0;
    return 1;
}

/* ================================================================== */
/*  JAM MESSAGE BASE OPERATIONS                                        */
/* ================================================================== */

int JAMmbOpen(JAMAPIRECptr apirec)
{
    CHAR8 fn[256];

    if (!apirec) return 0;
    if (apirec->isOpen) {
        apirec->APImsg = JAMAPIMSG_ISOPEN;
        return 0;
    }

    sprintf(fn, "%s%s", apirec->BaseName, EXT_HDRFILE);
    apirec->HdrHandle = open(fn, O_RDWR);
    if (apirec->HdrHandle < 0) {
        apirec->Errno = errno;
        return 0;
    }

    sprintf(fn, "%s%s", apirec->BaseName, EXT_TXTFILE);
    apirec->TxtHandle = open(fn, O_RDWR);
    if (apirec->TxtHandle < 0) {
        apirec->Errno = errno;
        close(apirec->HdrHandle); apirec->HdrHandle = -1;
        return 0;
    }

    sprintf(fn, "%s%s", apirec->BaseName, EXT_IDXFILE);
    apirec->IdxHandle = open(fn, O_RDWR);
    if (apirec->IdxHandle < 0) {
        apirec->Errno = errno;
        close(apirec->HdrHandle); apirec->HdrHandle = -1;
        close(apirec->TxtHandle); apirec->TxtHandle = -1;
        return 0;
    }

    sprintf(fn, "%s%s", apirec->BaseName, EXT_LRDFILE);
    apirec->LrdHandle = open(fn, O_RDWR | O_CREAT, 0644);
    if (apirec->LrdHandle < 0) {
        apirec->Errno = errno;
        close(apirec->HdrHandle); apirec->HdrHandle = -1;
        close(apirec->TxtHandle); apirec->TxtHandle = -1;
        close(apirec->IdxHandle); apirec->IdxHandle = -1;
        return 0;
    }

    if (read(apirec->HdrHandle, &apirec->HdrInfo, sizeof(JAMHDRINFO))
        != sizeof(JAMHDRINFO)) {
        apirec->APImsg = JAMAPIMSG_CANTRDFILE;
        apirec->Errno = errno;
        close(apirec->HdrHandle); apirec->HdrHandle = -1;
        close(apirec->TxtHandle); apirec->TxtHandle = -1;
        close(apirec->IdxHandle); apirec->IdxHandle = -1;
        close(apirec->LrdHandle); apirec->LrdHandle = -1;
        return 0;
    }

    if (memcmp(apirec->HdrInfo.Signature, HEADERSIGNATURE, 3) != 0) {
        apirec->APImsg = JAMAPIMSG_HDRPRINT;
        close(apirec->HdrHandle); apirec->HdrHandle = -1;
        close(apirec->TxtHandle); apirec->TxtHandle = -1;
        close(apirec->IdxHandle); apirec->IdxHandle = -1;
        close(apirec->LrdHandle); apirec->LrdHandle = -1;
        return 0;
    }

    apirec->isOpen = 1;
    return 1;
}

int JAMmbCreate(JAMAPIRECptr apirec)
{
    CHAR8 fn[256];

    if (!apirec) return 0;

    sprintf(fn, "%s%s", apirec->BaseName, EXT_HDRFILE);
    apirec->HdrHandle = open(fn, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (apirec->HdrHandle < 0) {
        apirec->APImsg = JAMAPIMSG_CANTMKFILE;
        apirec->Errno = errno;
        return 0;
    }

    sprintf(fn, "%s%s", apirec->BaseName, EXT_TXTFILE);
    apirec->TxtHandle = open(fn, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (apirec->TxtHandle < 0) {
        apirec->APImsg = JAMAPIMSG_CANTMKFILE;
        apirec->Errno = errno;
        close(apirec->HdrHandle); apirec->HdrHandle = -1;
        return 0;
    }

    sprintf(fn, "%s%s", apirec->BaseName, EXT_IDXFILE);
    apirec->IdxHandle = open(fn, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (apirec->IdxHandle < 0) {
        apirec->APImsg = JAMAPIMSG_CANTMKFILE;
        apirec->Errno = errno;
        close(apirec->HdrHandle); apirec->HdrHandle = -1;
        close(apirec->TxtHandle); apirec->TxtHandle = -1;
        return 0;
    }

    sprintf(fn, "%s%s", apirec->BaseName, EXT_LRDFILE);
    apirec->LrdHandle = open(fn, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (apirec->LrdHandle < 0) {
        apirec->APImsg = JAMAPIMSG_CANTMKFILE;
        apirec->Errno = errno;
        close(apirec->HdrHandle); apirec->HdrHandle = -1;
        close(apirec->TxtHandle); apirec->TxtHandle = -1;
        close(apirec->IdxHandle); apirec->IdxHandle = -1;
        return 0;
    }

    memset(&apirec->HdrInfo, 0, sizeof(JAMHDRINFO));
    memcpy(apirec->HdrInfo.Signature, HEADERSIGNATURE, 3);
    apirec->HdrInfo.Signature[3] = '\0';
    apirec->HdrInfo.DateCreated = (UINT32)time(NULL);
    apirec->HdrInfo.BaseMsgNum = 1;

    if (write(apirec->HdrHandle, &apirec->HdrInfo, sizeof(JAMHDRINFO))
        != sizeof(JAMHDRINFO)) {
        apirec->APImsg = JAMAPIMSG_CANTWRFILE;
        apirec->Errno = errno;
        close(apirec->HdrHandle); apirec->HdrHandle = -1;
        close(apirec->TxtHandle); apirec->TxtHandle = -1;
        close(apirec->IdxHandle); apirec->IdxHandle = -1;
        close(apirec->LrdHandle); apirec->LrdHandle = -1;
        return 0;
    }

    apirec->isOpen = 1;
    return 1;
}

int JAMmbClose(JAMAPIRECptr apirec)
{
    if (!apirec) return 0;
    if (!apirec->isOpen) {
        apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
        return 0;
    }

    if (apirec->HdrHandle >= 0) { close(apirec->HdrHandle); apirec->HdrHandle = -1; }
    if (apirec->TxtHandle >= 0) { close(apirec->TxtHandle); apirec->TxtHandle = -1; }
    if (apirec->IdxHandle >= 0) { close(apirec->IdxHandle); apirec->IdxHandle = -1; }
    if (apirec->LrdHandle >= 0) { close(apirec->LrdHandle); apirec->LrdHandle = -1; }

    apirec->isOpen = 0;
    apirec->HaveLock = 0;
    return 1;
}

int JAMmbFetchMsgIdx(JAMAPIRECptr apirec, UINT32 msgnum)
{
    INT32 offset;

    if (!apirec) return 0;
    if (!apirec->isOpen) {
        apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
        return 0;
    }

    offset = (INT32)(msgnum - apirec->HdrInfo.BaseMsgNum) * sizeof(JAMIDXREC);

    if (lseek(apirec->IdxHandle, offset, SEEK_SET) < 0) {
        apirec->APImsg = JAMAPIMSG_SEEKERROR;
        apirec->Errno = errno;
        return 0;
    }

    if (read(apirec->IdxHandle, &apirec->Idx, sizeof(JAMIDXREC))
        != sizeof(JAMIDXREC)) {
        apirec->APImsg = JAMAPIMSG_CANTRDFILE;
        apirec->Errno = errno;
        return 0;
    }

    apirec->LastMsgNum = msgnum;
    return 1;
}

int JAMmbFetchMsgHdr(JAMAPIRECptr apirec, UINT32 msgnum, int fetchSubFields)
{
    if (!apirec) return 0;
    if (!apirec->isOpen) {
        apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
        return 0;
    }

    if (!JAMmbFetchMsgIdx(apirec, msgnum))
        return 0;

    if (lseek(apirec->HdrHandle, apirec->Idx.HdrOffset, SEEK_SET) < 0) {
        apirec->APImsg = JAMAPIMSG_SEEKERROR;
        apirec->Errno = errno;
        return 0;
    }

    if (read(apirec->HdrHandle, &apirec->Hdr, sizeof(JAMHDR))
        != sizeof(JAMHDR)) {
        apirec->APImsg = JAMAPIMSG_CANTRDFILE;
        apirec->Errno = errno;
        return 0;
    }

    if (memcmp(apirec->Hdr.Signature, HEADERSIGNATURE, 3) != 0) {
        apirec->APImsg = JAMAPIMSG_BADHEADERSIG;
        return 0;
    }

    if (fetchSubFields && apirec->Hdr.SubfieldLen > 0) {
        UINT32 readLen = apirec->Hdr.SubfieldLen;
        if (readLen > apirec->WorkLen)
            readLen = apirec->WorkLen;

        if (read(apirec->HdrHandle, apirec->WorkBuf, readLen)
            != (INT32)readLen) {
            apirec->APImsg = JAMAPIMSG_CANTRDFILE;
            apirec->Errno = errno;
            return 0;
        }
    }

    apirec->LastMsgNum = msgnum;
    return 1;
}

int JAMmbFetchMsgTxt(JAMAPIRECptr apirec, int fetchMode)
{
    UINT32 readLen;

    if (!apirec) return 0;
    if (!apirec->isOpen) {
        apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
        return 0;
    }

    if (lseek(apirec->TxtHandle, apirec->Hdr.TxtOffset, SEEK_SET) < 0) {
        apirec->APImsg = JAMAPIMSG_SEEKERROR;
        apirec->Errno = errno;
        return 0;
    }

    readLen = apirec->Hdr.TxtLen;
    if (readLen > apirec->WorkLen - 1)
        readLen = apirec->WorkLen - 1;

    if (readLen > 0) {
        if (read(apirec->TxtHandle, apirec->WorkBuf, readLen)
            != (INT32)readLen) {
            apirec->APImsg = JAMAPIMSG_CANTRDFILE;
            apirec->Errno = errno;
            return 0;
        }
    }

    apirec->WorkBuf[readLen] = '\0';
    return 1;
}

int JAMmbStoreMsgHdr(JAMAPIRECptr apirec, UINT32 msgnum)
{
    if (!apirec) return 0;
    if (!apirec->isOpen) {
        apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
        return 0;
    }
    if (!apirec->HaveLock) {
        apirec->APImsg = JAMAPIMSG_ISNOTLOCKED;
        return 0;
    }

    memcpy(apirec->Hdr.Signature, HEADERSIGNATURE, 3);
    apirec->Hdr.Signature[3] = '\0';
    apirec->Hdr.Revision = CURRENTREVLEV;

    if (lseek(apirec->HdrHandle, apirec->Idx.HdrOffset, SEEK_SET) < 0) {
        apirec->APImsg = JAMAPIMSG_SEEKERROR;
        apirec->Errno = errno;
        return 0;
    }

    if (write(apirec->HdrHandle, &apirec->Hdr, sizeof(JAMHDR))
        != sizeof(JAMHDR)) {
        apirec->APImsg = JAMAPIMSG_CANTWRFILE;
        apirec->Errno = errno;
        return 0;
    }

    return 1;
}

int JAMmbStoreMsgIdx(JAMAPIRECptr apirec, UINT32 msgnum)
{
    INT32 offset;

    if (!apirec) return 0;
    if (!apirec->isOpen) {
        apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
        return 0;
    }
    if (!apirec->HaveLock) {
        apirec->APImsg = JAMAPIMSG_ISNOTLOCKED;
        return 0;
    }

    offset = (INT32)(msgnum - apirec->HdrInfo.BaseMsgNum) * sizeof(JAMIDXREC);

    if (lseek(apirec->IdxHandle, offset, SEEK_SET) < 0) {
        apirec->APImsg = JAMAPIMSG_SEEKERROR;
        apirec->Errno = errno;
        return 0;
    }

    if (write(apirec->IdxHandle, &apirec->Idx, sizeof(JAMIDXREC))
        != sizeof(JAMIDXREC)) {
        apirec->APImsg = JAMAPIMSG_CANTWRFILE;
        apirec->Errno = errno;
        return 0;
    }

    return 1;
}

int JAMmbStoreMsgTxtBuf(JAMAPIRECptr apirec, CHAR8ptr buf, INT32 len, int mode)
{
    if (!apirec) return 0;
    if (!apirec->isOpen) {
        apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
        return 0;
    }

    if (lseek(apirec->TxtHandle, 0, SEEK_END) < 0) {
        apirec->APImsg = JAMAPIMSG_SEEKERROR;
        apirec->Errno = errno;
        return 0;
    }

    if (write(apirec->TxtHandle, buf, len) != len) {
        apirec->APImsg = JAMAPIMSG_CANTWRFILE;
        apirec->Errno = errno;
        return 0;
    }

    return 1;
}

int JAMmbLockMsgBase(JAMAPIRECptr apirec, int doLock)
{
    if (!apirec) return 0;
    if (!apirec->isOpen) {
        apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
        return 0;
    }

    if (flock(apirec->HdrHandle, LOCK_EX | LOCK_NB) < 0) {
        apirec->APImsg = JAMAPIMSG_CANTLKFILE;
        apirec->Errno = errno;
        return 0;
    }

    /* Re-read header info after acquiring lock */
    lseek(apirec->HdrHandle, 0, SEEK_SET);
    read(apirec->HdrHandle, &apirec->HdrInfo, sizeof(JAMHDRINFO));

    apirec->HaveLock = 1;
    return 1;
}

int JAMmbUnLockMsgBase(JAMAPIRECptr apirec, int doUnlock)
{
    if (!apirec) return 0;
    if (!apirec->isOpen) {
        apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
        return 0;
    }

    /* Write header info back before unlocking */
    lseek(apirec->HdrHandle, 0, SEEK_SET);
    write(apirec->HdrHandle, &apirec->HdrInfo, sizeof(JAMHDRINFO));

    flock(apirec->HdrHandle, LOCK_UN);
    apirec->HaveLock = 0;
    return 1;
}

int JAMmbFetchLastRead(JAMAPIRECptr apirec, UINT32 usernum)
{
    JAMLREAD lr;
    UINT32 recnum = 0;

    if (!apirec) return 0;
    if (!apirec->isOpen) {
        apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
        return 0;
    }

    lseek(apirec->LrdHandle, 0, SEEK_SET);

    while (read(apirec->LrdHandle, &lr, sizeof(JAMLREAD)) == sizeof(JAMLREAD)) {
        if (lr.UserID == usernum) {
            apirec->LastRead = lr;
            apirec->LastLRDnum = recnum;
            return 1;
        }
        recnum++;
    }

    apirec->APImsg = JAMAPIMSG_CANTFINDUSER;
    return 0;
}

int JAMmbStoreLastRead(JAMAPIRECptr apirec, int mode)
{
    INT32 offset;

    if (!apirec) return 0;
    if (!apirec->isOpen) {
        apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
        return 0;
    }

    offset = (INT32)apirec->LastLRDnum * sizeof(JAMLREAD);

    if (lseek(apirec->LrdHandle, offset, SEEK_SET) < 0) {
        apirec->APImsg = JAMAPIMSG_SEEKERROR;
        apirec->Errno = errno;
        return 0;
    }

    if (write(apirec->LrdHandle, &apirec->LastRead, sizeof(JAMLREAD))
        != sizeof(JAMLREAD)) {
        apirec->APImsg = JAMAPIMSG_CANTWRFILE;
        apirec->Errno = errno;
        return 0;
    }

    return 1;
}

int JAMmbAddField(JAMAPIRECptr apirec, UINT32 fldType, int fldLen,
                  unsigned int fldAttr, UINT32ptr pPos, CHAR8ptr pData)
{
    JAMBINSUBFIELD sf;
    UINT32 alignedLen;

    if (!apirec || !pPos || !pData) return 0;

    alignedLen = JAMsysAlign(sizeof(JAMBINSUBFIELD) + fldAttr);

    if (*pPos + alignedLen > apirec->WorkLen)
        return 0;

    sf.LoID = (UINT16)fldType;
    sf.HiID = (UINT16)fldLen;
    sf.DatLen = (UINT32)fldAttr;

    memcpy(apirec->WorkBuf + *pPos, &sf, sizeof(JAMBINSUBFIELD));
    memcpy(apirec->WorkBuf + *pPos + sizeof(JAMBINSUBFIELD), pData, fldAttr);

    *pPos += alignedLen;

    return 1;
}

int JAMmbUpdateHeaderInfo(JAMAPIRECptr apirec, int mode)
{
    if (!apirec) return 0;
    if (!apirec->isOpen) {
        apirec->APImsg = JAMAPIMSG_ISNOTOPEN;
        return 0;
    }

    if (lseek(apirec->HdrHandle, 0, SEEK_SET) < 0) {
        apirec->APImsg = JAMAPIMSG_SEEKERROR;
        apirec->Errno = errno;
        return 0;
    }

    if (write(apirec->HdrHandle, &apirec->HdrInfo, sizeof(JAMHDRINFO))
        != sizeof(JAMHDRINFO)) {
        apirec->APImsg = JAMAPIMSG_CANTWRFILE;
        apirec->Errno = errno;
        return 0;
    }

    return 1;
}

/* ================================================================== */
/*  JAM CRC-32                                                         */
/* ================================================================== */

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
