/*
 * platform.h — macOS compatibility layer for Dominion BBS
 *
 * Replaces DOS/Borland C-specific headers, types, macros, and functions
 * with POSIX/macOS equivalents or stubs.
 *
 * Original code: Borland C++ 3.1, DOS real-mode, 16-bit
 * Target: clang, macOS, 64-bit
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

/* --- Standard POSIX/C headers that replace DOS headers --- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>    /* strcasecmp */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* --- Name conflict redirects --- */
/* BBS function yn() conflicts with Bessel function yn() in math.h */
/* BBS function wait() conflicts with POSIX wait() in sys/wait.h */
/* BBS local var 'pipe' conflicts with POSIX pipe() in unistd.h */
/* BBS function nl() conflicts with ncurses nl() */
/* BBS function filter() conflicts with ncurses filter() */
#define yn bbs_yn
#define y0 bbs_y0
#define y1 bbs_y1
#define wait bbs_wait
#define pipe bbs_pipe
#define nl bbs_nl
#define filter bbs_filter

/* --- Path buffer size (replaces hardcoded [81] from DOS era) --- */
#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 261
#endif

/* --- Remove far/near pointer qualifiers (flat memory model) --- */
#define far
#define near
#define huge
#define _far
#define _near

/* --- O_BINARY doesn't exist on POSIX (no text/binary distinction) --- */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/* --- DOS file sharing modes (share.h) — no-op on single-user macOS --- */
#define SH_DENYRW    0
#define SH_DENYWR    0
#define SH_DENYRD    0
#define SH_DENYNONE  0
#define SH_COMPAT    0
#define O_DENYALL    0
#define O_DENYWRITE  0
#define O_DENYREAD   0
#define O_DENYNONE   0

/* _fsopen — DOS shared file open, just use fopen on macOS */
#define _fsopen(name, mode, share)  fopen(name, mode)

/* S_IREAD/S_IWRITE — old DOS permission names */
#ifndef S_IREAD
#define S_IREAD  S_IRUSR
#endif
#ifndef S_IWRITE
#define S_IWRITE S_IWUSR
#endif

/* --- DOS standard streams --- */
/* stdprn = standard printer (LPT1:) — just discard on macOS */
#define stdprn  stderr  /* send printer output to stderr */

/* --- Borland-specific pragmas and calling conventions --- */
#define _JAMDATA    /* JAM uses _JAMDATA for pointer decoration */
#define _Cdecl      /* Borland calling convention — default on modern systems */
#define cdecl       /* same */
#define pascal      /* Pascal calling convention — not applicable */

/* --- Suppress Borland pragmas --- */
/* #pragma hdrstop, #pragma warn, etc. are ignored by clang */

/* --- Stack size (no-op on modern systems) --- */
/* extern unsigned _stklen is DOS overlay-related */

/* ================================================================== */
/*  TYPE COMPATIBILITY                                                 */
/* ================================================================== */

/*
 * DOS Borland C: int = 16 bits, long = 32 bits
 * macOS clang:   int = 32 bits, long = 64 bits
 *
 * For the BBS logic (not file I/O structs), we keep native int.
 * The struct definitions in vardec.h need careful handling — the types
 * there are sized for binary file compatibility.
 */

/* --- union REGS for int86() — stubbed out --- */
typedef struct {
    unsigned char al, ah, bl, bh, cl, ch, dl, dh;
} BYTEREGS_X86;

typedef struct {
    unsigned short ax, bx, cx, dx, si, di, cflag, flags;
} WORDREGS_X86;

typedef union REGS {
    BYTEREGS_X86 h;
    WORDREGS_X86 x;
} REGS;

/* int86() — stub, implemented in platform_stubs.c */
int int86(int intno, union REGS *inregs, union REGS *outregs);

/* --- Segment/offset memory (not applicable) --- */
#define MK_FP(seg, off)   ((void*)0)  /* Always returns NULL — never dereference */
#define FP_SEG(p)         (0)
#define FP_OFF(p)         (0)
#define peek(seg, off)    (0)
#define peekb(seg, off)   (0)

/* ================================================================== */
/*  BORLAND CONIO.H REPLACEMENTS                                      */
/* ================================================================== */

/* Colors */
#define BLACK        0
#define BLUE         1
#define GREEN        2
#define CYAN         3
#define RED          4
#define MAGENTA      5
#define BROWN        6
#define LIGHTGRAY    7
#define DARKGRAY     8
#define LIGHTBLUE    9
#define LIGHTGREEN   10
#define LIGHTCYAN    11
#define LIGHTRED     12
#define LIGHTMAGENTA 13
#define YELLOW       14
#define WHITE        15

/* Cursor types */
#define _NOCURSOR      0
#define _SOLIDCURSOR   1
#define _NORMALCURSOR  2

/* Console functions — implemented in platform_stubs.c */
#ifdef __cplusplus
extern "C" {
#endif
void textcolor(int color);
void textattr(int attr);
void clrscr(void);
void gotoxy(int x, int y);
int  wherex(void);
int  wherey(void);
void _setcursortype(int type);
int  kbhit(void);
#if !defined(_IO_NCURSES_H_)
int  getch(void);
#endif
int  getche(void);
void cprintf(const char *fmt, ...);
void cputs(const char *s);
#ifdef __cplusplus
}
#endif

/* directvideo — no-op */
#define directvideo  /* nothing */

/* ================================================================== */
/*  BORLAND DOS.H REPLACEMENTS                                        */
/* ================================================================== */

/* Date/time structures */
struct date {
    int da_year;
    int da_mon;
    int da_day;
};

struct time {
    int ti_hour;
    int ti_min;
    int ti_sec;
    int ti_hund;
};

/* Rename to avoid conflict with POSIX getdate() in time.h */
void dos_getdate(struct date *d);
void dos_gettime(struct time *t);
#define getdate dos_getdate
#define gettime dos_gettime

/* Disk free space */
struct dfree {
    unsigned df_avail;
    unsigned df_total;
    unsigned df_bsec;
    unsigned df_sclus;
};

void getdfree(int drive, struct dfree *df);

/* File time structure (Borland io.h) */
struct ftime {
    unsigned ft_tsec  : 5;
    unsigned ft_min   : 6;
    unsigned ft_hour  : 5;
    unsigned ft_day   : 5;
    unsigned ft_month : 4;
    unsigned ft_year  : 7;
};

/* getftime/setftime — stubs (file times handled by OS) */
#define getftime(fd, ft)  /* no-op */
#define setftime(fd, ft)  /* no-op */

/* disable/enable interrupts — no-op */
#define disable()   /* no-op */
#define enable()    /* no-op */

/* ================================================================== */
/*  BORLAND DIR.H REPLACEMENTS                                        */
/* ================================================================== */

/* File find structures (findfirst/findnext) */
#define MAXPATH  260
#define MAXDRIVE 3
#define MAXDIR   256
#define MAXFILE  256
#define MAXEXT   256

/* Attribute constants for findfirst */
#define FA_RDONLY  0x01
#define FA_HIDDEN  0x02
#define FA_SYSTEM  0x04
#define FA_LABEL   0x08
#define FA_DIREC   0x10
#define FA_ARCH    0x20

struct ffblk {
    char ff_name[256];
    long ff_fsize;
    unsigned ff_attrib;
    /* Internal state for iteration */
    DIR *_dir;
    char _pattern[256];
    char _path[256];
};

int findfirst(const char *pathname, struct ffblk *ffblk, int attrib);
int findnext(struct ffblk *ffblk);

/* Directory/drive functions */
int  getcurdir(int drive, char *buf);
int  getdisk(void);
int  setdisk(int drive);
char *searchpath(const char *file);

/* mkdir — already POSIX, but Borland version takes 1 arg */
/* POSIX mkdir takes 2 args (path, mode) */
/* We'll handle this with a macro */
/* Borland mkdir() takes 1 arg; POSIX takes 2 (path, mode) */
#define _mkdir(p) mkdir(p, 0755)
#define mkdir(p)  mkdir(p, 0755)

/* ================================================================== */
/*  BORLAND IO.H / ALLOC.H REPLACEMENTS                               */
/* ================================================================== */

/* filelength — implemented in platform_stubs.c */
long filelength(int fd);

/* itoa/ltoa/ultoa — implemented in platform_stubs.c */
char *itoa(int value, char *str, int radix);
char *ltoa(long value, char *str, int radix);
char *ultoa(unsigned long value, char *str, int radix);

/* farmalloc/farfree — just malloc/free */
#define farmalloc(n)  malloc(n)
#define farfree(p)    free(p)

/* Memory functions that Borland put in alloc.h or mem.h */
/* memmove, memcpy, memset are already in string.h */

/* stackavail — always return plenty */
#define stackavail()  (1024*1024)

/* ================================================================== */
/*  SOUND (PC SPEAKER) — NO-OP                                        */
/* ================================================================== */

#define sound(freq)   /* no-op */
#define nosound()     /* no-op */

/* delay — use usleep */
#define delay(ms)     usleep((ms) * 1000)

/* ================================================================== */
/*  PORT I/O — NO-OP                                                   */
/* ================================================================== */

#define inportb(port)      (0)
#define outportb(port, val) /* no-op */
#define inport(port)       (0)
#define outport(port, val) /* no-op */

/* ================================================================== */
/*  PROCESS FUNCTIONS                                                  */
/* ================================================================== */

/* spawnvpe — use fork/exec */
#define P_WAIT 0
int spawnvpe(int mode, const char *path, const char *const argv[], const char *const envp[]);

/* ================================================================== */
/*  OVERLAY MANAGEMENT — NO-OP                                         */
/* ================================================================== */

#define _OvrInitExt(a,b)  (-1)  /* Return -1 = "no XMS found" */

/* ================================================================== */
/*  STRING COMPATIBILITY                                               */
/* ================================================================== */

/* BBS strnstr conflicts with macOS strnstr */
#define strnstr bbs_strnstr

/* Borland had these; POSIX has strcasecmp instead */
#define stricmp(a,b)   strcasecmp(a,b)
#define strcmpi(a,b)   strcasecmp(a,b)
#define strnicmp(a,b,n) strncasecmp(a,b,n)

/* strupr/strlwr — implemented in platform_stubs.c */
char *strupr(char *s);
char *strlwr(char *s);

/* ================================================================== */
/*  MISCELLANEOUS                                                      */
/* ================================================================== */

/* Borland's random(n) is a macro: random(n) = rand() % n
 * macOS has random() as a different function (no args, returns long).
 * Override with the Borland-compatible version. */
#define random(n)  (rand() % (n))

/* bioskey — stub */
#define bioskey(cmd)  (0)

/* geninterrupt — no-op */
#define geninterrupt(n)  /* no-op */

/* environ — available on macOS but needs declaration */
extern char **environ;

/* setdta — no-op */
#define setdta(p)  /* no-op */

/* --- Functions in platform_stubs.c that need prototypes for C++ --- */
int  _chmod(const char *path, int func, ...);
int  _getdrive(void);
int  chsize(int fd, long size);
long dostounix(struct date *d, struct time *t);
void unixtodos(long unixtime, struct date *d, struct time *t);
int  fnsplit(const char *path, char *drive, char *dir, char *name, char *ext);
/* swap() declared in swap.h with extern "C" linkage */
#ifndef _SWAP_H_INCLUDED
#ifdef __cplusplus
extern "C"
#endif
int  swap(unsigned char *program_name, unsigned char *command_line,
          unsigned char *exec_return, unsigned char *swap_fname);
#endif
unsigned long bp(char *registration_string, unsigned int security_code);
void pr(char *fmt, ...);

/* --- BBS stub functions (platform_stubs.c) — not yet implemented --- */
void inmsg(void *msg, char *title, int *anession, int mode,
           char *subname, int flags);
char *readfile(void *msg, char *subname, long *len);
void remove_link(void *msg, char *subname);
char *rnam(char *name);
void sendout_email(char *title, void *msg, int anession,
                   int usernum, int sysnum, int flag);

#endif /* _PLATFORM_H_ */
