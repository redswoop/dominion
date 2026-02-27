/*
 * jamdump.c -- Dump contents of a JAM message base in human-readable form
 *
 * Usage: jamdump <basepath>
 *   e.g. jamdump msgs/EMAIL  (reads EMAIL.jhr, EMAIL.jdt, EMAIL.jdx, EMAIL.jlr)
 *
 * Build: cc -std=gnu89 -DPD -fsigned-char -Isrc -o build/jamdump tools/jamdump.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#include "jam/jamsys.h"
#include "jam/jam.h"

static long get_filesize(int fd)
{
    off_t pos = lseek(fd, 0, SEEK_CUR);
    off_t end = lseek(fd, 0, SEEK_END);
    lseek(fd, pos, SEEK_SET);
    return (long)end;
}

static const char *sfld_name(UINT16 id)
{
    switch (id) {
    case 0:    return "OADDRESS";
    case 1:    return "DADDRESS";
    case 2:    return "SENDERNAME";
    case 3:    return "RECVRNAME";
    case 4:    return "MSGID";
    case 5:    return "REPLYID";
    case 6:    return "SUBJECT";
    case 7:    return "PID";
    case 8:    return "TRACE";
    case 9:    return "ENCLFILE";
    case 666:  return "COMMENT";
    case 1000: return "EMBINDAT";
    case 2000: return "FTSKLUDGE";
    case 2001: return "SEENBY2D";
    case 2002: return "PATH2D";
    case 2003: return "FLAGS";
    case 2004: return "TZUTCINFO";
    default:   return "UNKNOWN";
    }
}

static const char *fmt_time(UINT32 t)
{
    static char buf[64];
    time_t tt = (time_t)t;
    struct tm *lt = localtime(&tt);
    if (lt)
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
    else
        strcpy(buf, "(invalid)");
    return buf;
}

static void print_attrs(UINT32 attr)
{
    if (attr & 0x00000001L) printf(" LOCAL");
    if (attr & 0x00000002L) printf(" INTRANSIT");
    if (attr & 0x00000004L) printf(" PRIVATE");
    if (attr & 0x00000008L) printf(" READ");
    if (attr & 0x00000010L) printf(" SENT");
    if (attr & 0x00000020L) printf(" KILLSENT");
    if (attr & 0x00000080L) printf(" HOLD");
    if (attr & 0x00800000L) printf(" TYPELOCAL");
    if (attr & 0x01000000L) printf(" TYPEECHO");
    if (attr & 0x02000000L) printf(" TYPENET");
    if (attr & 0x80000000L) printf(" DELETED");
}

int main(int argc, char **argv)
{
    char fn[512];
    int hdr_fd, txt_fd, idx_fd, lrd_fd;
    JAMHDRINFO hdrinfo;
    JAMIDXREC idx;
    JAMHDR hdr;
    long idx_size, num_msgs, i;
    char *sfbuf = NULL;
    char *txtbuf = NULL;

    if (argc < 2) {
        fprintf(stderr, "Usage: jamdump <basepath>\n");
        fprintf(stderr, "  e.g. jamdump msgs/EMAIL\n");
        return 1;
    }

    /* Open .jhr */
    sprintf(fn, "%s.jhr", argv[1]);
    hdr_fd = open(fn, O_RDONLY);
    if (hdr_fd < 0) {
        perror(fn);
        return 1;
    }

    /* Open .jdt */
    sprintf(fn, "%s.jdt", argv[1]);
    txt_fd = open(fn, O_RDONLY);
    if (txt_fd < 0) {
        perror(fn);
        close(hdr_fd);
        return 1;
    }

    /* Open .jdx */
    sprintf(fn, "%s.jdx", argv[1]);
    idx_fd = open(fn, O_RDONLY);
    if (idx_fd < 0) {
        perror(fn);
        close(hdr_fd); close(txt_fd);
        return 1;
    }

    /* Open .jlr (optional) */
    sprintf(fn, "%s.jlr", argv[1]);
    lrd_fd = open(fn, O_RDONLY);

    /* Read header info */
    if (read(hdr_fd, &hdrinfo, sizeof(JAMHDRINFO)) != sizeof(JAMHDRINFO)) {
        fprintf(stderr, "Error reading JAMHDRINFO from .jhr\n");
        close(hdr_fd); close(txt_fd); close(idx_fd);
        return 1;
    }

    printf("JAM Message Base: %s\n", argv[1]);
    printf("========================================\n");
    printf("Header Info:\n");
    printf("  Signature:    %.3s\n", hdrinfo.Signature);
    printf("  Created:      %s\n", fmt_time(hdrinfo.DateCreated));
    printf("  ModCounter:   %lu\n", (unsigned long)hdrinfo.ModCounter);
    printf("  Active Msgs:  %lu\n", (unsigned long)hdrinfo.ActiveMsgs);
    printf("  PasswordCRC:  0x%08lX\n", (unsigned long)hdrinfo.PasswordCRC);
    printf("  Base Msg Num: %lu\n", (unsigned long)hdrinfo.BaseMsgNum);

    /* Count messages from index file size */
    idx_size = get_filesize(idx_fd);
    num_msgs = idx_size / sizeof(JAMIDXREC);
    printf("  Index entries: %ld (file size %ld, rec size %lu)\n",
           num_msgs, idx_size, (unsigned long)sizeof(JAMIDXREC));

    printf("\nStruct sizes: JAMHDRINFO=%lu JAMHDR=%lu JAMIDXREC=%lu "
           "JAMLREAD=%lu JAMBINSUBFIELD=%lu\n",
           (unsigned long)sizeof(JAMHDRINFO),
           (unsigned long)sizeof(JAMHDR),
           (unsigned long)sizeof(JAMIDXREC),
           (unsigned long)sizeof(JAMLREAD),
           (unsigned long)sizeof(JAMBINSUBFIELD));

    /* Iterate messages */
    lseek(idx_fd, 0, SEEK_SET);
    for (i = 0; i < num_msgs; i++) {
        UINT32 msgnum = hdrinfo.BaseMsgNum + i;

        if (read(idx_fd, &idx, sizeof(JAMIDXREC)) != sizeof(JAMIDXREC)) {
            fprintf(stderr, "Error reading index entry %ld\n", i);
            break;
        }

        printf("\n--- Message %lu ---\n", (unsigned long)msgnum);
        printf("  Index: UserCRC=0x%08lX HdrOffset=%lu\n",
               (unsigned long)idx.UserCRC,
               (unsigned long)idx.HdrOffset);

        /* Read header */
        if (lseek(hdr_fd, idx.HdrOffset, SEEK_SET) < 0) {
            printf("  ERROR: seek to offset %lu failed\n",
                   (unsigned long)idx.HdrOffset);
            continue;
        }

        if (read(hdr_fd, &hdr, sizeof(JAMHDR)) != sizeof(JAMHDR)) {
            printf("  ERROR: reading JAMHDR\n");
            continue;
        }

        if (memcmp(hdr.Signature, "JAM", 3) != 0) {
            printf("  ERROR: bad header signature\n");
            continue;
        }

        printf("  Header:\n");
        printf("    MsgNum:      %lu\n", (unsigned long)hdr.MsgNum);
        printf("    Date:        %s\n", fmt_time(hdr.DateWritten));
        printf("    Attribute:   0x%08lX", (unsigned long)hdr.Attribute);
        print_attrs(hdr.Attribute);
        printf("\n");
        printf("    TxtOffset:   %lu\n", (unsigned long)hdr.TxtOffset);
        printf("    TxtLen:      %lu\n", (unsigned long)hdr.TxtLen);
        printf("    SubfieldLen: %lu\n", (unsigned long)hdr.SubfieldLen);
        printf("    ReplyTo:     %lu\n", (unsigned long)hdr.ReplyTo);
        printf("    Reply1st:    %lu\n", (unsigned long)hdr.Reply1st);
        printf("    ReplyNext:   %lu\n", (unsigned long)hdr.ReplyNext);
        printf("    TimesRead:   %lu\n", (unsigned long)hdr.TimesRead);

        /* Read and display subfields */
        if (hdr.SubfieldLen > 0) {
            UINT32 sflen = hdr.SubfieldLen;
            UINT32 remaining;
            char *ptr;

            sfbuf = realloc(sfbuf, sflen);
            if (!sfbuf) {
                printf("  ERROR: out of memory for subfields\n");
                continue;
            }

            if (read(hdr_fd, sfbuf, sflen) != (long)sflen) {
                printf("  ERROR: reading subfields\n");
                continue;
            }

            printf("  Subfields:\n");
            remaining = sflen;
            ptr = sfbuf;

            while (remaining >= sizeof(JAMBINSUBFIELD)) {
                JAMBINSUBFIELD *sf = (JAMBINSUBFIELD *)ptr;
                UINT32 datlen = sf->DatLen;
                UINT32 reclen;
                char *data;
                UINT32 printlen;

                if (datlen + sizeof(JAMBINSUBFIELD) > remaining)
                    break;

                data = ptr + sizeof(JAMBINSUBFIELD);
                printlen = datlen;
                if (printlen > 200) printlen = 200;

                printf("    %-12s [%u]: ", sfld_name(sf->LoID), (unsigned)sf->LoID);
                fwrite(data, 1, printlen, stdout);
                if (printlen < datlen)
                    printf("...");
                printf("\n");

                reclen = datlen + sizeof(JAMBINSUBFIELD);
                /* Align to 4 bytes (matches JAMsysAlign on macOS/POSIX) */
                if (reclen % 4)
                    reclen += 4 - (reclen % 4);
                if (reclen > remaining)
                    break;
                remaining -= reclen;
                ptr += reclen;
            }
        }

        /* Read and display message text */
        if (hdr.TxtLen > 0 && hdr.TxtLen < 0x100000) {
            UINT32 showlen = hdr.TxtLen;
            UINT32 j;

            if (showlen > 500) showlen = 500;

            txtbuf = realloc(txtbuf, showlen + 1);
            if (!txtbuf) {
                printf("  ERROR: out of memory for text\n");
                continue;
            }

            if (lseek(txt_fd, hdr.TxtOffset, SEEK_SET) >= 0 &&
                read(txt_fd, txtbuf, showlen) == (long)showlen) {
                txtbuf[showlen] = '\0';
                /* Replace control chars for display */
                for (j = 0; j < showlen; j++) {
                    if (txtbuf[j] == '\r') txtbuf[j] = '\n';
                    else if (txtbuf[j] < ' ' && txtbuf[j] != '\n' && txtbuf[j] != '\t')
                        txtbuf[j] = '.';
                }
                printf("  Text (%lu bytes):\n    ", (unsigned long)hdr.TxtLen);
                for (j = 0; j < showlen; j++) {
                    putchar(txtbuf[j]);
                    if (txtbuf[j] == '\n' && j + 1 < showlen)
                        printf("    ");
                }
                if (hdr.TxtLen > showlen)
                    printf("\n    [...%lu more bytes...]",
                           (unsigned long)(hdr.TxtLen - showlen));
                printf("\n");
            }
        }
    }

    /* Show lastread records */
    if (lrd_fd >= 0) {
        JAMLREAD lr;
        long lrd_count = 0;

        printf("\n========================================\n");
        printf("LastRead Records:\n");
        lseek(lrd_fd, 0, SEEK_SET);
        while (read(lrd_fd, &lr, sizeof(JAMLREAD)) == sizeof(JAMLREAD)) {
            printf("  [%ld] UserCRC=0x%08lX UserID=%lu LastRead=%lu HighRead=%lu\n",
                   lrd_count,
                   (unsigned long)lr.UserCRC,
                   (unsigned long)lr.UserID,
                   (unsigned long)lr.LastReadMsg,
                   (unsigned long)lr.HighReadMsg);
            lrd_count++;
        }
        if (lrd_count == 0)
            printf("  (none)\n");
        close(lrd_fd);
    }

    if (sfbuf) free(sfbuf);
    if (txtbuf) free(txtbuf);
    close(hdr_fd);
    close(txt_fd);
    close(idx_fd);

    return 0;
}
