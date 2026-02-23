/*
 * platform_stubs.c — macOS implementations of DOS/Borland functions
 *
 * These are real implementations where possible, stubs where not yet needed.
 */

#include "io_ncurses.h"  /* MUST come before platform.h — see io_ncurses.h */
#include "platform.h"
#include "session.h"
#include "cp437.h"
#include "terminal_bridge.h"

extern void reset_attr_cache(void);
extern void conio_sync_cursor(int x, int y);

/* ================================================================== */
/*  int86() — BIOS/DOS interrupt stub                                  */
/* ================================================================== */

int int86(int intno, union REGS *inregs, union REGS *outregs)
{
    /* Stub — DOS interrupts don't exist on macOS.
     * Individual callers (fossil, console, etc.) will be replaced
     * with native implementations over time.
     */
    if (outregs) {
        memset(outregs, 0, sizeof(union REGS));
    }
    return 0;
}

/* ================================================================== */
/*  CONSOLE FUNCTIONS — ncurses implementations                        */
/* ================================================================== */

static int _cur_color = 7;

void textcolor(int color)
{
    _cur_color = (_cur_color & 0xf0) | (color & 0x0f);
    if (nc_active) attrset(term_nc_attr(_cur_color));
}

void textattr(int attr)
{
    _cur_color = attr;
    if (nc_active) attrset(term_nc_attr(attr));
}

void clrscr(void)
{
    if (nc_active) {
        attrset(term_nc_attr(0x07));
        erase();
        move(0, 0);
        refresh();
    }
    /* Clear the scrn[] shadow buffer so conio.c stays in sync */
    if (scrn) memset(scrn, 0, 4000);
    /* Reset conio.c cursor tracking to top-left */
    conio_sync_cursor(0, 0);
    reset_attr_cache();
}

void gotoxy(int x, int y)
{
    /* Borland gotoxy is 1-based; ncurses move is 0-based */
    if (nc_active) {
        move(y - 1, x - 1);
        refresh();
    }
}

/* wherex/wherey are defined in conio.c */

void _setcursortype(int type)
{
    if (!nc_active) return;
    switch (type) {
    case _NOCURSOR:
        curs_set(0);
        break;
    default:
        curs_set(1);
        break;
    }
}

int kbhit(void)
{
    int ch;
    if (!nc_active) return 0;
    nodelay(stdscr, TRUE);
    ch = wgetch(stdscr);
    if (ch != ERR) {
        ungetch(ch);
        return 1;
    }
    return 0;
}

/* getch() — ncurses macro was #undef'd in io_ncurses.h, so provide a real function.
 * Must be extern "C" to match ncurses declaration and platform.h. */
extern "C" int getch(void)
{
    if (!nc_active) return 0;
    nodelay(stdscr, FALSE);
    return wgetch(stdscr);
}

int getche(void)
{
    int ch;
    if (!nc_active) return 0;
    nodelay(stdscr, FALSE);
    ch = wgetch(stdscr);
    if (ch != ERR && ch < 256) {
        term_put_cp437((unsigned char)ch);
        refresh();
    }
    return ch;
}

void cprintf(const char *fmt, ...)
{
    char buf[512];
    int i;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (!nc_active) return;
    /* Process each byte: convert CP437 high chars to UTF-8,
     * handle CR/LF/BS for ncurses compatibility */
    for (i = 0; buf[i]; i++) {
        unsigned char ch = (unsigned char)buf[i];
        if (ch >= 0x80) {
            addstr(cp437_to_utf8[ch]);
        } else if (ch == '\r') {
            int y, x;
            getyx(stdscr, y, x);
            move(y, 0);
        } else if (ch == '\n') {
            int y, x;
            getyx(stdscr, y, x);
            if (y < LINES - 1)
                move(y + 1, x);
        } else if (ch == '\b') {
            int y, x;
            getyx(stdscr, y, x);
            if (x > 0) move(y, x - 1);
        } else {
            addch(ch);
        }
    }
    refresh();
}

void cputs(const char *s)
{
    int i;
    if (!nc_active) return;
    for (i = 0; s[i]; i++) {
        unsigned char ch = (unsigned char)s[i];
        if (ch >= 0x80)
            addstr(cp437_to_utf8[ch]);
        else
            addch(ch);
    }
    refresh();
}

/* ================================================================== */
/*  DATE/TIME FUNCTIONS                                                */
/* ================================================================== */

/* Note: these are called via macros getdate→dos_getdate, gettime→dos_gettime
 * defined in platform.h to avoid conflict with POSIX getdate() */

void dos_getdate(struct date *d)
{
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    d->da_year = lt->tm_year + 1900;
    d->da_mon = lt->tm_mon + 1;
    d->da_day = lt->tm_mday;
}

void dos_gettime(struct time *t)
{
    struct timeval tv;
    struct tm *lt;
    gettimeofday(&tv, NULL);
    lt = localtime(&tv.tv_sec);
    t->ti_hour = lt->tm_hour;
    t->ti_min = lt->tm_min;
    t->ti_sec = lt->tm_sec;
    t->ti_hund = tv.tv_usec / 10000;
}

/* ================================================================== */
/*  DISK FREE SPACE                                                    */
/* ================================================================== */

void getdfree(int drive, struct dfree *df)
{
    /* Return something reasonable */
    df->df_avail = 1000000;
    df->df_total = 2000000;
    df->df_bsec = 512;
    df->df_sclus = 8;
}

/* ================================================================== */
/*  FILE FUNCTIONS                                                     */
/* ================================================================== */

long filelength(int fd)
{
    struct stat st;
    if (fstat(fd, &st) == -1)
        return -1;
    return (long)st.st_size;
}

/* ================================================================== */
/*  DIRECTORY FUNCTIONS                                                */
/* ================================================================== */

/*
 * findfirst/findnext — simplified implementation using opendir/readdir
 * Supports simple wildcard patterns (*.ext, name.*, etc.)
 */

static int _match_pattern(const char *name, const char *pattern)
{
    /* Simple wildcard matching: * matches any sequence */
    while (*pattern) {
        if (*pattern == '*') {
            pattern++;
            if (!*pattern) return 1;
            while (*name) {
                if (_match_pattern(name, pattern)) return 1;
                name++;
            }
            return 0;
        } else if (*pattern == '?' || toupper(*pattern) == toupper(*name)) {
            pattern++;
            name++;
        } else {
            return 0;
        }
    }
    return *name == 0;
}

int findfirst(const char *pathname, struct ffblk *ff, int attrib)
{
    char dirpath[512];
    const char *pattern;
    const char *slash;
    const char *base;

    /* DOS NUL device: "path/nul" or "path\nul" means "does directory exist?" */
    base = strrchr(pathname, '/');
    if (!base) base = strrchr(pathname, '\\');
    if (base) base++; else base = pathname;
    if (strcasecmp(base, "nul") == 0) {
        struct stat st;
        int len = (int)(base - pathname);
        if (len > 0) {
            strncpy(dirpath, pathname, len - 1);
            dirpath[len - 1] = 0;
        } else {
            strcpy(dirpath, ".");
        }
        if (stat(dirpath, &st) == 0 && S_ISDIR(st.st_mode)) {
            strcpy(ff->ff_name, "nul");
            ff->ff_fsize = 0;
            ff->ff_attrib = 0;
            ff->_dir = NULL;
            return 0;  /* success — directory exists */
        }
        ff->_dir = NULL;
        return -1;  /* directory doesn't exist */
    }

    /* Split pathname into directory + pattern */
    slash = strrchr(pathname, '/');
    if (!slash) slash = strrchr(pathname, '\\');

    if (slash) {
        int len = (int)(slash - pathname);
        strncpy(dirpath, pathname, len);
        dirpath[len] = 0;
        pattern = slash + 1;
    } else {
        strcpy(dirpath, ".");
        pattern = pathname;
    }

    strncpy(ff->_pattern, pattern, sizeof(ff->_pattern) - 1);
    ff->_pattern[sizeof(ff->_pattern) - 1] = 0;
    strncpy(ff->_path, dirpath, sizeof(ff->_path) - 1);
    ff->_path[sizeof(ff->_path) - 1] = 0;

    ff->_dir = opendir(dirpath);
    if (!ff->_dir) return -1;

    return findnext(ff);
}

int findnext(struct ffblk *ff)
{
    struct dirent *de;
    struct stat st;
    char fullpath[512];

    if (!ff->_dir) return -1;

    while ((de = readdir(ff->_dir)) != NULL) {
        if (de->d_name[0] == '.') continue;
        if (_match_pattern(de->d_name, ff->_pattern)) {
            strncpy(ff->ff_name, de->d_name, sizeof(ff->ff_name) - 1);
            ff->ff_name[sizeof(ff->ff_name) - 1] = 0;

            snprintf(fullpath, sizeof(fullpath), "%s/%s", ff->_path, de->d_name);
            if (stat(fullpath, &st) == 0) {
                ff->ff_fsize = st.st_size;
                ff->ff_attrib = S_ISDIR(st.st_mode) ? FA_DIREC : 0;
            } else {
                ff->ff_fsize = 0;
                ff->ff_attrib = 0;
            }
            return 0;
        }
    }

    closedir(ff->_dir);
    ff->_dir = NULL;
    return -1;
}

int getcurdir(int drive, char *buf)
{
    (void)drive;
    if (getcwd(buf, 256))
        return 0;
    return -1;
}

int getdisk(void)
{
    return 0; /* No drive letters on macOS; return "C:" equivalent */
}

int setdisk(int drive)
{
    (void)drive;
    return 1;
}

char *searchpath(const char *file)
{
    static char result[512];
    /* Just check current directory */
    if (access(file, F_OK) == 0) {
        strncpy(result, file, sizeof(result) - 1);
        return result;
    }
    return NULL;
}

/* ================================================================== */
/*  STRING FUNCTIONS                                                   */
/* ================================================================== */

char *itoa(int value, char *str, int radix)
{
    if (radix == 10) {
        sprintf(str, "%d", value);
    } else if (radix == 16) {
        sprintf(str, "%x", value);
    } else if (radix == 8) {
        sprintf(str, "%o", value);
    } else {
        sprintf(str, "%d", value);
    }
    return str;
}

char *ltoa(long value, char *str, int radix)
{
    if (radix == 10) {
        sprintf(str, "%ld", value);
    } else if (radix == 16) {
        sprintf(str, "%lx", value);
    } else {
        sprintf(str, "%ld", value);
    }
    return str;
}

char *ultoa(unsigned long value, char *str, int radix)
{
    if (radix == 10) {
        sprintf(str, "%lu", value);
    } else if (radix == 16) {
        sprintf(str, "%lx", value);
    } else {
        sprintf(str, "%lu", value);
    }
    return str;
}

char *strupr(char *s)
{
    char *p = s;
    while (*p) {
        *p = toupper((unsigned char)*p);
        p++;
    }
    return s;
}

char *strlwr(char *s)
{
    char *p = s;
    while (*p) {
        *p = tolower((unsigned char)*p);
        p++;
    }
    return s;
}

/* ================================================================== */
/*  PROCESS FUNCTIONS                                                  */
/* ================================================================== */

int spawnvpe(int mode, const char *path, const char *const argv[], const char *const envp[])
{
    (void)mode;
    (void)envp;
    /* Stub — external program execution */
    fprintf(stderr, "[STUB] spawnvpe: %s\n", path);
    return -1;
}

/* ================================================================== */
/*  BORLAND FILE ATTRIBUTE FUNCTIONS                                   */
/* ================================================================== */

/* _chmod — Borland's file attribute function (not POSIX chmod) */
/* mode 0 = get attributes, mode 1 = set attributes */
int _chmod(const char *path, int func, ...)
{
    (void)path;
    (void)func;
    return 0;  /* Return 0 = no special attributes */
}

/* _getdrive — returns current drive (1=A, 2=B, 3=C, etc.) */
int _getdrive(void)
{
    return 3;  /* C: drive equivalent */
}

/* chsize — truncate a file to a given size (Borland io.h) */
int chsize(int fd, long size)
{
    return ftruncate(fd, (off_t)size);
}

/* ================================================================== */
/*  DOS DATE/TIME CONVERSION                                           */
/* ================================================================== */

/* dostounix — convert Borland date/time structs to Unix timestamp */
long dostounix(struct date *d, struct time *t)
{
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = d->da_year - 1900;
    tm.tm_mon  = d->da_mon - 1;
    tm.tm_mday = d->da_day;
    tm.tm_hour = t->ti_hour;
    tm.tm_min  = t->ti_min;
    tm.tm_sec  = t->ti_sec;
    tm.tm_isdst = -1;
    return (long)mktime(&tm);
}

/* unixtodos — convert Unix timestamp to Borland date/time structs */
void unixtodos(long unixtime, struct date *d, struct time *t)
{
    time_t tt = (time_t)unixtime;
    struct tm *lt = localtime(&tt);
    d->da_year = lt->tm_year + 1900;
    d->da_mon  = lt->tm_mon + 1;
    d->da_day  = lt->tm_mday;
    t->ti_hour = lt->tm_hour;
    t->ti_min  = lt->tm_min;
    t->ti_sec  = lt->tm_sec;
    t->ti_hund = 0;
}

/* ================================================================== */
/*  FILENAME SPLITTING (Borland dir.h)                                 */
/* ================================================================== */

/* fnsplit — split a path into drive, directory, filename, extension */
/* Returns flags indicating which components were found */
#define WILDCARDS  0x01
#define EXTENSION  0x02
#define FILENAME   0x04
#define DIRECTORY  0x08
#define DRIVE      0x10

int fnsplit(const char *path, char *drive, char *dir, char *name, char *ext)
{
    int flags = 0;
    const char *p, *slash, *dot;

    if (drive) drive[0] = 0;
    if (dir)   dir[0] = 0;
    if (name)  name[0] = 0;
    if (ext)   ext[0] = 0;

    if (!path || !path[0]) return 0;

    /* No drive letters on POSIX */
    p = path;

    /* Find last slash */
    slash = strrchr(p, '/');
    if (!slash) slash = strrchr(p, '\\');

    if (slash) {
        if (dir) {
            int len = (int)(slash - p) + 1;
            strncpy(dir, p, len);
            dir[len] = 0;
        }
        flags |= DIRECTORY;
        p = slash + 1;
    }

    /* Find extension */
    dot = strrchr(p, '.');
    if (dot) {
        if (ext) strcpy(ext, dot);
        if (name) {
            int len = (int)(dot - p);
            strncpy(name, p, len);
            name[len] = 0;
        }
        flags |= EXTENSION | FILENAME;
    } else {
        if (name) strcpy(name, p);
        if (p[0]) flags |= FILENAME;
    }

    /* Check for wildcards */
    if (strchr(path, '*') || strchr(path, '?'))
        flags |= WILDCARDS;

    return flags;
}

/* ================================================================== */
/*  SWAP — DOS memory swap for external execution                      */
/* ================================================================== */

int swap(unsigned char *program_name, unsigned char *command_line,
         unsigned char *exec_return, unsigned char *swap_fname)
{
    (void)program_name;
    (void)command_line;
    (void)swap_fname;
    /* Can't swap on modern OS — just report error */
    if (exec_return) *exec_return = 0x08;  /* NO_MEMORY */
    fprintf(stderr, "[STUB] swap: %s %s\n", program_name, command_line);
    return 2;  /* SWAP_NO_SAVE */
}

/* ================================================================== */
/*  STANDALONE TOOL FUNCTIONS USED BY BBS MODULES                      */
/* ================================================================== */

/* pr — ANSI-aware formatted output (from std.c standalone tool).
 * Used by config.c, mc.c, nfo.c which are compiled into the main BBS. */
void pr(char *fmt, ...)
{
    va_list ap;
    char s[512];
    va_start(ap, fmt);
    vsprintf(s, fmt, ap);
    va_end(ap);
    /* In the real std.c this goes through outstr() with color parsing.
     * For now just output raw to stdout. */
    fputs(s, stdout);
    fflush(stdout);
}

/* ================================================================== */
/*  MISSING BBS CORE FUNCTIONS                                         */
/*  These were likely in source files not included in the archive.      */
/* ================================================================== */

/* inmsg — message input editor.
 * Sets stored_as = 0xffffffff to indicate "no message entered" (cancel). */
typedef struct {
    unsigned long stored_as;
} messagerec_stub;

void inmsg(void *msg, char *title, int *anession, int mode,
           char *subname, int flags)
{
    messagerec_stub *m = (messagerec_stub *)msg;
    (void)title;
    (void)anession;
    (void)mode;
    (void)subname;
    (void)flags;
    fprintf(stderr, "[STUB] inmsg: message input not yet implemented\n");
    if (m) m->stored_as = 0xffffffffUL;
}

/* readfile — read message text from message base file.
 * Returns pointer to allocated buffer, sets *len. */
char *readfile(void *msg, char *subname, long *len)
{
    (void)msg;
    (void)subname;
    fprintf(stderr, "[STUB] readfile: not yet implemented\n");
    if (len) *len = 0;
    return NULL;
}

/* remove_link — remove a message from message base chain */
void remove_link(void *msg, char *subname)
{
    (void)msg;
    (void)subname;
    fprintf(stderr, "[STUB] remove_link: not yet implemented\n");
}

/* sendout_email — send an email message */
void sendout_email(char *title, void *msg, int anession,
                   int usernum, int sysnum, int flag)
{
    (void)title;
    (void)msg;
    (void)anession;
    (void)usernum;
    (void)sysnum;
    (void)flag;
    fprintf(stderr, "[STUB] sendout_email: not yet implemented\n");
}
