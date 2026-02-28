/*
 * console.cpp — Sysop console multiplexer
 *
 * Every BBS session (local sysop + TCP users) runs as a forked child
 * connected via a PTY pair. The parent owns the ncurses console, TCP
 * listener, and all PTY master fds. A poll-based event loop multiplexes
 * everything.
 *
 * Data flow — TCP session:
 *   Client → parent reads tcp_fd → strip telnet IAC → write pty_master
 *   Child BBS → write pty_slave → parent reads pty_master → forward to tcp_fd + feed VT100
 *
 * Data flow — Local sysop:
 *   Stdin → parent reads → write pty_master → child BBS
 *   Child → pty_master → parent → VT100 → ncurses mirror
 */

#include "console.h"
#include "terminal/vt100.h"
#include "terminal/terminal.h"
#include "terminal_bridge.h"
#include "node_registry.h"
#include "system.h"
#include "session.h"
#include "version.h"
#include "tcpio.h"
#include "lilo.h"
#include "xinit.h"
#include "utility.h"
#include "mm1.h"
#include "menudb.h"
#include "menu_nav.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "jam_bbs.h"
#include "wfc.h"
#include "config.h"
#include "files/diredit.h"
#include "user/uedit.h"
#include "stringed.h"
#include "menued.h"
#include "subedit.h"
#include "sysopf.h"
#include "user/userdb.h"

extern int nc_active;

#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if defined(__APPLE__)
#include <util.h>       /* forkpty() on macOS */
#elif defined(__linux__)
#include <pty.h>        /* forkpty() on Linux */
#endif
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>


/* ================================================================== */
/*  Constants                                                          */
/* ================================================================== */

#define MAX_CONSOLE_SESSIONS 20
#define POLL_TIMEOUT_MS      200
/* status_row() defined after g_console_term declaration */

/* Maintenance function IDs for child_maintenance_pty() */
enum {
    MF_BOARDEDIT = 1,
    MF_DIREDIT,
    MF_PROTEDIT,
    MF_TEXT_EDIT,
    MF_MENUED,
    MF_UEDIT,
    MF_EDSTRING,
    MF_CONFEDIT,
    MF_CONFIG,
    MF_READMAIL,
    MF_POST,
    MF_ZLOG,
    MF_VIEWLOG,
};


/* ================================================================== */
/*  Session state                                                      */
/* ================================================================== */

struct ConsoleSession {
    int  id;                 /* 1-based session number */
    int  pty_master;         /* PTY master fd */
    pid_t child_pid;
    int  tcp_fd;             /* TCP socket (-1 for local) */
    int  node_num;           /* BBS node number (-1 for maintenance) */
    VT100 vt;               /* screen buffer emulator */
    char remote_addr[64];    /* IP or "local" */
    bool active;
    bool is_maintenance;     /* transient maintenance session */
};

static ConsoleSession sessions_[MAX_CONSOLE_SESSIONS];
static int num_sessions_ = 0;
static int viewed_id_ = 0;   /* 0 = WFC view, >0 = session mirror */
static volatile sig_atomic_t sigchld_flag_ = 0;
static volatile sig_atomic_t shutdown_flag_ = 0;


/* ================================================================== */
/*  Signal handlers                                                    */
/* ================================================================== */

static void console_sigchld(int sig)
{
    (void)sig;
    sigchld_flag_ = 1;
}

static void console_sigterm(int sig)
{
    (void)sig;
    shutdown_flag_ = 1;
}


/* ================================================================== */
/*  Session management                                                 */
/* ================================================================== */

static ConsoleSession *find_session(int id)
{
    for (int i = 0; i < num_sessions_; i++)
        if (sessions_[i].id == id && sessions_[i].active)
            return &sessions_[i];
    return nullptr;
}

static int next_session_id()
{
    static int counter = 0;
    return ++counter;
}

static ConsoleSession *add_session(int pty_master, pid_t pid, int tcp_fd,
                                   int node_num, const char *addr, bool maint)
{
    if (num_sessions_ >= MAX_CONSOLE_SESSIONS) return nullptr;
    ConsoleSession *s = &sessions_[num_sessions_++];
    s->id = next_session_id();
    s->pty_master = pty_master;
    s->child_pid = pid;
    s->tcp_fd = tcp_fd;
    s->node_num = node_num;
    s->vt.reset();
    std::strncpy(s->remote_addr, addr, sizeof(s->remote_addr) - 1);
    s->remote_addr[sizeof(s->remote_addr) - 1] = '\0';
    s->active = true;
    s->is_maintenance = maint;
    return s;
}

static void remove_inactive()
{
    int w = 0;
    for (int r = 0; r < num_sessions_; r++) {
        if (sessions_[r].active) {
            if (w != r)
                sessions_[w] = sessions_[r];
            w++;
        }
    }
    num_sessions_ = w;
}

static ConsoleSession *viewed_session()
{
    return find_session(viewed_id_);
}

static ConsoleSession *next_active_session(int current_id, int direction)
{
    if (num_sessions_ == 0) return nullptr;

    int idx = -1;
    for (int i = 0; i < num_sessions_; i++) {
        if (sessions_[i].id == current_id) { idx = i; break; }
    }

    for (int i = 1; i <= num_sessions_; i++) {
        int j = (idx + i * direction + num_sessions_) % num_sessions_;
        if (sessions_[j].active)
            return &sessions_[j];
    }
    return nullptr;
}


/* ================================================================== */
/*  Telnet IAC stripping (parent-side, for TCP→PTY proxy)              */
/* ================================================================== */

/* Simple stateful IAC stripper. Returns number of clean bytes written to out[]. */
static int strip_telnet_iac(const unsigned char *in, int inlen,
                            unsigned char *out, int *iac_state)
{
    int w = 0;
    for (int i = 0; i < inlen; i++) {
        unsigned char c = in[i];
        switch (*iac_state) {
        case 0:
            if (c == 255) *iac_state = 1;
            else out[w++] = c;
            break;
        case 1:
            if (c == 255) { out[w++] = 255; *iac_state = 0; }
            else if (c >= 251 && c <= 254) *iac_state = 2;  /* WILL/WONT/DO/DONT */
            else if (c == 250) *iac_state = 3;               /* SB */
            else *iac_state = 0;
            break;
        case 2:
            *iac_state = 0;
            break;
        case 3:
            if (c == 255) *iac_state = 4;
            break;
        case 4:
            *iac_state = (c == 240) ? 0 : 3;  /* SE */
            break;
        }
    }
    return w;
}


/* ================================================================== */
/*  Child process entry points                                         */
/* ================================================================== */

/*
 * child_session_pty() — BBS session running in PTY child.
 * Similar to child_session() in bbs.cpp but uses PTY slave instead
 * of direct TCP socket. Parent handles telnet with TCP client.
 */
static void child_session_pty(int node_num, bool is_local)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();

    sys.is_child_process = 1;

    if (sys.cdir[0])
        chdir(sys.cdir);

    signal(SIGPIPE, SIG_IGN);

    /* Close listen socket — child doesn't accept connections */
    if (sys.listen_fd >= 0) {
        close(sys.listen_fd);
        sys.listen_fd = -1;
    }

    /* Disable ncurses in child (parent owns the terminal). */
    term_shutdown();
    nc_active = 0;

    /* Re-apply raw mode — term_shutdown() restored the parent's saved
     * termios (ECHO on) onto our PTY slave fd. */
    {
        struct termios raw;
        tcgetattr(STDIN_FILENO, &raw);
        cfmakeraw(&raw);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    frequent_init();

    /* PTY slave is our stdin/stdout/stderr (from forkpty).
     * Set it as the remote fd — BBS output goes through Terminal's
     * remote path (CP437→UTF-8 + ANSI) which parent reads and feeds to VT100. */
    int pty_slave_fd = STDOUT_FILENO;
    io.stream[IO_REMOTE].fd_in = STDIN_FILENO;
    io.stream[IO_REMOTE].fd_out = pty_slave_fd;
    io.stream[IO_REMOTE].needs_iac = 0;  /* no telnet on PTY */
    /* Don't send telnet negotiation or terminal init — parent handles that */
    incom = 1;
    outcom = 1;
    using_modem = is_local ? -1 : 1;
    sess.com_speed = sess.modem_speed = 38400;
    std::strcpy(io.curspeed, is_local ? "Local" : "TCP/IP");

    /* Tell Terminal about the remote fd (without IAC — PTY, not telnet) */
    Terminal *t = (Terminal *)term_instance();
    t->setRemoteNoIac(pty_slave_fd);

    /* Claim our node */
    if (node_num > 0) {
        node_claim(sys.cfg.datadir, node_num, getpid());
        sess.okskey = 1;
    }

    /* Run the session */
    if (is_local) {
        /* Local sysop: auto-login as user 1 */
        sess.usernum = 1;
        { auto __p = UserDB::instance().get(1); if (__p) sess.user = *__p; }
        sess.reset_act_sl();
        sess.changedsl();
        sess.okmacro = 1;
        sess.okskey = 1;
        logon();
    } else {
        getuser();

        if (!io.hangup && sess.usernum > 0) {
            if (node_user_online(sys.cfg.datadir, sess.usernum)) {
                pl("Already logged in on another node.");
                io.hangup = 1;
            }
        }

        if (!io.hangup) {
            if (node_num > 0) {
                node_update(sys.cfg.datadir, node_num, sess.usernum,
                            sess.user.display_name(sess.usernum).c_str(), "Online");
            }
            logon();
        }
    }

    if (!io.hangup) {
        if (!menudb_exists(sys.nifty.firstmenu)) {
            pl("8Main Menu is missing!!  System cannot Continue!");
            io.hangup = 1;
        } else {
            if (sess.actsl <= sys.cfg.newusersl) readmenu(sys.nifty.newusermenu);
            else readmenu(sys.nifty.firstmenu);
        }
        menu_nav_loop();
        logoff();
    }

    if (node_num > 0)
        node_release(sys.cfg.datadir, node_num);
    _exit(0);
}


/*
 * child_maintenance_pty() — Lightweight child for WFC maintenance commands.
 * No node allocation, no login flow. Just runs the function and exits.
 */
static void child_maintenance_pty(int function_id)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();

    sys.is_child_process = 1;

    if (sys.cdir[0])
        chdir(sys.cdir);

    signal(SIGPIPE, SIG_IGN);

    if (sys.listen_fd >= 0) {
        close(sys.listen_fd);
        sys.listen_fd = -1;
    }

    term_shutdown();
    nc_active = 0;

    frequent_init();

    /* Set up PTY as remote output */
    io.stream[IO_REMOTE].fd_in = STDIN_FILENO;
    io.stream[IO_REMOTE].fd_out = STDOUT_FILENO;
    io.stream[IO_REMOTE].needs_iac = 0;
    incom = 1;
    outcom = 1;
    using_modem = -1;  /* local fast-login mode */
    std::strcpy(io.curspeed, "Local");

    /* Load sysop user, set full access */
    sess.usernum = 1;
    { auto __p = UserDB::instance().get(1); if (__p) sess.user = *__p; }
    sess.actsl = 255;
    sess.okskey = 1;
    sess.changedsl();

    /* Dispatch to the requested function */
    switch (function_id) {
    case MF_BOARDEDIT: boardedit(); break;
    case MF_DIREDIT:   diredit();   break;
    case MF_PROTEDIT:  protedit();  break;
    case MF_TEXT_EDIT:  text_edit(); break;
    case MF_MENUED:    menu("");    break;
    case MF_UEDIT:     uedit(1);    break;
    case MF_EDSTRING:  edstring(0); break;
    case MF_CONFEDIT:  confedit();  break;
    case MF_CONFIG:    config();    break;
    case MF_READMAIL:  readmailj(0, 0); break;
    case MF_POST:      post(0);     break;
    case MF_ZLOG:      zlog();      break;
    case MF_VIEWLOG:   viewlog();   break;
    }

    _exit(0);
}


/* ================================================================== */
/*  Fork helpers                                                       */
/* ================================================================== */

static ConsoleSession *fork_tcp_session(int tcp_fd, const char *addr)
{
    auto& sys = System::instance();

    int node_num = node_assign(sys.cfg.datadir, MAX_NODES);
    if (node_num < 0) {
        const char *msg = "\r\nAll nodes are busy. Please try again later.\r\n";
        write(tcp_fd, msg, std::strlen(msg));
        close(tcp_fd);
        return nullptr;
    }

    /* Send telnet negotiation + terminal init directly to TCP client.
     * Do NOT use send_telnet_negotiation() — it calls term_set_remote()
     * which would pollute the parent's Terminal state. */
    {
        unsigned char neg[] = {
            255, 251, 1,   /* IAC WILL ECHO */
            255, 251, 3,   /* IAC WILL SGA */
            255, 253, 31,  /* IAC DO NAWS */
        };
        write(tcp_fd, neg, sizeof(neg));
        const char *init = "\033[?1049h\033[0m\033[40m\033[2J\033[H";
        write(tcp_fd, init, std::strlen(init));
    }

    /* PTY must start in raw mode — no echo, no line buffering.
     * The console handles telnet echo; child handles ANSI output. */
    struct termios pty_termios;
    memset(&pty_termios, 0, sizeof(pty_termios));
    cfmakeraw(&pty_termios);

    int pty_master;
    pid_t pid = forkpty(&pty_master, nullptr, &pty_termios, nullptr);
    if (pid < 0) {
        close(tcp_fd);
        return nullptr;
    }

    if (pid == 0) {
        /* Child — close tcp_fd (parent proxies) */
        close(tcp_fd);
        child_session_pty(node_num, false);
        _exit(0);
    }

    /* Parent — set pty_master non-blocking */
    fcntl(pty_master, F_SETFL, O_NONBLOCK);

    return add_session(pty_master, pid, tcp_fd, node_num, addr, false);
}

static ConsoleSession *fork_local_session()
{
    auto& sys = System::instance();

    int node_num = node_assign(sys.cfg.datadir, MAX_NODES);
    if (node_num < 0) return nullptr;

    struct termios pty_termios;
    memset(&pty_termios, 0, sizeof(pty_termios));
    cfmakeraw(&pty_termios);

    int pty_master;
    pid_t pid = forkpty(&pty_master, nullptr, &pty_termios, nullptr);
    if (pid < 0) return nullptr;

    if (pid == 0) {
        child_session_pty(node_num, true);
        _exit(0);
    }

    fcntl(pty_master, F_SETFL, O_NONBLOCK);
    return add_session(pty_master, pid, -1, node_num, "local", false);
}

static ConsoleSession *fork_maintenance(int function_id)
{
    struct termios pty_termios;
    memset(&pty_termios, 0, sizeof(pty_termios));
    cfmakeraw(&pty_termios);

    int pty_master;
    pid_t pid = forkpty(&pty_master, nullptr, &pty_termios, nullptr);
    if (pid < 0) return nullptr;

    if (pid == 0) {
        child_maintenance_pty(function_id);
        _exit(0);
    }

    fcntl(pty_master, F_SETFL, O_NONBLOCK);
    return add_session(pty_master, pid, -1, -1, "maint", true);
}


/* ================================================================== */
/*  Rendering                                                          */
/* ================================================================== */

static Terminal *g_console_term = nullptr;

/* Last row of the terminal — status bar goes here */
static int status_row() {
    if (!g_console_term) return 24;
    int lines = g_console_term->ncLines();
    return (lines > 0 ? lines : 25) - 1;
}

static void render_status_bar()
{
    if (!g_console_term) return;

    char line[256];
    int pos = 0;

    if (viewed_id_ == 0) {
        pos += std::snprintf(line + pos, sizeof(line) - pos, " [W*]");
    } else {
        pos += std::snprintf(line + pos, sizeof(line) - pos, " [W]");
    }

    for (int i = 0; i < num_sessions_; i++) {
        if (!sessions_[i].active) continue;
        const char *name = sessions_[i].remote_addr;
        bool viewing = (sessions_[i].id == viewed_id_);
        if (viewing)
            pos += std::snprintf(line + pos, sizeof(line) - pos, " [%d:%s*]",
                                 sessions_[i].id, name);
        else
            pos += std::snprintf(line + pos, sizeof(line) - pos, " [%d:%s]",
                                 sessions_[i].id, name);
    }

    /* Pad with key hints — respect actual terminal width */
    int cols = g_console_term->ncCols();
    if (cols < 40) cols = 80;  /* fallback if not initialized */
    int remaining = cols - pos;
    if (remaining > 0) {
        const char *keys = "  F1=WFC F2/F3=Next F4=Dis F10=Quit";
        const char *keys_long = "  F1=WFC F2/F3=Prev/Next F4=Discon F10=Quit";
        const char *chosen = (remaining >= (int)std::strlen(keys_long))
                             ? keys_long : keys;
        int klen = (int)std::strlen(chosen);
        int pad = remaining - klen;
        if (pad > 0) {
            for (int i = 0; i < pad; i++)
                line[pos++] = ' ';
        }
        std::snprintf(line + pos, sizeof(line) - pos, "%s", chosen);
    }

    g_console_term->drawStatusLine(status_row(), line, 0x1F);  /* white on blue */
}

static void render_wfc_view()
{
    if (!g_console_term) return;
    auto& sys = System::instance();

    /* Clear area up to (but not including) the status bar row */
    g_console_term->clearArea(0, status_row());

    /* Title bar */
    time_t now = time(nullptr);
    struct tm *lt = localtime(&now);
    char datebuf[64];
    std::strftime(datebuf, sizeof(datebuf), "%a %b %d, %Y  %l:%M%p", lt);

    char title[160];
    std::snprintf(title, sizeof(title), " %-40s %s", wwiv_version, datebuf);
    g_console_term->drawStatusLine(0, title, 0x1E);  /* yellow on blue */

    /* Port / node info */
    int active_count = 0;
    for (int i = 0; i < num_sessions_; i++)
        if (sessions_[i].active && !sessions_[i].is_maintenance)
            active_count++;

    char info[160];
    std::snprintf(info, sizeof(info),
                  " Port: %d   Nodes: %d/%d   Calls Today: %d",
                  sys.tcp_port, active_count, MAX_NODES,
                  sys.status.callstoday);
    g_console_term->drawStatusLine(1, info, 0x0F);  /* bright white on black */

    /* Separator */
    g_console_term->drawStatusLine(2, " ──────────────────────────────────────────────────────────────────────────────", 0x08);

    /* Active nodes header */
    g_console_term->drawStatusLine(3, " Active Nodes:", 0x0B);  /* bright cyan */

    char hdr[160];
    std::snprintf(hdr, sizeof(hdr), "  %-3s %-18s %-16s %-16s %s", "#", "User", "IP", "Status", "Since");
    g_console_term->drawStatusLine(4, hdr, 0x07);

    /* Node list from registry */
    NodeInfo nodes[MAX_NODES];
    int count = node_list(sys.cfg.datadir, nodes, MAX_NODES);

    int row = 5;
    for (int i = 0; i < count && row < 15; i++) {
        char since_buf[32] = "";
        struct tm *st = localtime(&nodes[i].since);
        if (st) std::strftime(since_buf, sizeof(since_buf), "%H:%M", st);

        char line[160];
        std::snprintf(line, sizeof(line), "  %-3d %-18s %-16s %-16s %s",
                      nodes[i].node,
                      nodes[i].user_name[0] ? nodes[i].user_name : "(connecting)",
                      "TCP",
                      nodes[i].status,
                      since_buf);
        g_console_term->drawStatusLine(row++, line, 0x07);
    }

    /* Also show local sessions from our session list */
    for (int i = 0; i < num_sessions_ && row < 15; i++) {
        if (!sessions_[i].active || sessions_[i].is_maintenance) continue;
        if (sessions_[i].tcp_fd >= 0) continue;  /* already shown via node_list */
        char line[160];
        std::snprintf(line, sizeof(line), "  %-3d %-18s %-16s %-16s",
                      sessions_[i].id, "SYSOP", "local", "Active");
        g_console_term->drawStatusLine(row++, line, 0x07);
    }

    /* Separator */
    g_console_term->drawStatusLine(16, " ──────────────────────────────────────────────────────────────────────────────", 0x08);

    /* Maintenance commands */
    g_console_term->drawStatusLine(17, " Maintenance:", 0x0B);
    g_console_term->drawStatusLine(18, "  B=Boards  D=Dirs  U=Users  O=Config  #=Menus  C=Conf", 0x07);
    g_console_term->drawStatusLine(19, "  E=Text  S=Strings  X=Protos  M=Mail  W=Post  Z=Zlog  V=Log", 0x07);
    g_console_term->drawStatusLine(status_row() - 1, "  L=Login  Q=Quit", 0x0A);  /* bright green */
}

static void refresh_display()
{
    if (!g_console_term) return;

    if (viewed_id_ == 0) {
        render_wfc_view();
    } else {
        ConsoleSession *s = viewed_session();
        if (s) {
            g_console_term->renderBuffer(s->vt.buffer(), 0, 25);
        }
    }
    render_status_bar();
}


/* ================================================================== */
/*  Input dispatch                                                     */
/* ================================================================== */

/* F-key scancodes (DOS-style, as returned by Terminal::ncToScancode):
 *   F1=59 F2=60 F3=61 F4=62 ... F10=68 */
#define SC_F1   59
#define SC_F2   60
#define SC_F3   61
#define SC_F4   62
#define SC_F10  68

static bool handle_fkey(int scancode)
{
    switch (scancode) {
    case SC_F1:
        /* Toggle WFC view */
        viewed_id_ = 0;
        refresh_display();
        return true;

    case SC_F2: {
        /* Previous session */
        ConsoleSession *s = next_active_session(viewed_id_, -1);
        if (s) { viewed_id_ = s->id; refresh_display(); }
        return true;
    }

    case SC_F3: {
        /* Next session */
        ConsoleSession *s = next_active_session(viewed_id_, 1);
        if (s) { viewed_id_ = s->id; refresh_display(); }
        return true;
    }

    case SC_F4: {
        /* Disconnect viewed session */
        ConsoleSession *s = viewed_session();
        if (s && s->active) {
            kill(s->child_pid, SIGTERM);
        }
        return true;
    }

    case SC_F10:
        /* Shutdown */
        shutdown_flag_ = 1;
        return true;
    }

    return false;
}

static void handle_wfc_key(unsigned char ch)
{
    ch = (ch >= 'a' && ch <= 'z') ? ch - 32 : ch;  /* toupper */

    int maint = 0;
    switch (ch) {
    case 'B': maint = MF_BOARDEDIT; break;
    case 'D': maint = MF_DIREDIT;   break;
    case 'X': maint = MF_PROTEDIT;  break;
    case 'E': maint = MF_TEXT_EDIT;  break;
    case '#': maint = MF_MENUED;     break;
    case 'U': maint = MF_UEDIT;     break;
    case 'S': maint = MF_EDSTRING;   break;
    case 'C': maint = MF_CONFEDIT;   break;
    case 'O': maint = MF_CONFIG;     break;
    case 'M': maint = MF_READMAIL;   break;
    case 'W': maint = MF_POST;       break;
    case 'Z': maint = MF_ZLOG;       break;
    case 'V': maint = MF_VIEWLOG;    break;

    case 'L': {
        /* Local sysop login */
        ConsoleSession *s = fork_local_session();
        if (s) {
            viewed_id_ = s->id;
            refresh_display();
        }
        return;
    }

    case 'Q':
        shutdown_flag_ = 1;
        return;

    default:
        /* Number keys: jump to session by ID */
        if (ch >= '1' && ch <= '9') {
            int target = ch - '0';
            ConsoleSession *s = find_session(target);
            if (s) {
                viewed_id_ = s->id;
                refresh_display();
            }
        }
        return;
    }

    if (maint) {
        ConsoleSession *s = fork_maintenance(maint);
        if (s) {
            viewed_id_ = s->id;
            refresh_display();
        }
    }
}


/* ================================================================== */
/*  Child reaping                                                      */
/* ================================================================== */

static void reap_children()
{
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < num_sessions_; i++) {
            if (sessions_[i].child_pid == pid) {
                sessions_[i].active = false;

                /* Close PTY master */
                if (sessions_[i].pty_master >= 0) {
                    close(sessions_[i].pty_master);
                    sessions_[i].pty_master = -1;
                }
                /* Close TCP socket — write restore sequence directly
                 * (don't use send_terminal_restore which goes through Terminal bridge) */
                if (sessions_[i].tcp_fd >= 0) {
                    const char *restore = "\033[0m\033[?1049l";
                    write(sessions_[i].tcp_fd, restore, std::strlen(restore));
                    close(sessions_[i].tcp_fd);
                    sessions_[i].tcp_fd = -1;
                }

                /* If we were viewing this session, switch back to WFC */
                if (viewed_id_ == sessions_[i].id) {
                    viewed_id_ = 0;
                }
                break;
            }
        }
    }
    remove_inactive();
}


/* ================================================================== */
/*  Main event loop                                                    */
/* ================================================================== */

void console_run(void)
{
    auto& sys = System::instance();

    /* Get Terminal instance for console rendering */
    g_console_term = (Terminal *)term_instance();

    /* Install signal handlers */
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = console_sigchld;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, nullptr);

    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = console_sigterm;
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    /* Initial render */
    viewed_id_ = 0;
    refresh_display();

    time_t last_wfc_refresh = time(nullptr);
    time_t last_reap = time(nullptr);
    int pending_scancode = -1;

    while (!shutdown_flag_ && !sys.endday) {

        /* --- Build poll set --- */
        std::vector<struct pollfd> pfds;
        std::vector<int> pfd_type;    /* 0=stdin, 1=listen, 2=pty, 3=tcp */
        std::vector<int> pfd_index;   /* index into sessions_[] for type 2/3 */

        /* stdin */
        {
            struct pollfd pf = {};
            pf.fd = STDIN_FILENO;
            pf.events = POLLIN;
            pfds.push_back(pf);
            pfd_type.push_back(0);
            pfd_index.push_back(-1);
        }

        /* Listen socket */
        if (sys.listen_fd >= 0) {
            struct pollfd pf = {};
            pf.fd = sys.listen_fd;
            pf.events = POLLIN;
            pfds.push_back(pf);
            pfd_type.push_back(1);
            pfd_index.push_back(-1);
        }

        /* PTY masters + TCP sockets */
        for (int i = 0; i < num_sessions_; i++) {
            if (!sessions_[i].active) continue;

            if (sessions_[i].pty_master >= 0) {
                struct pollfd pf = {};
                pf.fd = sessions_[i].pty_master;
                pf.events = POLLIN;
                pfds.push_back(pf);
                pfd_type.push_back(2);
                pfd_index.push_back(i);
            }

            if (sessions_[i].tcp_fd >= 0) {
                struct pollfd pf = {};
                pf.fd = sessions_[i].tcp_fd;
                pf.events = POLLIN;
                pfds.push_back(pf);
                pfd_type.push_back(3);
                pfd_index.push_back(i);
            }
        }

        /* --- Poll --- */
        int nready = poll(pfds.data(), pfds.size(), POLL_TIMEOUT_MS);

        /* --- Handle SIGCHLD --- */
        if (sigchld_flag_) {
            sigchld_flag_ = 0;
            reap_children();
            refresh_display();
        }

        if (nready <= 0) {
            /* Periodic WFC refresh (every 2 seconds) */
            time_t now = time(nullptr);
            if (viewed_id_ == 0 && now - last_wfc_refresh >= 2) {
                render_wfc_view();
                render_status_bar();
                last_wfc_refresh = now;
            }
            /* Periodic stale node reap (every 30s) */
            if (now - last_reap >= 30) {
                node_reap_stale(sys.cfg.datadir);
                last_reap = now;
            }
            continue;
        }

        /* --- Process events --- */
        bool need_refresh = false;

        for (size_t p = 0; p < pfds.size(); p++) {
            if (!(pfds[p].revents & (POLLIN | POLLHUP | POLLERR)))
                continue;

            switch (pfd_type[p]) {
            case 0: {
                /* stdin — local keyboard */
                /* Use ncurses to read keys (handles escape sequences) */
                unsigned char ch = g_console_term->localGetKeyNB();
                if (ch == 255) break;  /* nothing */

                /* Check for pending scancode (F-keys etc.) */
                if (ch == 0) {
                    /* Next call will return the scancode */
                    ch = g_console_term->localGetKeyNB();
                    if (ch != 255 && handle_fkey(ch))
                        break;
                    /* Not an F-key we handle — ignore */
                    break;
                }

                if (viewed_id_ == 0) {
                    /* WFC view — dispatch WFC commands */
                    handle_wfc_key(ch);
                } else {
                    /* Session mirror — forward to PTY */
                    ConsoleSession *s = viewed_session();
                    if (s && s->pty_master >= 0) {
                        write(s->pty_master, &ch, 1);
                    }
                }
                break;
            }

            case 1: {
                /* Listen socket — new TCP connection */
                struct sockaddr_in caddr;
                socklen_t clen = sizeof(caddr);
                int accepted_fd = accept(sys.listen_fd, (struct sockaddr *)&caddr, &clen);
                if (accepted_fd >= 0) {
                    char addr[64];
                    inet_ntop(AF_INET, &caddr.sin_addr, addr, sizeof(addr));
                    ConsoleSession *s = fork_tcp_session(accepted_fd, addr);
                    if (s) need_refresh = true;
                }
                break;
            }

            case 2: {
                /* PTY master readable — child output */
                int idx = pfd_index[p];
                ConsoleSession *s = &sessions_[idx];

                unsigned char buf[4096];
                ssize_t n = read(s->pty_master, buf, sizeof(buf));

                if (n <= 0) {
                    /* EOF or error — child is done */
                    if (pfds[p].revents & POLLHUP) {
                        /* Child closed PTY — will be reaped via SIGCHLD */
                    }
                    break;
                }

                /* Forward to TCP client (already UTF-8 from BBS) */
                if (s->tcp_fd >= 0) {
                    ssize_t written = 0;
                    while (written < n) {
                        ssize_t w = write(s->tcp_fd, buf + written, n - written);
                        if (w <= 0) {
                            /* TCP client disconnected */
                            close(s->tcp_fd);
                            s->tcp_fd = -1;
                            kill(s->child_pid, SIGTERM);
                            break;
                        }
                        written += w;
                    }
                }

                /* Feed VT100 emulator */
                s->vt.feedBlock(buf, (int)n);

                /* If this is the viewed session, refresh */
                if (s->id == viewed_id_)
                    need_refresh = true;
                break;
            }

            case 3: {
                /* TCP socket readable — client input */
                int idx = pfd_index[p];
                ConsoleSession *s = &sessions_[idx];

                unsigned char buf[4096];
                ssize_t n = read(s->tcp_fd, buf, sizeof(buf));

                if (n <= 0) {
                    /* Client disconnected */
                    close(s->tcp_fd);
                    s->tcp_fd = -1;
                    kill(s->child_pid, SIGTERM);
                    break;
                }

                /* Strip telnet IAC and forward to PTY */
                static int iac_states[MAX_CONSOLE_SESSIONS] = {};
                unsigned char clean[4096];
                int clean_len = strip_telnet_iac(buf, (int)n, clean,
                                                 &iac_states[idx]);
                if (clean_len > 0 && s->pty_master >= 0) {
                    write(s->pty_master, clean, clean_len);
                }
                break;
            }
            }
        }

        if (need_refresh)
            refresh_display();

        /* Periodic tasks */
        time_t now = time(nullptr);
        if (viewed_id_ == 0 && now - last_wfc_refresh >= 2) {
            render_wfc_view();
            render_status_bar();
            last_wfc_refresh = now;
        }
        if (now - last_reap >= 30) {
            node_reap_stale(sys.cfg.datadir);
            last_reap = now;
        }
    }

    /* --- Graceful shutdown --- */

    /* Signal all children */
    for (int i = 0; i < num_sessions_; i++) {
        if (sessions_[i].active && sessions_[i].child_pid > 0)
            kill(sessions_[i].child_pid, SIGTERM);
    }

    /* Wait up to 5 seconds */
    time_t deadline = time(nullptr) + 5;
    while (time(nullptr) < deadline) {
        int ret = waitpid(-1, nullptr, WNOHANG);
        if (ret <= 0) {
            usleep(100000);

            /* Check if any children remain */
            bool any = false;
            for (int i = 0; i < num_sessions_; i++)
                if (sessions_[i].active) { any = true; break; }
            if (!any) break;
            continue;
        }
        /* Mark reaped session inactive */
        for (int i = 0; i < num_sessions_; i++) {
            if (sessions_[i].child_pid == ret) {
                sessions_[i].active = false;
                if (sessions_[i].pty_master >= 0)
                    close(sessions_[i].pty_master);
                if (sessions_[i].tcp_fd >= 0)
                    close(sessions_[i].tcp_fd);
            }
        }
    }

    /* Kill stragglers */
    for (int i = 0; i < num_sessions_; i++) {
        if (sessions_[i].active && sessions_[i].child_pid > 0) {
            kill(sessions_[i].child_pid, SIGKILL);
            waitpid(sessions_[i].child_pid, nullptr, 0);
            if (sessions_[i].pty_master >= 0)
                close(sessions_[i].pty_master);
            if (sessions_[i].tcp_fd >= 0)
                close(sessions_[i].tcp_fd);
        }
    }

    g_console_term = nullptr;
}
