/*
 * mnudump.c - Dump Dominion BBS .mnu files as human-readable ASCII
 *
 * Reads the binary menu file format and displays all fields with
 * embedded color codes stripped.  Auto-detects old (v2.x) vs new
 * (v3.x) menu format based on file size arithmetic.
 *
 * Usage: ./mnudump <file.mnu> [file2.mnu ...]
 *        ./mnudump menus/main.mnu menus/file.mnu ...
 *
 * Build: cc -std=gnu89 -DPD -fsigned-char -Isrc -o mnudump tools/mnudump.c
 *
 * ============================================================================
 * MENU FILE BINARY FORMAT
 * ============================================================================
 *
 * A .mnu file is: one header record followed by N command records.
 * All structs are packed (no padding between fields).
 *
 * TWO FORMATS EXIST (auto-detected by this tool):
 *
 * ----------------------------------------------------------------------------
 * OLD FORMAT (v2.x / dist/ files from the v3.0 snapshot)
 * ----------------------------------------------------------------------------
 *
 * Old header (oldmmrec) — 312 bytes:
 *   char prompt[100]      Menu prompt string
 *   char title1[80]       Title line 1
 *   char title2[80]       Title line 2
 *   char altmenu[12]      External ANSI menu filename
 *   char res[3]           Reserved
 *   char fallback[15]     Packed display config:
 *                           [0]=bracket color, [1]=key color, [2]=desc color,
 *                           [3]=columns, [4]=3D flag, [5]=border attr,
 *                           [6]=hotkey mode (0=normal,1=forced,2=off)
 *   char slneed[10]       ACS string required to enter menu
 *   char pausefile[10]    MCI name
 *   INT16 helplevel       Help level (2 bytes, 16-bit int on DOS)
 *
 * Old command (oldmenurec) — 189 bytes:
 *   char desc[50]         Display description (may have embedded colors)
 *   char key[8]           Hotkey(s) that trigger this command
 *   char type[2]          2-char command type code (e.g. "MR", "=/", "OX")
 *   char at               Attribute as single char: H/U/T/P/F/D or 0
 *   char line[40]         Command line / display text
 *   char ms[80]           Parameter string passed to command handler
 *   char sl[8]            ACS string (access control)
 *
 * ----------------------------------------------------------------------------
 * NEW FORMAT (v3.x / current BBS code, #pragma pack(push,1))
 * ----------------------------------------------------------------------------
 *
 * New header (mmrec) — 471 bytes (PD build):
 *   char prompt[101]      Menu prompt string
 *   char prompt2[91]      Pulldown prompt (PD build only)
 *   char helpfile[10]     External help filename (.HLP)
 *   char title1[101]      Title line 1
 *   char title2[101]      Title line 2
 *   char altmenu[9]       External ANSI menu filename
 *   char format[9]        Format file (.FMT)
 *   char slneed[31]       ACS string required to enter menu
 *   char pausefile[9]     MCI name for menu
 *   char helplevel        Forced help level (-1 = disabled)
 *   char columns          Number of display columns (1-5)
 *   char col[3]           Color indices: [0]=brackets, [1]=keys, [2]=desc
 *   char boarder          Hotkey mode: 0=normal, 1=forced, 2=off
 *   char battr            Border attribute
 *   INT16 attr            Menu attribute flags (bitmask):
 *                           0x0001=ext-prompt, 0x0002=pulldown,
 *                           0x0004=format, 0x0008=prompt-append,
 *                           0x0010=popup, 0x0020=no-global,
 *                           0x0040=hide-global
 *
 * New command (menurec) — 217 bytes:
 *   char desc[50]         Display description (may have embedded colors)
 *   char key[21]          Hotkey(s) that trigger this command
 *   char type[3]          2-char command type code + null
 *   char line[40]         Command line / extra text
 *   char ms[80]           Parameter string passed to command handler
 *   char sl[21]           ACS string (access control)
 *   INT16 attr            Command attribute flags (bitmask):
 *                           0x0001=hidden, 0x0002=unhidden,
 *                           0x0004=title, 0x0008=pulldown-sep,
 *                           0x0010=forced, 0x0020=every,
 *                           0x0040=default
 *
 * ============================================================================
 * EMBEDDED COLOR CODES
 * ============================================================================
 *
 * Text fields (desc, line, ms, prompt, title) may contain:
 *   \x03 + digit     Color 0-9 from user's palette
 *   \x0E + digit     Color 10-19 (extended palette)
 *   \x05 + digit     Alternate color code (same as \x03)
 *   \x06 + byte      "Easy color" (byte 1-20 maps to palette index)
 *   \x16 + 2 bytes   Avatar color sequence (3 bytes total)
 *   |XX              Pipe MCI code (3 chars: pipe + 2 chars)
 *   `X               Backtick MCI code (2 chars)
 *
 * ============================================================================
 * COMMAND TYPE CODES (first character of type field)
 * ============================================================================
 *
 *   M  Message commands    MR=read, MP=post, ME=email, MN=newscan, etc.
 *   F  File commands       FL=list, FU=upload, FD=download, FN=newscan, etc.
 *   D  Doors/externals     D1..D9=run door by number
 *   S  Sysop functions     SU=user edit, SS=sub edit, SD=dir edit, etc.
 *   O  Other commands      OX=autoval, OW=callers, O1=oneliner, OC=chat,
 *                          OF=show file, OI=sysinfo, OU=userlist, OV=vote,
 *                          OT=timebank, OP=help toggle, OL=noop
 *   I  Hangup/logoff       IL=logoff (asks), IH=hangup (immediate)
 *   =  Navigation          =/=submenu, =\=back, =^=goto, =*=list menus,
 *                          =+=next sub, -=-prev sub, =J=jump conference
 *   ?  Display             ??=show menu
 *   Q  BBS list
 *   J  Auto-message
 *   W  Matrix (pre-login menu)
 *
 * ============================================================================
 * ACS STRING SYNTAX (used in sl[] and slneed[])
 * ============================================================================
 *
 *   S##      Security level >= ##
 *   D##      DSL (download security) >= ##
 *   A[A-P]   Board access restriction flag required
 *   I[A-P]   Directory access restriction flag required
 *   G##      Age >= ##
 *   B##      Baud rate >= ## (in hundreds)
 *   U##      User number == ##
 *   !        Negate next condition
 *   &        AND separator between conditions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "vardec.h"

/* Old format struct sizes (from legacy/convmnu.c) */
#define OLD_MMREC_SIZE   312
#define OLD_MENUREC_SIZE 189

/*
 * strip_colors - Remove embedded BBS color codes from a string buffer.
 *
 * Copies up to 'maxlen' source bytes (stopping at null terminator),
 * stripping color codes, into caller-provided 'out' buffer of 'outsz'.
 */
static void strip_colors(const char *src, int maxlen, char *out, int outsz)
{
    int i = 0, o = 0;
    unsigned char c;

    while (i < maxlen && o < outsz - 1) {
        c = (unsigned char)src[i];
        if (c == 0) break;

        if (c == 3 || c == 5 || c == 14) {
            i += 2;    /* \x03/\x05/\x0E + digit */
        } else if (c == 6) {
            i += 2;    /* \x06 + byte */
        } else if (c == 22) {
            i += 3;    /* avatar: \x16 + 2 bytes */
        } else if (c == 0x96) {
            i += 1;    /* title prefix marker */
        } else if (c == '|' && i + 2 < maxlen
                   && isdigit((unsigned char)src[i+1])
                   && isdigit((unsigned char)src[i+2])) {
            i += 3;    /* pipe MCI: |## (two digits) */
        } else if (c == '`' && i + 1 < maxlen && src[i+1]) {
            i += 2;    /* backtick MCI: `X */
        } else {
            out[o++] = src[i++];
        }
    }
    out[o] = 0;
}

/*
 * old_attr_char - Convert old format single-char attribute to string.
 */
static const char *old_attr_str(unsigned char at)
{
    switch (at) {
    case 'H': return "hidden";
    case 'U': return "unhidden";
    case 'T': return "title";
    case 'P': return "pulldown-sep";
    case 'F': return "forced";
    case 'D': return "default";
    }
    if (at == 0) return "";
    return "";
}

/*
 * new_attr_str - Convert new format bitmask attribute to string.
 */
static const char *new_cmd_attr_str(short attr)
{
    if (attr & 0x0001) return "hidden";
    if (attr & 0x0002) return "unhidden";
    if (attr & 0x0004) return "title";
    if (attr & 0x0008) return "pulldown-sep";
    if (attr & 0x0010) return "forced";
    if (attr & 0x0020) return "every";
    if (attr & 0x0040) return "default";
    return "";
}

static const char *hotkey_str(int boarder)
{
    if (boarder == 1) return "forced";
    if (boarder == 2) return "off";
    return "normal";
}

static char *menu_attr_str(short attr)
{
    static char buf[256];
    buf[0] = 0;
    if (attr & 0x0001) strcat(buf, "ext-prompt ");
    if (attr & 0x0002) strcat(buf, "pulldown ");
    if (attr & 0x0004) strcat(buf, "format ");
    if (attr & 0x0008) strcat(buf, "prompt-append ");
    if (attr & 0x0010) strcat(buf, "popup ");
    if (attr & 0x0020) strcat(buf, "no-global ");
    if (attr & 0x0040) strcat(buf, "hide-global ");
    if (buf[0]) buf[strlen(buf) - 1] = 0;
    return buf;
}

/*
 * detect_format - Returns 1 for old format, 0 for new format, -1 for unknown.
 */
static int detect_format(long fsize)
{
    long rem;

    /* Try new format: mmrec(471) + N * menurec(217) */
    rem = fsize - (long)sizeof(mmrec);
    if (rem >= 0 && (rem % (long)sizeof(menurec)) == 0)
        return 0;

    /* Try old format: oldmmrec(312) + N * oldmenurec(189) */
    rem = fsize - OLD_MMREC_SIZE;
    if (rem >= 0 && (rem % OLD_MENUREC_SIZE) == 0)
        return 1;

    return -1;
}

static void dump_old_format(const unsigned char *data, long fsize, const char *path)
{
    int ncmds, i;
    int off;
    char buf[512];
    short hlp;

    ncmds = (fsize - OLD_MMREC_SIZE) / OLD_MENUREC_SIZE;

    printf("================================================================================\n");
    printf("MENU: %s  (old v2.x format)\n", path);
    printf("================================================================================\n");

    /* Parse old header: prompt[100] title1[80] title2[80] altmenu[12]
       res[3] fallback[15] slneed[10] pausefile[10] helplevel(2) */
    off = 0;

    strip_colors((char *)&data[off], 100, buf, sizeof(buf));
    printf("  Prompt     : %s\n", buf);
    off += 100;

    strip_colors((char *)&data[off], 80, buf, sizeof(buf));
    printf("  Title 1    : %s\n", buf);
    off += 80;

    strip_colors((char *)&data[off], 80, buf, sizeof(buf));
    if (buf[0]) printf("  Title 2    : %s\n", buf);
    off += 80;

    strip_colors((char *)&data[off], 12, buf, sizeof(buf));
    if (buf[0]) printf("  ANSI Menu  : %s\n", buf);
    off += 12;

    off += 3; /* res[3] */

    /* fallback[15]: [0]=brackets, [1]=keys, [2]=desc, [3]=columns,
       [4]=3d, [5]=border attr, [6]=hotkey mode */
    printf("  Columns    : %d\n", (int)data[off + 3]);
    printf("  Colors     : brackets=%d  keys=%d  desc=%d\n",
           (int)data[off], (int)data[off+1], (int)data[off+2]);
    printf("  Hotkeys    : %s\n", hotkey_str((int)data[off + 6]));
    off += 15;

    strip_colors((char *)&data[off], 10, buf, sizeof(buf));
    if (buf[0]) printf("  Security   : %s\n", buf);
    off += 10;

    strip_colors((char *)&data[off], 10, buf, sizeof(buf));
    if (buf[0]) printf("  MCI Name   : %s\n", buf);
    off += 10;

    memcpy(&hlp, &data[off], 2);
    off += 2;

    printf("  Commands   : %d\n", ncmds);
    printf("--------------------------------------------------------------------------------\n");
    printf("  ##  Type  Key       Attr          Security  Description\n");
    printf("      Line                                    Parameter\n");
    printf("--------------------------------------------------------------------------------\n");

    for (i = 0; i < ncmds; i++) {
        /* old menurec: desc[50] key[8] type[2] at[1] line[40] ms[80] sl[8] */
        const unsigned char *rec = &data[off];
        char desc_s[512], key_s[64], type_s[8], line_s[512], ms_s[512], sl_s[64];
        unsigned char at;

        strip_colors((char *)rec, 50, desc_s, sizeof(desc_s));
        strip_colors((char *)rec + 50, 8, key_s, sizeof(key_s));

        type_s[0] = rec[58];
        type_s[1] = rec[59];
        type_s[2] = 0;

        at = rec[60];

        strip_colors((char *)rec + 61, 40, line_s, sizeof(line_s));
        strip_colors((char *)rec + 101, 80, ms_s, sizeof(ms_s));
        strip_colors((char *)rec + 181, 8, sl_s, sizeof(sl_s));

        printf("  %2d  %-4s  %-8s  %-12s  %-8s  %s\n",
               i, type_s, key_s, old_attr_str(at), sl_s, desc_s);

        if (line_s[0] || ms_s[0]) {
            printf("      %-18s%-12s%s\n",
                   line_s[0] ? line_s : "",
                   "",
                   ms_s[0] ? ms_s : "");
        }

        off += OLD_MENUREC_SIZE;
    }
    printf("\n");
}

static void dump_new_format(const unsigned char *data, long fsize, const char *path)
{
    mmrec hdr;
    menurec cmd;
    int ncmds, i, off;
    char buf[512];
    const char *a;

    ncmds = (fsize - sizeof(mmrec)) / sizeof(menurec);

    memcpy(&hdr, data, sizeof(mmrec));

    printf("================================================================================\n");
    printf("MENU: %s  (new v3.x format)\n", path);
    printf("================================================================================\n");

    strip_colors(hdr.title1, sizeof(hdr.title1), buf, sizeof(buf));
    printf("  Title 1    : %s\n", buf);
    strip_colors(hdr.title2, sizeof(hdr.title2), buf, sizeof(buf));
    if (buf[0]) printf("  Title 2    : %s\n", buf);
    strip_colors(hdr.prompt, sizeof(hdr.prompt), buf, sizeof(buf));
    printf("  Prompt     : %s\n", buf);
#ifdef PD
    strip_colors(hdr.prompt2, sizeof(hdr.prompt2), buf, sizeof(buf));
    if (buf[0]) printf("  Pldn Prompt: %s\n", buf);
#endif
    if (hdr.altmenu[0])  printf("  ANSI Menu  : %s\n", hdr.altmenu);
    if (hdr.format[0])   printf("  Format File: %s\n", hdr.format);
    if (hdr.slneed[0])   printf("  Security   : %s\n", hdr.slneed);
    if (hdr.helpfile[0]) printf("  Help File  : %s\n", hdr.helpfile);
    if (hdr.pausefile[0])printf("  MCI Name   : %s\n", hdr.pausefile);
    printf("  Columns    : %d\n", (int)hdr.columns);
    printf("  Colors     : brackets=%d  keys=%d  desc=%d\n",
           (int)hdr.col[0], (int)hdr.col[1], (int)hdr.col[2]);
    printf("  Hotkeys    : %s\n", hotkey_str((int)hdr.boarder));
    a = menu_attr_str(hdr.attr);
    if (a[0]) printf("  Flags      : %s\n", a);
    printf("  Commands   : %d\n", ncmds);
    printf("--------------------------------------------------------------------------------\n");
    printf("  ##  Type  Key           Attr          Security      Description\n");
    printf("      Line                              Parameter\n");
    printf("--------------------------------------------------------------------------------\n");

    off = sizeof(mmrec);
    for (i = 0; i < ncmds && i < 64; i++) {
        char desc_s[512], line_s[512], ms_s[512];
        memcpy(&cmd, &data[off], sizeof(menurec));
        off += sizeof(menurec);

        a = new_cmd_attr_str(cmd.attr);
        strip_colors(cmd.desc, sizeof(cmd.desc), desc_s, sizeof(desc_s));
        strip_colors(cmd.line, sizeof(cmd.line), line_s, sizeof(line_s));
        strip_colors(cmd.ms, sizeof(cmd.ms), ms_s, sizeof(ms_s));

        printf("  %2d  %-4.2s  %-12s  %-12s  %-12s  %s\n",
               i, cmd.type, cmd.key, a, cmd.sl, desc_s);

        if (line_s[0] || ms_s[0]) {
            printf("      %-18s%-12s%s\n",
                   line_s[0] ? line_s : "",
                   "",
                   ms_s[0] ? ms_s : "");
        }
    }
    printf("\n");
}

static void dump_menu(const char *path)
{
    FILE *f;
    unsigned char *data;
    long fsize;
    int fmt;

    f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "mnudump: cannot open %s: ", path);
        perror("");
        return;
    }

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize < OLD_MMREC_SIZE) {
        fprintf(stderr, "mnudump: %s too small (%ld bytes)\n", path, fsize);
        fclose(f);
        return;
    }

    data = (unsigned char *)malloc(fsize);
    if (!data) {
        fprintf(stderr, "mnudump: out of memory\n");
        fclose(f);
        return;
    }

    if (fread(data, 1, fsize, f) != fsize) {
        fprintf(stderr, "mnudump: error reading %s\n", path);
        free(data);
        fclose(f);
        return;
    }
    fclose(f);

    fmt = detect_format(fsize);

    if (fmt == 1) {
        dump_old_format(data, fsize, path);
    } else if (fmt == 0) {
        dump_new_format(data, fsize, path);
    } else {
        fprintf(stderr, "mnudump: %s: unrecognized format (size=%ld)\n", path, fsize);
        fprintf(stderr, "  Not old (%ld - %d = %ld, mod %d = %ld)\n",
                fsize, OLD_MMREC_SIZE, fsize - OLD_MMREC_SIZE,
                OLD_MENUREC_SIZE, (fsize - OLD_MMREC_SIZE) % OLD_MENUREC_SIZE);
        fprintf(stderr, "  Not new (%ld - %lu = %ld, mod %lu = %ld)\n",
                fsize, (unsigned long)sizeof(mmrec),
                fsize - (long)sizeof(mmrec),
                (unsigned long)sizeof(menurec),
                (fsize - (long)sizeof(mmrec)) % (long)sizeof(menurec));
    }

    free(data);
}

int main(int argc, char **argv)
{
    int i;

    if (argc < 2) {
        fprintf(stderr,
            "Usage: mnudump <file.mnu> [file2.mnu ...]\n"
            "\n"
            "Dumps Dominion BBS menu files as human-readable ASCII.\n"
            "Auto-detects old (v2.x) and new (v3.x) menu formats.\n"
            "Strips embedded color codes from all text fields.\n"
            "\n"
            "Examples:\n"
            "  mnudump menus/main.mnu\n"
            "  mnudump menus/main.mnu menus/file.mnu menus/email.mnu\n");
        return 1;
    }

    for (i = 1; i < argc; i++) {
        dump_menu(argv[i]);
    }

    return 0;
}
