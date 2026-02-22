/*
 * sgrtrace.c — Trace the BBS SGR state machine
 *
 * Replicates the exact SGR logic from sp_execute_ansi() in
 * stream_processor.c.  Reads input (stdin or file), processes
 * ANSI escape sequences, and traces every curatr state change.
 *
 * Usage:
 *   sgrtrace < file.ans          # trace an ANSI art file
 *   echo -e '\e[1;30mHi' | sgrtrace   # trace a simple sequence
 *   sgrtrace file1.ans file2.ans       # trace multiple files
 *
 * Build:
 *   cc -o build/sgrtrace tools/sgrtrace.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char curatr = 0x07;  /* same default as BBS */
static char *clrlst = "04261537";    /* SGR→DOS color index map */

/* Exact replicas of BBS functions */
static void setfgc(int i) { curatr = (curatr & 0xf8) | i; }
static void setbgc(int i) { curatr = (curatr & 0x8f) | (i << 4); }

/* Decode a DOS attribute byte to readable string */
static const char *attr_str(unsigned char a)
{
    static char buf[64];
    static const char *cnames[] = {
        "BLACK", "BLUE", "GREEN", "CYAN",
        "RED", "MAGENTA", "BROWN", "LTGREY"
    };
    static const char *bright_cnames[] = {
        "DKGREY", "LTBLUE", "LTGREEN", "LTCYAN",
        "LTRED", "LTMAGENTA", "YELLOW", "WHITE"
    };
    int fg = a & 0x07;
    int bold = (a & 0x08) ? 1 : 0;
    int bg = (a >> 4) & 0x07;
    int blink = (a & 0x80) ? 1 : 0;
    const char *fgname = bold ? bright_cnames[fg] : cnames[fg];

    sprintf(buf, "0x%02X  fg=%-10s bg=%-7s%s",
            a, fgname, cnames[bg], blink ? " BLINK" : "");
    return buf;
}

/* Check if attribute is invisible */
static int is_invisible(unsigned char a)
{
    int fg = a & 0x0F;  /* fg color including intensity */
    int bg = (a >> 4) & 0x07;  /* bg color (no blink) */

    /* Map to effective visual color:
     * fg 0x00 = black, 0x08 = dark grey (visible on black bg)
     * bg 0x00 = black
     * Invisible when fg == 0 and bg == 0 (black on black, no bold) */
    if (fg == 0 && bg == 0) return 1;
    return 0;
}

static void process_sgr(int args[], int argptr, int filepos)
{
    int count, ptr;
    unsigned char before = curatr;

    if (!argptr) {
        argptr = 1;
        args[0] = 0;
    }

    /* Print the ESC sequence */
    printf("  SGR[");
    for (count = 0; count < argptr; count++) {
        if (count) printf(";");
        printf("%d", args[count]);
    }
    printf("]  (file offset %d)\n", filepos);

    /* Process each arg with per-step tracing */
    for (count = 0; count < argptr; count++) {
        unsigned char step_before = curatr;
        switch (args[count]) {
        case 0:
            curatr = 0x07;
            break;
        case 1:
            curatr = curatr | 0x08;
            break;
        case 4:
            break;
        case 5:
            curatr = curatr | 0x80;
            break;
        case 7:
            ptr = curatr & 0x77;
            curatr = (curatr & 0x88) | (ptr << 4) | (ptr >> 4);
            break;
        case 8:
            curatr = 0;
            break;
        default:
            if ((args[count] >= 30) && (args[count] <= 37))
                setfgc(clrlst[args[count] - 30] - '0');
            else if ((args[count] >= 40) && (args[count] <= 47))
                setbgc(clrlst[args[count] - 40] - '0');
            break;
        }
        if (curatr != step_before) {
            printf("    arg %d: 0x%02X -> 0x%02X", args[count], step_before, curatr);
            if (is_invisible(curatr))
                printf("  *** INVISIBLE ***");
            printf("\n");
        } else {
            printf("    arg %d: 0x%02X (unchanged)\n", args[count], curatr);
        }
    }

    printf("    result: %s", attr_str(curatr));
    if (is_invisible(curatr))
        printf("  <<<< BLACK ON BLACK >>>>");
    printf("\n");
}

static void trace_stream(FILE *fp, const char *name)
{
    int c;
    char ansistr[80];
    int ansiptr = 0;
    int filepos = 0;
    int vis_count = 0;     /* visible chars since last SGR */
    int invis_count = 0;   /* invisible chars total */
    int invis_start = -1;  /* file offset where invisible run started */

    printf("=== Tracing: %s  (initial curatr = %s) ===\n\n", name, attr_str(curatr));

    while ((c = fgetc(fp)) != EOF) {
        filepos++;

        if (ansiptr) {
            ansistr[ansiptr++] = (char)c;
            ansistr[ansiptr] = 0;

            /* Check for terminating character (same logic as BBS) */
            if (((c < '0' || c > '9') && c != '[' && c != ';') ||
                ansistr[1] != '[' || ansiptr > 75) {

                /* Only trace SGR 'm' commands */
                if (c == 'm' && ansistr[1] == '[') {
                    int args[11], argcnt = 0, tempptr = 0, ptr = 2;
                    char temp[11];
                    memset(args, 0, sizeof(args));
                    memset(temp, 0, sizeof(temp));
                    ansistr[ansiptr - 1] = 0;  /* remove 'm' */
                    while (ansistr[ptr] && argcnt < 10 && tempptr < 10) {
                        if (ansistr[ptr] == ';') {
                            temp[tempptr] = 0;
                            tempptr = 0;
                            args[argcnt++] = atoi(temp);
                            memset(temp, 0, sizeof(temp));
                        } else {
                            temp[tempptr++] = ansistr[ptr];
                        }
                        ptr++;
                    }
                    if (tempptr && argcnt < 10) {
                        temp[tempptr] = 0;
                        args[argcnt++] = atoi(temp);
                    }
                    /* Print visible char count since last SGR */
                    if (vis_count > 0) {
                        printf("  [%d visible chars]\n", vis_count);
                        vis_count = 0;
                    }
                    process_sgr(args, argcnt, filepos - ansiptr);
                } else {
                    /* Non-SGR escape sequence — just note it */
                    if (vis_count > 0) {
                        printf("  [%d visible chars]\n", vis_count);
                        vis_count = 0;
                    }
                    ansistr[ansiptr] = 0;
                    printf("  CSI '%c' (non-SGR, skipped)\n", c);
                }
                ansiptr = 0;
            }
            continue;
        }

        if (c == 27) {
            ansistr[0] = 27;
            ansiptr = 1;
            ansistr[ansiptr] = 0;
            continue;
        }

        /* Normal character */
        if (c >= 32 || c >= 128) {
            /* Printable (including CP437 high bytes) */
            if (is_invisible(curatr)) {
                if (invis_start < 0) invis_start = filepos;
                invis_count++;
                /* Print first few invisible chars explicitly */
                if (invis_count <= 3 || (invis_count % 20 == 0)) {
                    if (vis_count > 0) {
                        printf("  [%d visible chars]\n", vis_count);
                        vis_count = 0;
                    }
                    printf("  INVISIBLE char 0x%02X '%c' at offset %d  (attr %s)\n",
                           c, (c >= 32 && c < 127) ? c : '.', filepos, attr_str(curatr));
                }
            } else {
                if (invis_count > 3) {
                    printf("  ... (%d invisible chars total, offsets %d-%d)\n",
                           invis_count, invis_start, filepos - 1);
                }
                if (invis_count > 0) {
                    invis_count = 0;
                    invis_start = -1;
                }
                vis_count++;
            }
        }
        /* Control chars (CR, LF, etc.) — skip silently */
    }

    if (vis_count > 0)
        printf("  [%d visible chars]\n", vis_count);
    if (invis_count > 0)
        printf("  ... (%d invisible chars total, offsets %d-%d)\n",
               invis_count, invis_start, filepos);

    printf("\n=== End trace: %s  (final curatr = %s) ===\n\n", name, attr_str(curatr));
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        /* Read from stdin */
        trace_stream(stdin, "<stdin>");
    } else {
        int i;
        for (i = 1; i < argc; i++) {
            FILE *fp = fopen(argv[i], "rb");
            if (!fp) {
                fprintf(stderr, "sgrtrace: cannot open %s: ", argv[i]);
                perror("");
                continue;
            }
            curatr = 0x07;  /* reset for each file */
            trace_stream(fp, argv[i]);
            fclose(fp);
        }
    }
    return 0;
}
