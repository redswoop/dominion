/*
 * ui.cpp — Declarative UI event loop and input dispatch
 *
 * LEAF COMPONENT: depends on screen_form.h (via ui.h) and nothing else.
 * No BBS headers, no vardec.h, no vars.h, no globals.
 *
 * Manages multiple sessions over TCP.  Each session has its own
 * Terminal instance and independent UI state (navigator/form stack).
 *
 * ScreenForm engine lives in screen_form.cpp (canonical library).
 * This file adds: Navigator dispatch, sequential Form dispatch,
 * multi-session event loop with poll().
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

/* Perform form exit transition (sequential Form only) */
static void form_exit(Session& s, FormExit exit)
{
    switch (exit) {
    case FormExit::Clear:
        s.term.clearScreen();
        break;
    case FormExit::Scroll:
        s.term.newline();
        break;
    case FormExit::None:
        break;
    }
}

static void fire_on_enter(Session& s)
{
    if (auto* sf = std::get_if<ScreenForm>(&s.current_ui)) {
        sf_init(s.term, s.sf_ctx, *sf);
        return;
    }

    if (auto* nav = std::get_if<Navigator>(&s.current_ui)) {
        if (nav->on_enter) nav->on_enter(s);
    }

    if (auto* form = std::get_if<Form>(&s.current_ui)) {
        if (form->on_enter) form->on_enter(s);
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

void ui_goto(Session& s, ActiveUI ui)
{
    s.ui_stack.clear();
    s.current_ui = std::move(ui);
    fire_on_enter(s);
}

/* ================================================================== */
/*  Sequential Form input handling                                     */
/* ================================================================== */

/* Begin gathering a field: print its prompt */
static void begin_field(Session& s, Form& form)
{
    if (s.current_field_index >= (int)form.fields.size()) {
        /* All fields gathered — submit */
        s.form_state.completed = true;
        form_exit(s, form.exit);
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
            s.form_state.completed = false;
            form_exit(s, form.exit);
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
            s.form_state.completed = false;
            form_exit(s, form.exit);
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
    if (auto* sf = std::get_if<ScreenForm>(&s.current_ui)) {
        KeyEvent ev = sf_translate_key(s.term, ch);
        if (ev.key != Key::None)
            sf_dispatch(s.term, s.sf_ctx, *sf, ev);
    } else if (auto* nav = std::get_if<Navigator>(&s.current_ui)) {
        char upper = (char)std::toupper(ch);
        for (auto& action : nav->actions) {
            if (std::toupper(action.key) == upper) {
                action.handler(s);
                return;
            }
        }
        /* Catch-all: '\0' key matches anything */
        for (auto& action : nav->actions) {
            if (action.key == '\0') {
                action.handler(s);
                return;
            }
        }
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

    if (!have_console) {
        std::fprintf(stderr, "%s: listening on port %d (no console, kill to stop)\n",
                     config.banner ? config.banner : "uitest", config.listen_port);
    }

    /*
     * Layout: last row = status bar, everything above = session mirror.
     * Console Terminal keeps ncurses ownership at all times.
     * Session Terminals write to TCP + their own screen buffer; no ncurses.
     * The sysop views one session at a time via [ and ] keys.
     *
     * Use actual terminal height so the status bar lands on the visible
     * last row regardless of terminal size (no hardcoded 25).
     */
    int console_rows = have_console ? console.ncLines() : 25;
    if (console_rows < 2) console_rows = 2;
    const int MIRROR_ROWS = console_rows - 1;   /* rows 0 .. MIRROR_ROWS-1 */
    const int STATUS_ROW  = console_rows - 1;   /* last row */

    std::vector<Session*> sessions;
    int next_session_id = 1;
    bool running = true;
    int viewed_id = -1;   /* ID of session shown in mirror area; -1 = none */
    bool need_refresh = true;

    /* -- Draw status bar at STATUS_ROW -- */
    auto draw_status = [&]() {
        if (!have_console) return;
        char buf[128];
        if (sessions.empty()) {
            std::snprintf(buf, sizeof(buf), " %s | No sessions | Q quit",
                          config.banner ? config.banner : "uitest");
        } else {
            /* Build session indicator strip: [1*][2][3] */
            char inds[64] = {};
            for (auto* s : sessions) {
                char ind[10];
                std::snprintf(ind, sizeof(ind), "[%d%s]",
                              s->id, s->id == viewed_id ? "*" : "");
                std::strncat(inds, ind, sizeof(inds) - std::strlen(inds) - 1);
            }
            int n = (int)sessions.size();
            std::snprintf(buf, sizeof(buf), " %s  %d session%s  [ prev  ] next  Q quit",
                          inds, n, n == 1 ? "" : "s");
        }
        /* Pad/truncate to exactly 80 columns */
        int len = (int)std::strlen(buf);
        if (len < 80) std::memset(buf + len, ' ', 80 - len);
        buf[80] = '\0';
        console.drawStatusLine(STATUS_ROW, buf, 0x17);  /* white on blue */
    };

    /* -- Render viewed session buffer into mirror area, then redraw status -- */
    auto render_view = [&]() {
        if (!have_console) return;
        Session* vs = nullptr;
        for (auto* s : sessions)
            if (s->id == viewed_id) { vs = s; break; }
        if (!vs)
            console.clearArea(0, MIRROR_ROWS);
        else
            console.renderBuffer(vs->term.screenBuffer(), 0, MIRROR_ROWS);
        draw_status();
    };

    render_view();  /* initial draw: blank mirror + status bar */

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

        /* Slots 2+: active session fds */
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
            } else if (ch == '[') {
                /* Previous session */
                if (!sessions.empty()) {
                    int idx = (int)sessions.size() - 1;
                    for (int i = 0; i < (int)sessions.size(); i++) {
                        if (sessions[i]->id == viewed_id) {
                            idx = (i == 0) ? (int)sessions.size() - 1 : i - 1;
                            break;
                        }
                    }
                    viewed_id = sessions[idx]->id;
                    need_refresh = true;
                }
            } else if (ch == ']') {
                /* Next session */
                if (!sessions.empty()) {
                    int idx = 0;
                    for (int i = 0; i < (int)sessions.size(); i++) {
                        if (sessions[i]->id == viewed_id) {
                            idx = (i + 1 < (int)sessions.size()) ? i + 1 : 0;
                            break;
                        }
                    }
                    viewed_id = sessions[idx]->id;
                    need_refresh = true;
                }
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
                    /* Auto-view first session */
                    if (viewed_id == -1)
                        viewed_id = s->id;
                    need_refresh = true;
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
                /* remoteGetKey returns 0 and detaches on EOF */
                if (!s->term.remoteConnected())
                    s->active = false;
                continue;
            }
            dispatch_input(*s, ch);
            /* If sysop is watching this session, refresh the mirror */
            if (s->id == viewed_id)
                need_refresh = true;
        }

        /* -- Reap dead sessions -- */
        for (auto it = sessions.begin(); it != sessions.end(); ) {
            Session* s = *it;
            if (!s->active) {
                /* If this was the viewed session, pick a neighbor */
                if (s->id == viewed_id) {
                    auto nxt = std::next(it);
                    if (nxt != sessions.end())
                        viewed_id = (*nxt)->id;
                    else if (it != sessions.begin())
                        viewed_id = (*std::prev(it))->id;
                    else
                        viewed_id = -1;
                    need_refresh = true;
                }
                s->term.sendTerminalRestore();
                s->term.closeRemote();
                delete s;
                it = sessions.erase(it);
            } else {
                ++it;
            }
        }

        /* -- Refresh console display -- */
        if (need_refresh) {
            render_view();
            need_refresh = false;
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
