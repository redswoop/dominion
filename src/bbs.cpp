#pragma hdrstop

#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "tcpio.h"
#include "conio.h"
#include "timest.h"
#include "disk.h"
#include "utility.h"
#include "wfc.h"
#include "mm1.h"
#include "stringed.h"
#include "session.h"
#include "system.h"
#include "version.h"
#include <math.h>
#include "menudb.h"
#include "mci_bbs.h"
#include "menu_nav.h"
#include "extrn.h"
#include "xinit.h"
#include "lilo.h"
#include "node_registry.h"
#include "terminal_bridge.h"

extern int nc_active;  /* from terminal_bridge.cpp, via io_ncurses.h */

#include <signal.h>
#include <sys/wait.h>

#define modem_time 3.5
extern double thing;

int node=0,SYSTEMDEBUG=0;

/* Flag for graceful supervisor shutdown */
static volatile sig_atomic_t shutdown_requested = 0;

static void sigchld_handler(int sig)
{
    (void)sig;
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = saved_errno;
}

static void sigterm_handler(int sig)
{
    (void)sig;
    shutdown_requested = 1;
}

/*
 * child_session() — forked child process handles one TCP user.
 * Called after fork(). Never returns — calls _exit().
 */
static void child_session(int accepted_fd, int node_num)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();

    sys.is_child_process = 1;

    /* Restore cwd — config paths may be relative (e.g. ./menus/, ./data/).
     * Parent's cwd should be stable, but be defensive after fork. */
    if (sys.cdir[0])
        chdir(sys.cdir);

    /* Ignore SIGPIPE — writes to dead TCP sockets return EPIPE instead
     * of killing the process.  Standard for network server children. */
    signal(SIGPIPE, SIG_IGN);

    /* Close the listen socket — child doesn't accept connections */
    if (sys.listen_fd >= 0) {
        close(sys.listen_fd);
        sys.listen_fd = -1;
    }

    /* Disable ncurses in child (parent owns the terminal) */
    term_shutdown();
    nc_active = 0;

    /* Reset session state for this connection */
    frequent_init();

    /* Set up TCP I/O streams */
    io.stream[IO_REMOTE].fd_in = accepted_fd;
    io.stream[IO_REMOTE].fd_out = accepted_fd;
    io.stream[IO_REMOTE].needs_iac = 1;
    send_telnet_negotiation(accepted_fd);
    send_terminal_init(accepted_fd);
    incom = 1;
    outcom = 1;
    using_modem = 1;
    sess.com_speed = sess.modem_speed = 38400;
    strcpy(io.curspeed, "TCP/IP");

    /* Claim our node */
    node_claim(sys.cfg.datadir, node_num, getpid());
    sess.okskey = 1;

    /* Run the session */
    getuser();

    if (!io.hangup && sess.usernum > 0) {
        /* Double-login check */
        if (node_user_online(sys.cfg.datadir, sess.usernum)) {
            pl("Already logged in on another node.");
            io.hangup = 1;
        }
    }

    if (!io.hangup) {
        node_update(sys.cfg.datadir, node_num, sess.usernum,
                    sess.user.display_name(sess.usernum).c_str(), "Online");
        logon();

        if(!menudb_exists(sys.nifty.firstmenu)) {
            pl("8Main Menu is missing!!  System cannot Continue!  If Possible, Inform SysOp!");
            logpr("7!0 MAIN MENU MISSING.  Hanging up on User");
            pausescr();
            io.hangup=1;
        }
        else {
            if(sess.actsl<=sys.cfg.newusersl) readmenu(sys.nifty.newusermenu);
            else readmenu(sys.nifty.firstmenu);
        }
        menu_nav_loop();
        logoff();
    }

    /* Clean up */
    if (client_fd >= 0)
        send_terminal_restore(client_fd);
    dtr(0);
    node_release(sys.cfg.datadir, node_num);
    _exit(0);
}

/*
 * supervisor_loop() — parent process: accept connections, fork children.
 * Used when ok_modem_stuff is set and we have a listen socket (TCP mode).
 */
static void supervisor_loop()
{
    auto& sys = System::instance();

    int max_nodes = MAX_NODES;

    /* Install SIGCHLD handler to reap children.
     * Skip in -Q mode: we do an explicit waitpid() there, and the
     * async handler would race (reap the child before we can). */
    struct sigaction sa;
    if (!sys.ooneuser) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = sigchld_handler;
        sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
        sigaction(SIGCHLD, &sa, NULL);
    }

    /* Install SIGTERM/SIGINT handler for graceful shutdown */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigterm_handler;
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    time_t last_reap = time(NULL);

    while (!sys.endday && !shutdown_requested) {
        int accepted_fd = wfc_poll_accept();
        if (accepted_fd < 0)
            continue;

        int node_num = node_assign(sys.cfg.datadir, max_nodes);
        if (node_num < 0) {
            /* All nodes busy — send a message and close */
            const char *busy_msg = "\r\nAll nodes are busy. Please try again later.\r\n";
            write(accepted_fd, busy_msg, strlen(busy_msg));
            close(accepted_fd);
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            /* Fork failed */
            close(accepted_fd);
            continue;
        }

        if (pid == 0) {
            /* Child process */
            child_session(accepted_fd, node_num);
            /* Never reaches here */
        }

        /* Parent: close the accepted fd (child owns it now) */
        close(accepted_fd);

        /* -Q mode: wait for single child, then exit */
        if (sys.ooneuser) {
            pid_t ret;
            do {
                ret = waitpid(pid, NULL, 0);
                if (ret == -1 && errno == EINTR && shutdown_requested) {
                    /* Parent got SIGTERM — forward to child and wait briefly */
                    kill(pid, SIGTERM);
                    usleep(500000);
                    if (waitpid(pid, NULL, WNOHANG) <= 0)
                        kill(pid, SIGKILL);
                    waitpid(pid, NULL, 0);
                    break;
                }
            } while (ret == -1 && errno == EINTR);
            break;
        }

        /* Periodically reap stale node files (every 30s) */
        time_t now = time(NULL);
        if (now - last_reap >= 30) {
            node_reap_stale(sys.cfg.datadir);
            last_reap = now;
        }
    }

    /* Graceful shutdown: signal all children */
    if (shutdown_requested) {
        NodeInfo nodes[MAX_NODES];
        int count = node_list(sys.cfg.datadir, nodes, MAX_NODES);
        for (int i = 0; i < count; i++) {
            if (nodes[i].pid > 0)
                kill(nodes[i].pid, SIGTERM);
        }
        /* Wait for children to exit (up to 5 seconds) */
        time_t deadline = time(NULL) + 5;
        while (time(NULL) < deadline) {
            int ret = waitpid(-1, NULL, WNOHANG);
            if (ret <= 0) {
                usleep(100000);
                continue;
            }
        }
        /* Kill any stragglers */
        count = node_list(sys.cfg.datadir, nodes, MAX_NODES);
        for (int i = 0; i < count; i++) {
            if (nodes[i].pid > 0) {
                kill(nodes[i].pid, SIGKILL);
                node_release(sys.cfg.datadir, nodes[i].node);
            }
        }
    }
}

/*
 * inline_session_loop() — run sessions inline (no fork).
 * Used for -M (local mode) and -L (local sysop login).
 */
static void inline_session_loop(unsigned int ui, unsigned int us)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    double dt;

    if (sys.restoring_shrink) {
        sys.restoring_shrink=0;
        _setcursortype(2);
        switch(restore_data("exitdata.dom")) {
        case 0: /* WFC */
            _setcursortype(0);
            goto wfc_label;
        case 1: /* main menu */
        case 2:
            read_menu(sess.menuat,0);
            goto main_menu_label;
        }
    }
    else _setcursortype(0);

    do {
        if (sess.already_on)
            gotcaller(ui, us);
        else
             getcaller();

        if (using_modem>-1) {
                getuser();
        }
        else {
                using_modem=0;
            sess.checkit=0;
            sess.okmacro=1;
            sess.usernum=1;
            reset_act_sl();
            changedsl();
        }

        if (!io.hangup) {
            logon();
            if(!menudb_exists(sys.nifty.firstmenu)) {
                pl("8Main Menu is missing!!  System cannot Continue!  If Possible, Inform SysOp!");
                logpr("7!0 MAIN MENU MISSING.  Hanging up on User");
                pausescr();
                io.hangup=1;
            }
            else {
                if(sess.actsl<=sys.cfg.newusersl) readmenu(sys.nifty.newusermenu);
                else readmenu(sys.nifty.firstmenu);
            }
main_menu_label:
            menu_nav_loop();
            logoff();
        }

        if (client_fd >= 0)
            send_terminal_restore(client_fd);
        if (!sys.no_hangup && ok_modem_stuff)
            dtr(0);

        frequent_init();
wfc_label:
        if ((!sys.no_hangup) && ok_modem_stuff)
            dtr(0);
        sess.already_on=0;
        if (sess.sysop_alert && (!kbhitb())) {
            dt=timer();
            nl();
            pl("User Has Logged Off");
            nl();
            while ((!kbhitb()) && (fabs(timer()-dt)<60.0)) {
                wait1(9);
                wait1(18);
            }
            clrscrb();
        }
        sess.sysop_alert=0;
    }
    while ((!sys.endday) && (!sys.ooneuser));
}


int main(int argc, char *argv[])
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],ch;
    int i,port,show=0;
    unsigned int ui=0, us=0;
    int splash_pause=0;

    if(getenv("DOM")) cd_to(getenv("DOM"));
    if(!exist("config.json")) {
        cd_to(searchpath("config.json"));
    }

    /* Singletons self-initialize via constructors */
    mci_bbs_init();
    sess.already_on=0;
    sys.endday=0;
    sys.oklevel=0;
    sys.noklevel=255;
    sys.ooneuser=0;
    sys.no_hangup=0;
    ok_modem_stuff=1;
    sys.tcp_port=0;
    sys.listen_fd=-1;
    io.term_raw_mode=0;
    if (exist("exitdata.dom"))
        sys.restoring_shrink=1;
    else
        sys.restoring_shrink=0;

    port=0;

    textcolor(9);
    cprintf("  ");
    textcolor(15);
    cprintf("%s, Dominous 1993\n\n\r",wwiv_version);

    for (i=1; i<argc; i++) {
        strcpy(s,argv[i]);
        if ((s[0]=='-') || (s[0]=='/')) {
            ch=toupper(s[1]);
            switch(ch) {
            case '?':
                printf("/N  - System Node\n");
                printf("/P  - Com Port (1-5) or TCP port (e.g. -P2323)\n");
                printf("/Q  - quit the bbs after one user done\n");
                printf("/H  - don't hang up on user when he loggs off\n");
                printf("/M  - don't access modem at all\n");
                printf("/L  - Logon as SysOp Locally\n");
                printf("/I  - Load Quietly(Bypass all loading screens)\n");
                printf("/W  - Pause at splash screen\n");
                printf("\n");
                exit(0);
            case 'D':
                SYSTEMDEBUG=1;
                break;
            case 'L':
                sess.already_on=2;
                ui=19200;
                us=19200;
                break;
            case 'E':
                sys.oklevel=atoi(&(s[2]));
                break;
            case 'N':
                node=atoi(s+2);
                break;
            case 'P':
                if (s[2] >= '0' && s[2] <= '9' && atoi(s+2) > 255) {
                    sys.tcp_port=atoi(s+2);
                } else {
                    port=atoi(s+2);
                }
                break;
            case 'Q':
                sys.ooneuser=1;
                break;
            case 'H':
                sys.no_hangup=1;
                break;
            case 'I':
                show=opp(show);
                break;
            case 'M':
                ok_modem_stuff=0;
                break;
            case 'W':
                splash_pause=1;
                break;
            }
        }
    }

    init(show);
    if(port!=0) sys.cfg.primaryport=port;

    if(node) cprintf("System is Node %d using port %d",node,sys.cfg.primaryport);

    if(exist("critical")) {
        pl("8Cirtical Errors have occured!  Read Error.log!");
        sound(1400);
        delay(500);
        nosound();
        unlink("critical");
    }

    if(splash_pause) {
        printf("\nPress any key...");
        fflush(stdout);
        getchar();
    }

    /*
     * Dispatch to the right run mode:
     *
     * TCP mode (has listen socket): Fork-per-connection supervisor loop.
     *   -Q: fork one child, waitpid, exit.
     *   Default: multi-user, runs until shutdown.
     *
     * Local mode (-M or -L): Inline session loop, no forking.
     *   -M: ok_modem_stuff=0, no TCP. Uses getcaller() WFC loop.
     *   -L: already_on=2, gotcaller() path.
     */
    if (ok_modem_stuff && sys.tcp_port > 0) {
        /* TCP mode — open listen socket, then fork per connection */
        initport(0);
        supervisor_loop();
    } else {
        /* Local mode — inline session, no fork */
        inline_session_loop(ui, us);
    }

    outs("\x0c");
    end_bbs(sys.oklevel);
}
