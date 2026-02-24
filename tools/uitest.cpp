/*
 * uitest.cpp — Toy BBS exercising the declarative UI harness
 *
 * Demonstrates: Navigator with sub-navigation, Form with text input,
 * form submit → return to navigator.  Multi-session over telnet.
 * ZERO BBS DEPENDENCIES.
 *
 * Build:  make build/uitest
 * Run:    build/uitest            (local-only — not very useful)
 *         build/uitest -P2024     (telnet on port 2024)
 */

#include "ui.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

/* ================================================================== */
/*  Color palette — matches BBS defaults                               */
/* ================================================================== */

static const unsigned char palette[] = {
    0x07,   /* 0: default text */
    0x0B,   /* 1: highlight — bright cyan */
    0x0E,   /* 2: input — yellow */
    0x05,   /* 3: YN prompt — magenta */
    0x1F,   /* 4: prompt — bright white on blue */
    0x02,   /* 5: info — green */
    0x0C,   /* 6: warning — bright red */
    0x09,   /* 7: title — bright blue */
    0x06,   /* 8: misc — brown */
    0x03,   /* 9: misc2 — cyan */
};

static void ansic(Terminal &t, int n)
{
    if (n >= 0 && n <= 9) t.setAttr(palette[n]);
}

static void prt(Terminal &t, int color, const char *s)
{
    ansic(t, color);
    t.puts(s);
    ansic(t, 0);
}

/* ================================================================== */
/*  Messages sub-navigator                                             */
/* ================================================================== */

static Navigator make_messages_nav()
{
    Navigator nav;
    nav.id = "messages";

    nav.on_enter = [](Session& s) {
        s.term.newline();
        prt(s.term, 7, "== Message Board ==");
        s.term.newline();
        s.term.newline();

        ansic(s.term, 3); s.term.putch('[');
        ansic(s.term, 7); s.term.putch('R');
        ansic(s.term, 3); s.term.puts("] ");
        ansic(s.term, 2); s.term.puts("Read Messages");
        s.term.newline();

        ansic(s.term, 3); s.term.putch('[');
        ansic(s.term, 7); s.term.putch('P');
        ansic(s.term, 3); s.term.puts("] ");
        ansic(s.term, 2); s.term.puts("Post Message");
        s.term.newline();

        ansic(s.term, 3); s.term.putch('[');
        ansic(s.term, 7); s.term.putch('Q');
        ansic(s.term, 3); s.term.puts("] ");
        ansic(s.term, 2); s.term.puts("Back to Main");
        s.term.newline();

        s.term.newline();
        prt(s.term, 3, "Messages: ");
    };

    nav.actions = {
        {'R', "Read", [](Session& s) {
            s.term.newline();
            s.term.newline();
            prt(s.term, 5, "Message #1");
            s.term.newline();
            ansic(s.term, 1);
            s.term.puts("From: SYSOP");
            s.term.newline();
            ansic(s.term, 1);
            s.term.puts("Subj: Welcome!");
            s.term.newline();
            s.term.newline();
            ansic(s.term, 0);
            s.term.puts("Welcome to the Dominion BBS UI test.");
            s.term.newline();
            s.term.puts("This is a fake message to exercise the navigator.");
            s.term.newline();
            s.term.newline();
            prt(s.term, 3, "Messages: ");
        }},
        {'P', "Post", [](Session& s) {
            s.term.newline();
            prt(s.term, 6, "Post not implemented in this demo.");
            s.term.newline();
            s.term.newline();
            prt(s.term, 3, "Messages: ");
        }},
        {'Q', "Back", [](Session& s) {
            ui_pop(s);
        }},
    };

    return nav;
}

/* ================================================================== */
/*  Guestbook form                                                     */
/* ================================================================== */

static Form make_guestbook_form()
{
    Form form;
    form.id = "guestbook";

    form.on_enter = [](Session& s) {
        s.term.newline();
        prt(s.term, 7, "== Guestbook ==");
        s.term.newline();
        ansic(s.term, 0);
        s.term.puts("Fill out the form below (ESC to cancel).");
        s.term.newline();
        s.term.newline();
    };

    form.fields = {
        {"name",    InputKind::TextInput, "Your name: ",      30, true, "", true},
        {"city",    InputKind::TextInput, "Your city: ",      30, true, "", true},
        {"message", InputKind::TextInput, "Leave a message: ", 60, true, "", true},
    };

    form.on_submit = [](Session& s, const FormResult& r) {
        s.term.newline();
        prt(s.term, 5, "Thanks ");
        ansic(s.term, 1);
        s.term.puts(r.values.at("name").c_str());
        prt(s.term, 5, " from ");
        ansic(s.term, 1);
        s.term.puts(r.values.at("city").c_str());
        prt(s.term, 5, "!");
        s.term.newline();
        ansic(s.term, 0);
        s.term.puts("You said: ");
        ansic(s.term, 2);
        s.term.puts(r.values.at("message").c_str());
        s.term.newline();
        s.term.newline();
        ui_pop(s);
    };

    form.on_abort = [](Session& s) {
        s.term.newline();
        prt(s.term, 6, "Guestbook entry cancelled.");
        s.term.newline();
        s.term.newline();
        ui_pop(s);
    };

    return form;
}

/* ================================================================== */
/*  Color test — immediate action                                      */
/* ================================================================== */

static void show_colors(Session& s)
{
    s.term.newline();
    prt(s.term, 7, "== Color Test ==");
    s.term.newline();
    s.term.newline();

    for (int i = 0; i <= 9; i++) {
        ansic(s.term, i);
        s.term.printf("Color %d: The quick brown fox", i);
        s.term.newline();
    }
    ansic(s.term, 0);
    s.term.newline();
}

/* ================================================================== */
/*  Main menu navigator                                                */
/* ================================================================== */

static Navigator make_main_menu()
{
    Navigator nav;
    nav.id = "main";

    nav.on_enter = [](Session& s) {
        s.term.newline();
        prt(s.term, 7, "== Dominion UI Test ==");
        s.term.newline();
        s.term.newline();

        struct { char key; const char *label; } items[] = {
            {'M', "Messages"},
            {'G', "Guestbook"},
            {'C', "Colors"},
            {'Q', "Quit"},
        };

        for (int i = 0; i < 4; i++) {
            ansic(s.term, 3); s.term.putch('[');
            ansic(s.term, 7); s.term.putch(items[i].key);
            ansic(s.term, 3); s.term.puts("] ");
            ansic(s.term, 2); s.term.puts(items[i].label);
            s.term.newline();
        }

        s.term.newline();
        prt(s.term, 3, "Main: ");
    };

    nav.actions = {
        {'M', "Messages", [](Session& s) {
            ui_push(s, make_messages_nav());
        }},
        {'G', "Guestbook", [](Session& s) {
            ui_push(s, make_guestbook_form());
        }},
        {'C', "Colors", [](Session& s) {
            show_colors(s);
            /* Re-show main menu prompt */
            prt(s.term, 3, "Main: ");
        }},
        {'Q', "Quit", [](Session& s) {
            s.term.newline();
            prt(s.term, 5, "Goodbye!");
            s.term.newline();
            ui_quit(s);
        }},
    };

    return nav;
}

/* ================================================================== */
/*  main                                                               */
/* ================================================================== */

int main(int argc, char *argv[])
{
    int port = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'P')
            port = std::atoi(&argv[i][2]);
    }

    UIConfig config;
    config.listen_port = port;
    config.banner = "uitest";
    config.on_connect = [](Session& s) -> ActiveUI {
        return make_main_menu();
    };

    ui_run(config);
    return 0;
}
