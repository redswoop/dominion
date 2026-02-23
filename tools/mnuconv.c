/*
 * mnuconv.c - Convert Dominion BBS menu files from old v2.x to new v3.x format
 *
 * The dist/ menu files from the v3.0 snapshot use the old binary layout
 * (312-byte header + 189-byte commands).  The BBS code expects the new
 * layout (471-byte header + 217-byte commands, with #pragma pack(1)).
 *
 * This tool reads old-format .mnu files and writes new-format ones.
 * Converts in-place by default; use -o to write to a different file.
 *
 * Usage: ./mnuconv <file.mnu> [file2.mnu ...]
 *        ./mnuconv -o outdir/ menus/main.mnu menus/file.mnu ...
 *
 * Build: cc -std=gnu89 -DPD -fsigned-char -Isrc -o mnuconv tools/mnuconv.c
 *
 * Field mapping (old -> new):
 *   Header:
 *     prompt[100]   -> prompt[101]
 *     (none)        -> prompt2[91] = ""  (PD build)
 *     (none)        -> helpfile[10] = ""
 *     title1[80]    -> title1[101]
 *     title2[80]    -> title2[101]
 *     altmenu[12]   -> altmenu[9]
 *     (none)        -> format[9] = ""
 *     slneed[10]    -> slneed[31]
 *     pausefile[10] -> pausefile[9]
 *     (none)        -> helplevel = 0
 *     fallback[3]   -> columns = fallback[3]
 *     fallback[0-2] -> col[3] = {fallback[0], fallback[1], fallback[2]}
 *     (none)        -> boarder = 0  (old fallback[6] was border char, not input mode)
 *     fallback[5]   -> battr = fallback[5]
 *     fallback[4]   -> attr bit 0 (menu_3d/menu_extprompt)
 *
 *   Commands:
 *     desc[50]      -> desc[50]
 *     key[8]        -> key[21]
 *     type[2]       -> type[3]
 *     line[40]      -> line[40]
 *     ms[80]        -> ms[80]
 *     sl[8]         -> sl[21]
 *     at (char)     -> attr (INT16 bitmask):
 *       'H' -> 0x0001 (hidden)
 *       'U' -> 0x0002 (unhidden)
 *       'T' -> 0x0004 (title)
 *       'P' -> 0x0008 (pulldown-sep)
 *       'F' -> 0x0010 (forced)
 *       'D' -> 0x0040 (default)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "vardec.h"

/* Old format sizes */
#define OLD_HDR_SIZE  312
#define OLD_CMD_SIZE  189

/* Old format field offsets within header */
#define OLD_PROMPT_OFF    0
#define OLD_PROMPT_LEN    100
#define OLD_TITLE1_OFF    100
#define OLD_TITLE1_LEN    80
#define OLD_TITLE2_OFF    180
#define OLD_TITLE2_LEN    80
#define OLD_ALTMENU_OFF   260
#define OLD_ALTMENU_LEN   12
#define OLD_RES_OFF       272
#define OLD_RES_LEN       3
#define OLD_FALLBACK_OFF  275
#define OLD_FALLBACK_LEN  15
#define OLD_SLNEED_OFF    290
#define OLD_SLNEED_LEN    10
#define OLD_PAUSE_OFF     300
#define OLD_PAUSE_LEN     10
#define OLD_HELP_OFF      310

/* Old format field offsets within command */
#define OLD_DESC_OFF      0
#define OLD_DESC_LEN      50
#define OLD_KEY_OFF       50
#define OLD_KEY_LEN       8
#define OLD_TYPE_OFF      58
#define OLD_TYPE_LEN      2
#define OLD_AT_OFF        60
#define OLD_LINE_OFF      61
#define OLD_LINE_LEN      40
#define OLD_MS_OFF        101
#define OLD_MS_LEN        80
#define OLD_SL_OFF        181
#define OLD_SL_LEN        8

static void safe_copy(char *dst, const char *src, int srcmax, int dstmax)
{
    int len = 0;
    while (len < srcmax && len < dstmax - 1 && src[len])
        len++;
    memcpy(dst, src, len);
    dst[len] = 0;
}

static INT16 convert_attr(unsigned char at)
{
    switch (at) {
    case 'H': return 0x0001;
    case 'U': return 0x0002;
    case 'T': return 0x0004;
    case 'P': return 0x0008;
    case 'F': return 0x0010;
    case 'D': return 0x0040;
    }
    return 0;
}

static int convert_file(const char *inpath, const char *outpath)
{
    FILE *fin, *fout;
    unsigned char *data;
    long fsize;
    int ncmds, i;
    const unsigned char *old;
    mmrec hdr;
    menurec cmd;

    fin = fopen(inpath, "rb");
    if (!fin) {
        fprintf(stderr, "mnuconv: cannot open %s: ", inpath);
        perror("");
        return 1;
    }

    fseek(fin, 0, SEEK_END);
    fsize = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    /* Check if already new format */
    if (fsize >= (long)sizeof(mmrec) &&
        (fsize - (long)sizeof(mmrec)) % (long)sizeof(menurec) == 0) {
        fprintf(stderr, "mnuconv: %s already in new format (%ld bytes), skipping\n",
                inpath, fsize);
        fclose(fin);
        return 0;
    }

    /* Verify old format */
    if (fsize < OLD_HDR_SIZE ||
        (fsize - OLD_HDR_SIZE) % OLD_CMD_SIZE != 0) {
        fprintf(stderr, "mnuconv: %s: unrecognized format (size=%ld)\n", inpath, fsize);
        fclose(fin);
        return 1;
    }

    ncmds = (fsize - OLD_HDR_SIZE) / OLD_CMD_SIZE;

    data = (unsigned char *)malloc(fsize);
    if (!data) {
        fprintf(stderr, "mnuconv: out of memory\n");
        fclose(fin);
        return 1;
    }

    if (fread(data, 1, fsize, fin) != (size_t)fsize) {
        fprintf(stderr, "mnuconv: error reading %s\n", inpath);
        free(data);
        fclose(fin);
        return 1;
    }
    fclose(fin);

    /* Convert header */
    memset(&hdr, 0, sizeof(hdr));
    old = data;

    safe_copy(hdr.prompt,   (char *)&old[OLD_PROMPT_OFF],  OLD_PROMPT_LEN,  sizeof(hdr.prompt));
    hdr.helpfile[0] = 0;
    safe_copy(hdr.title1,   (char *)&old[OLD_TITLE1_OFF],  OLD_TITLE1_LEN,  sizeof(hdr.title1));
    safe_copy(hdr.title2,   (char *)&old[OLD_TITLE2_OFF],  OLD_TITLE2_LEN,  sizeof(hdr.title2));
    safe_copy(hdr.altmenu,  (char *)&old[OLD_ALTMENU_OFF], OLD_ALTMENU_LEN, sizeof(hdr.altmenu));
    hdr.format[0] = 0;
    safe_copy(hdr.slneed,   (char *)&old[OLD_SLNEED_OFF],  OLD_SLNEED_LEN,  sizeof(hdr.slneed));
    safe_copy(hdr.pausefile,(char *)&old[OLD_PAUSE_OFF],   OLD_PAUSE_LEN,   sizeof(hdr.pausefile));
    hdr.helplevel = 0;
    hdr.columns   = old[OLD_FALLBACK_OFF + 3];
    hdr.col[0]    = old[OLD_FALLBACK_OFF + 0];  /* brackets */
    hdr.col[1]    = old[OLD_FALLBACK_OFF + 1];  /* keys */
    hdr.col[2]    = old[OLD_FALLBACK_OFF + 2];  /* desc */
    hdr.boarder   = 0;  /* hotkey mode: 0=normal (old format had no equivalent) */
    hdr.battr     = old[OLD_FALLBACK_OFF + 5];  /* border attr */
    hdr.attr      = 0;
    if (old[OLD_FALLBACK_OFF + 4])
        hdr.attr |= 0x0001;  /* menu_extprompt (was menu_3d) */

    /* Write output */
    fout = fopen(outpath, "wb");
    if (!fout) {
        fprintf(stderr, "mnuconv: cannot write %s: ", outpath);
        perror("");
        free(data);
        return 1;
    }

    fwrite(&hdr, sizeof(hdr), 1, fout);

    /* Convert and write each command */
    for (i = 0; i < ncmds; i++) {
        const unsigned char *rec = &data[OLD_HDR_SIZE + i * OLD_CMD_SIZE];

        memset(&cmd, 0, sizeof(cmd));
        safe_copy(cmd.desc, (char *)&rec[OLD_DESC_OFF], OLD_DESC_LEN, sizeof(cmd.desc));
        safe_copy(cmd.key,  (char *)&rec[OLD_KEY_OFF],  OLD_KEY_LEN,  sizeof(cmd.key));

        cmd.type[0] = rec[OLD_TYPE_OFF];
        cmd.type[1] = rec[OLD_TYPE_OFF + 1];
        cmd.type[2] = 0;

        safe_copy(cmd.line, (char *)&rec[OLD_LINE_OFF], OLD_LINE_LEN, sizeof(cmd.line));
        safe_copy(cmd.ms,   (char *)&rec[OLD_MS_OFF],   OLD_MS_LEN,   sizeof(cmd.ms));
        safe_copy(cmd.sl,   (char *)&rec[OLD_SL_OFF],   OLD_SL_LEN,   sizeof(cmd.sl));
        cmd.attr = convert_attr(rec[OLD_AT_OFF]);

        fwrite(&cmd, sizeof(cmd), 1, fout);
    }

    fclose(fout);
    free(data);

    printf("  %s: %d commands converted (%ld -> %ld bytes)\n",
           inpath, ncmds, fsize,
           (long)sizeof(mmrec) + (long)ncmds * (long)sizeof(menurec));
    return 0;
}

int main(int argc, char **argv)
{
    int i, errors = 0;
    char *outdir = NULL;

    if (argc < 2) {
        fprintf(stderr,
            "Usage: mnuconv <file.mnu> [file2.mnu ...]\n"
            "       mnuconv -o <outdir> <file.mnu> [file2.mnu ...]\n"
            "\n"
            "Converts Dominion BBS menu files from old v2.x format\n"
            "(312+189*N bytes) to new v3.x format (471+217*N bytes).\n"
            "\n"
            "Without -o, converts files in place.\n"
            "With -o, writes converted files to the output directory.\n"
            "\n"
            "Already-converted files are skipped.\n");
        return 1;
    }

    i = 1;
    if (argc > 2 && strcmp(argv[1], "-o") == 0) {
        outdir = argv[2];
        i = 3;
    }

    for (; i < argc; i++) {
        char outpath[1024];

        if (outdir) {
            /* Extract just the filename from the path */
            const char *base = strrchr(argv[i], '/');
            base = base ? base + 1 : argv[i];
            snprintf(outpath, sizeof(outpath), "%s/%s", outdir, base);
        } else {
            /* Convert in place */
            snprintf(outpath, sizeof(outpath), "%s", argv[i]);
        }

        errors += convert_file(argv[i], outpath);
    }

    return errors ? 1 : 0;
}
