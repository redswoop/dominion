/*
 * mnu2json.c — Convert Dominion BBS .mnu files (binary) to .json
 *
 * Reads both old (v2.x, 312+189*N) and new (v3.x, 471+217*N) binary
 * menu formats and writes human-readable JSON using the menu_json layer.
 *
 * Usage: ./mnu2json <file.mnu> [file2.mnu ...]
 *        ./mnu2json -o <outdir> <file.mnu> [file2.mnu ...]
 *
 * Build: make build/mnu2json
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "vardec.h"
#include "cJSON.h"

/* We reuse menu_json functions but link against the .c directly */
#include "menudb.h"
#include "menu_json.h"
#include "json_io.h"

/* Old format struct sizes */
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


/*
 * detect_format — Returns 1 for old format, 0 for new format, -1 for unknown.
 */
static int detect_format(long fsize)
{
    long rem;

    /* Try new format: mmrec + N * menurec */
    rem = fsize - (long)sizeof(mmrec);
    if (rem >= 0 && (rem % (long)sizeof(menurec)) == 0)
        return 0;

    /* Try old format: 312 + N * 189 */
    rem = fsize - OLD_HDR_SIZE;
    if (rem >= 0 && (rem % OLD_CMD_SIZE) == 0)
        return 1;

    return -1;
}


/*
 * load_old_format — Parse old v2.x binary into menu_data_t.
 */
static int load_old_format(const unsigned char *data, long fsize,
                           const char *basename, menu_data_t *out)
{
    int ncmds, i;
    const unsigned char *old;
    mmrec *h;

    ncmds = (fsize - OLD_HDR_SIZE) / OLD_CMD_SIZE;
    memset(out, 0, sizeof(menu_data_t));
    snprintf(out->name, sizeof(out->name), "%s.mnu", basename);

    h = &out->header;
    old = data;

    safe_copy(h->prompt,   (char *)&old[OLD_PROMPT_OFF],  OLD_PROMPT_LEN,  sizeof(h->prompt));
    h->helpfile[0] = 0;
    safe_copy(h->title1,   (char *)&old[OLD_TITLE1_OFF],  OLD_TITLE1_LEN,  sizeof(h->title1));
    safe_copy(h->title2,   (char *)&old[OLD_TITLE2_OFF],  OLD_TITLE2_LEN,  sizeof(h->title2));
    safe_copy(h->altmenu,  (char *)&old[OLD_ALTMENU_OFF], OLD_ALTMENU_LEN, sizeof(h->altmenu));
    h->format[0] = 0;
    safe_copy(h->slneed,   (char *)&old[OLD_SLNEED_OFF],  OLD_SLNEED_LEN,  sizeof(h->slneed));
    safe_copy(h->pausefile,(char *)&old[OLD_PAUSE_OFF],   OLD_PAUSE_LEN,   sizeof(h->pausefile));
    h->helplevel = 0;
    h->columns   = old[OLD_FALLBACK_OFF + 3];
    h->col[0]    = old[OLD_FALLBACK_OFF + 0];
    h->col[1]    = old[OLD_FALLBACK_OFF + 1];
    h->col[2]    = old[OLD_FALLBACK_OFF + 2];
    h->boarder   = 0;
    h->battr     = old[OLD_FALLBACK_OFF + 5];
    h->attr      = 0;
    if (old[OLD_FALLBACK_OFF + 4])
        h->attr |= 0x0001;

    for (i = 0; i < ncmds && i < MAX_MENU_COMMANDS; i++) {
        const unsigned char *rec = &data[OLD_HDR_SIZE + i * OLD_CMD_SIZE];
        menurec *cmd = &out->commands[i];

        memset(cmd, 0, sizeof(menurec));
        safe_copy(cmd->desc, (char *)&rec[OLD_DESC_OFF], OLD_DESC_LEN, sizeof(cmd->desc));
        safe_copy(cmd->key,  (char *)&rec[OLD_KEY_OFF],  OLD_KEY_LEN,  sizeof(cmd->key));
        cmd->type[0] = rec[OLD_TYPE_OFF];
        cmd->type[1] = rec[OLD_TYPE_OFF + 1];
        cmd->type[2] = 0;
        safe_copy(cmd->line, (char *)&rec[OLD_LINE_OFF], OLD_LINE_LEN, sizeof(cmd->line));
        safe_copy(cmd->ms,   (char *)&rec[OLD_MS_OFF],   OLD_MS_LEN,   sizeof(cmd->ms));
        safe_copy(cmd->sl,   (char *)&rec[OLD_SL_OFF],   OLD_SL_LEN,   sizeof(cmd->sl));
        cmd->attr = convert_attr(rec[OLD_AT_OFF]);
    }
    out->count = (ncmds < MAX_MENU_COMMANDS) ? ncmds : MAX_MENU_COMMANDS;

    return 0;
}


/*
 * load_new_format — Parse new v3.x binary into menu_data_t.
 */
static int load_new_format(const unsigned char *data, long fsize,
                           const char *basename, menu_data_t *out)
{
    int ncmds, i;

    ncmds = (fsize - sizeof(mmrec)) / sizeof(menurec);
    memset(out, 0, sizeof(menu_data_t));
    snprintf(out->name, sizeof(out->name), "%s.mnu", basename);

    memcpy(&out->header, data, sizeof(mmrec));

    for (i = 0; i < ncmds && i < MAX_MENU_COMMANDS; i++) {
        memcpy(&out->commands[i], &data[sizeof(mmrec) + i * sizeof(menurec)],
               sizeof(menurec));
    }
    out->count = (ncmds < MAX_MENU_COMMANDS) ? ncmds : MAX_MENU_COMMANDS;

    return 0;
}


static int convert_file(const char *inpath, const char *outpath)
{
    FILE *fin;
    unsigned char *data;
    long fsize;
    int fmt;
    menu_data_t menu;

    fin = fopen(inpath, "rb");
    if (!fin) {
        fprintf(stderr, "mnu2json: cannot open %s: ", inpath);
        perror("");
        return 1;
    }

    fseek(fin, 0, SEEK_END);
    fsize = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    if (fsize < OLD_HDR_SIZE) {
        fprintf(stderr, "mnu2json: %s too small (%ld bytes)\n", inpath, fsize);
        fclose(fin);
        return 1;
    }

    data = (unsigned char *)malloc(fsize);
    if (!data) {
        fprintf(stderr, "mnu2json: out of memory\n");
        fclose(fin);
        return 1;
    }

    if (fread(data, 1, fsize, fin) != (size_t)fsize) {
        fprintf(stderr, "mnu2json: error reading %s\n", inpath);
        free(data);
        fclose(fin);
        return 1;
    }
    fclose(fin);

    /* Extract basename for menu id */
    {
        const char *base = strrchr(inpath, '/');
        char basename[20];

        base = base ? base + 1 : inpath;
        strncpy(basename, base, sizeof(basename) - 1);
        basename[sizeof(basename) - 1] = '\0';
        {
            char *dot = strrchr(basename, '.');
            if (dot) *dot = '\0';
        }

        fmt = detect_format(fsize);
        if (fmt == 1) {
            load_old_format(data, fsize, basename, &menu);
        } else if (fmt == 0) {
            load_new_format(data, fsize, basename, &menu);
        } else {
            fprintf(stderr, "mnu2json: %s: unrecognized format (size=%ld)\n", inpath, fsize);
            free(data);
            return 1;
        }
    }

    free(data);

    if (menu_json_save(outpath, &menu) != 0) {
        fprintf(stderr, "mnu2json: failed to write %s\n", outpath);
        return 1;
    }

    printf("  %s -> %s (%d commands)\n", inpath, outpath, menu.count);
    return 0;
}


int main(int argc, char **argv)
{
    int i, errors = 0;
    char *outdir = NULL;

    if (argc < 2) {
        fprintf(stderr,
            "Usage: mnu2json <file.mnu> [file2.mnu ...]\n"
            "       mnu2json -o <outdir> <file.mnu> [file2.mnu ...]\n"
            "\n"
            "Converts Dominion BBS binary .mnu files to JSON format.\n"
            "Auto-detects old (v2.x) and new (v3.x) binary formats.\n"
            "\n"
            "Without -o, writes .json next to the .mnu file.\n"
            "With -o, writes .json files to the output directory.\n");
        return 1;
    }

    i = 1;
    if (argc > 2 && strcmp(argv[1], "-o") == 0) {
        outdir = argv[2];
        i = 3;
    }

    for (; i < argc; i++) {
        char outpath[1024];
        const char *base;
        char basename[20];

        base = strrchr(argv[i], '/');
        base = base ? base + 1 : argv[i];
        strncpy(basename, base, sizeof(basename) - 1);
        basename[sizeof(basename) - 1] = '\0';
        {
            char *dot = strrchr(basename, '.');
            if (dot) *dot = '\0';
        }

        if (outdir) {
            snprintf(outpath, sizeof(outpath), "%s/%s.json", outdir, basename);
        } else {
            /* Write .json next to .mnu — figure out directory */
            const char *slash = strrchr(argv[i], '/');
            if (slash) {
                int dirlen = slash - argv[i] + 1;
                snprintf(outpath, sizeof(outpath), "%.*s%s.json", dirlen, argv[i], basename);
            } else {
                snprintf(outpath, sizeof(outpath), "%s.json", basename);
            }
        }

        errors += convert_file(argv[i], outpath);
    }

    return errors ? 1 : 0;
}
