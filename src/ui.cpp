/*
 * ui.cpp — Declarative UI event loop and input dispatch
 *
 * LEAF COMPONENT: depends on terminal.h and nothing else.
 * No BBS headers, no vardec.h, no vars.h, no globals.
 *
 * Manages multiple sessions over TCP.  Each session has its own
 * Terminal instance and independent UI state (navigator/form stack).
 */

#include "ui.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <algorithm>

/* Max simultaneous sessions */
static const int MAX_SESSIONS = 16;

/* ================================================================== */
/*  Session methods                                                    */
/* ================================================================== */

void Session::push_ui(ActiveUI ui)
{
    ui_stack.push_back(std::move(current_ui));
    current_ui = std::move(ui);
}

void Session::pop_ui()
{
    if (ui_stack.empty()) {
        active = false;
        return;
    }
    current_ui = std::move(ui_stack.back());
    ui_stack.pop_back();
}

/* ================================================================== */
/*  Navigation helpers                                                 */
/* ================================================================== */

static void begin_field(Session& s, Form& form);

static void fire_on_enter(Session& s)
{
    std::visit([&](auto& ui) {
        if (ui.on_enter) ui.on_enter(s);
    }, s.current_ui);

    /* For forms, reset state and show the first field prompt */
    if (auto* form = std::get_if<Form>(&s.current_ui)) {
        s.form_state = FormResult();
        s.current_field_index = 0;
        s.input_buffer.clear();
        begin_field(s, *form);
    }
}

void ui_push(Session& s, ActiveUI ui)
{
    s.push_ui(std::move(ui));
    fire_on_enter(s);
}

void ui_pop(Session& s)
{
    s.pop_ui();
    if (s.active)
        fire_on_enter(s);
}

void ui_quit(Session& s)
{
    s.active = false;
}

/* ================================================================== */
/*  Form input handling                                                */
/* ================================================================== */

/* Begin gathering a field: print its prompt */
static void begin_field(Session& s, Form& form)
{
    if (s.current_field_index >= (int)form.fields.size()) {
        /* All fields gathered — submit */
        s.form_state.completed = true;
        if (form.on_submit)
            form.on_submit(s, s.form_state);
        return;
    }

    FormField& f = form.fields[s.current_field_index];
    s.term.puts(f.prompt.c_str());
    s.input_buffer.clear();

    if (f.kind == InputKind::Confirm) {
        if (f.default_yes)
            s.term.puts("[Y/n] ");
        else
            s.term.puts("[y/N] ");
    }
}

/* Advance to the next field after recording current value */
static void advance_field(Session& s, Form& form, const std::string& value)
{
    FormField& f = form.fields[s.current_field_index];
    s.form_state.values[f.name] = value;
    s.current_field_index++;
    begin_field(s, form);
}

static void dispatch_form_input(Session& s, Form& form, unsigned char ch)
{
    if (s.current_field_index >= (int)form.fields.size())
        return;

    FormField& f = form.fields[s.current_field_index];

    switch (f.kind) {
    case InputKind::TextInput:
        if (ch == 27) {  /* ESC — abort */
            s.term.newline();
            s.form_state.completed = false;
            if (form.on_abort)
                form.on_abort(s);
            return;
        }
        if (ch == '\r' || ch == '\n') {
            s.term.newline();
            advance_field(s, form, s.input_buffer);
            return;
        }
        if (ch == 8 || ch == 127) {  /* BS or DEL */
            if (!s.input_buffer.empty()) {
                s.input_buffer.pop_back();
                s.term.backspace();
            }
            return;
        }
        if (ch >= 32 && ch < 127 && (int)s.input_buffer.size() < f.max_len) {
            s.input_buffer += (char)ch;
            s.term.putch(ch);
        }
        break;

    case InputKind::Choice:
        if (ch == 27) {
            s.term.newline();
            s.form_state.completed = false;
            if (form.on_abort)
                form.on_abort(s);
            return;
        }
        {
            char upper = (char)std::toupper(ch);
            if (f.allowed_keys.find(upper) != std::string::npos) {
                s.term.putch(upper);
                s.term.newline();
                advance_field(s, form, std::string(1, upper));
            }
        }
        break;

    case InputKind::Confirm:
        {
            char upper = (char)std::toupper(ch);
            if (upper == 'Y') {
                s.term.puts("Yes");
                s.term.newline();
                advance_field(s, form, "yes");
            } else if (upper == 'N') {
                s.term.puts("No");
                s.term.newline();
                advance_field(s, form, "no");
            } else if (ch == '\r' || ch == '\n') {
                const char *val = f.default_yes ? "yes" : "no";
                s.term.puts(f.default_yes ? "Yes" : "No");
                s.term.newline();
                advance_field(s, form, val);
            }
        }
        break;
    }
}

/* ================================================================== */
/*  Input dispatch                                                     */
/* ================================================================== */

static void dispatch_input(Session& s, unsigned char ch)
{
    if (auto* nav = std::get_if<Navigator>(&s.current_ui)) {
        char upper = (char)std::toupper(ch);
        for (auto& action : nav->actions) {
            if (std::toupper(action.key) == upper) {
                action.handler(s);
                return;
            }
        }
        /* Unknown key — ignore */
    } else if (auto* form = std::get_if<Form>(&s.current_ui)) {
        dispatch_form_input(s, *form, ch);
    }
}

/* ================================================================== */
/*  TCP listener                                                       */
/* ================================================================== */

static int start_listener(int port)
{
    int fd, opt = 1;
    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("uitest: socket"); return -1; }
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("uitest: bind"); close(fd); return -1;
    }
    if (listen(fd, 4) < 0) {
        perror("uitest: listen"); close(fd); return -1;
    }
    return fd;
}

/* ================================================================== */
/*  Event loop                                                         */
/* ================================================================== */

void ui_run(const UIConfig& config)
{
    Terminal console;
    bool have_console = console.initLocal();

    int listen_fd = -1;
    if (config.listen_port > 0) {
        listen_fd = start_listener(config.listen_port);
        if (listen_fd < 0) {
            if (have_console) console.shutdown();
            return;
        }
    }

    if (!have_console && listen_fd < 0) {
        std::fprintf(stderr, "uitest: need a terminal or -P port\n");
        return;
    }

    /* Sysop console banner */
    if (have_console) {
        console.setAttr(0x0B);
        console.puts("uitest");
        console.setAttr(0x07);
        if (listen_fd >= 0)
            console.printf(" | Listening on port %d", config.listen_port);
        else
            console.puts(" | Local only (no -P flag)");
        console.newline();
        console.setAttr(0x03);
        console.puts("Press Q to quit");
        console.newline();
        console.newline();
    } else {
        std::fprintf(stderr, "uitest: listening on port %d (no console, kill to stop)\n",
                     config.listen_port);
    }

    std::vector<Session*> sessions;
    int next_session_id = 1;
    bool running = true;

    while (running) {
        /* -- Build poll set -- */
        struct pollfd fds[2 + MAX_SESSIONS];
        int nfds = 0;

        /* Slot 0: stdin (sysop keystrokes), only if we have a console */
        int stdin_slot = -1;
        if (have_console) {
            stdin_slot = nfds;
            fds[nfds].fd = STDIN_FILENO;
            fds[nfds].events = POLLIN;
            nfds++;
        }

        /* Slot 1: listen socket (if any) */
        int listen_slot = -1;
        if (listen_fd >= 0) {
            listen_slot = nfds;
            fds[nfds].fd = listen_fd;
            fds[nfds].events = POLLIN;
            nfds++;
        }

        /* Slots 2+: session fds */
        int session_base = nfds;
        for (auto* s : sessions) {
            if (s->active) {
                fds[nfds].fd = s->fd;
                fds[nfds].events = POLLIN;
                nfds++;
            }
        }

        int ret = poll(fds, nfds, 50);
        if (ret < 0) continue;  /* EINTR etc */

        /* -- Sysop keystroke -- */
        if (stdin_slot >= 0 && (fds[stdin_slot].revents & POLLIN)) {
            unsigned char ch = console.localGetKeyNB();
            if (ch == 'Q' || ch == 'q') {
                running = false;
                break;
            }
        }

        /* -- New connection -- */
        if (listen_slot >= 0 && (fds[listen_slot].revents & POLLIN)) {
            if ((int)sessions.size() < MAX_SESSIONS) {
                struct sockaddr_in cli;
                socklen_t len = sizeof(cli);
                int cfd = accept(listen_fd, (struct sockaddr *)&cli, &len);
                if (cfd >= 0) {
                    Session* s = new Session();
                    s->id = next_session_id++;
                    s->fd = cfd;
                    s->term.setRemote(cfd);
                    s->term.sendTelnetNegotiation();
                    s->term.sendTerminalInit();
                    s->current_ui = config.on_connect(*s);
                    fire_on_enter(*s);
                    sessions.push_back(s);

                    if (have_console) {
                        console.setAttr(0x0A);
                        console.printf("Session %d connected (fd=%d)", s->id, cfd);
                        console.newline();
                        console.setAttr(0x07);
                    }
                }
            }
        }

        /* -- Session input -- */
        for (int i = 0; i < (int)sessions.size(); i++) {
            Session* s = sessions[i];
            if (!s->active) continue;

            int slot = session_base + i;
            if (slot >= nfds) break;
            if (!(fds[slot].revents & POLLIN)) continue;

            unsigned char ch = s->term.remoteGetKey();
            if (ch == 0) {
                /* Check if disconnected (remoteGetKey returns 0 and
                 * detaches remote on EOF) */
                if (!s->term.remoteConnected()) {
                    s->active = false;
                }
                continue;
            }
            dispatch_input(*s, ch);
        }

        /* -- Reap dead sessions -- */
        for (auto it = sessions.begin(); it != sessions.end(); ) {
            Session* s = *it;
            if (!s->active) {
                if (have_console) {
                    console.setAttr(0x0C);
                    console.printf("Session %d disconnected", s->id);
                    console.newline();
                    console.setAttr(0x07);
                }
                s->term.sendTerminalRestore();
                s->term.closeRemote();
                delete s;
                it = sessions.erase(it);
            } else {
                ++it;
            }
        }
    }

    /* Shutdown: close all sessions */
    for (auto* s : sessions) {
        s->term.sendTerminalRestore();
        s->term.closeRemote();
        delete s;
    }
    sessions.clear();

    if (listen_fd >= 0) close(listen_fd);
    if (have_console) console.shutdown();
}
