/*
 * formtest.cpp — Standalone test for fullscreen positioned forms
 *
 * Exercises ScreenForm: background ANSI art, positioned entry fields,
 * tab navigation, field-level validation.  Mimics the BBS new-user
 * registration form layout.
 *
 * ZERO BBS DEPENDENCIES — links only ui.o + screen_form.o + terminal.o.
 *
 * Build:  make build/formtest
 * Run:    build/formtest -P2024
 *         telnet localhost 2024
 */

#include "ui.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

/* ================================================================== */
/*  Form definition                                                    */
/* ================================================================== */

/*
 * Field positions match the newans.ans ANSI art layout.
 * All coordinates are 0-based (col, row) for Terminal::gotoXY.
 *
 * The original newuser.cpp uses go(row_1based, col_1based) which maps
 * to gotoXY(col-1, row-1).  So goin(7,18) → gotoXY(17, 6).
 *
 * Command line ("gotop") is at row 2, col 4 → gotoXY(3, 1).
 */

static std::string g_datadir;  /* path to afiles/ directory */

static ScreenForm make_newuser_form(Session* sp)
{
    ScreenForm form;
    form.id = "newuser";
    form.background_file = g_datadir + "/afiles/newans.ans";
    form.cmd_row = 1;
    form.cmd_col = 3;
    form.cmd_width = 60;

    /* --- Name --- */
    {
        ScreenField f;
        f.name = "name";
        f.widget = TextField{.max_chars = 30};
        f.row = 6; f.col = 17; f.width = 30;
        f.required = true;
        f.prompt = "Enter your handle or real name:";
        f.validate = [](const std::string& v) {
            if (v.empty()) return false;
            if (v[0] < 'A') return false;           /* must start with letter */
            if (v.back() == ' ') return false;       /* no trailing space */
            return true;
        };
        form.fields.push_back(f);
    }

    /* --- Real Name --- */
    {
        ScreenField f;
        f.name = "realname";
        f.widget = TextField{.max_chars = 20};
        f.row = 7; f.col = 17; f.width = 20;
        f.required = true;
        f.prompt = "Enter your real name:";
        form.fields.push_back(f);
    }

    /* --- Sex --- */
    {
        ScreenField f;
        f.name = "sex";
        f.widget = SelectField{
            .options = {{'M', "Male"}, {'F', "Female"}, {'Y', "Yes"}, {'L', "Lots"}},
        };
        f.row = 8; f.col = 17; f.width = 10;
        f.prompt = "Sex: (M)ale (F)emale (Y)es (L)ots";
        form.fields.push_back(f);
    }

    /* --- Birthdate --- */
    {
        ScreenField f;
        f.name = "birthdate";
        f.widget = DateField{.format = DateFormat::MonthDayFullYear};
        f.row = 9; f.col = 17; f.width = 12;
        f.required = true;
        f.prompt = "Enter birthdate (MM/DD/YYYY):";
        /* No external validator needed — Date widget validates segments */
        form.fields.push_back(f);
    }

    /* --- Phone --- */
    {
        ScreenField f;
        f.name = "phone";
        f.widget = PhoneField{};
        f.row = 10; f.col = 17; f.width = 14;
        f.required = true;
        f.prompt = "Enter voice phone (XXX-XXX-XXXX):";
        f.validate = [](const std::string& digits) {
            return digits.size() == 10;
        };
        form.fields.push_back(f);
    }

    /* --- Street --- */
    {
        ScreenField f;
        f.name = "street";
        f.widget = TextField{.max_chars = 35};
        f.row = 11; f.col = 17; f.width = 35;
        f.required = true;
        f.prompt = "Enter your street address:";
        form.fields.push_back(f);
    }

    /* --- City --- */
    {
        ScreenField f;
        f.name = "city";
        f.widget = TextField{.max_chars = 35};
        f.row = 12; f.col = 17; f.width = 35;
        f.required = true;
        f.prompt = "Enter your city and state:";
        form.fields.push_back(f);
    }

    /* --- Comment --- */
    {
        ScreenField f;
        f.name = "comment";
        f.widget = TextField{.max_chars = 35};
        f.row = 13; f.col = 17; f.width = 35;
        f.required = false;
        f.prompt = "Enter a comment (optional):";
        form.fields.push_back(f);
    }

    /* --- Computer Type --- */
    {
        ScreenField f;
        f.name = "comptype";
        f.widget = SelectField{
            .options = {
                {'1', "IBM Compatible"},
                {'2', "Macintosh"},
                {'3', "Amiga"},
                {'4', "Atari ST"},
                {'5', "Linux/Unix"},
                {'6', "Other"},
            },
            .display = SelectDisplay::Popup,
            .popup_row = 7, .popup_col = 56,
        };
        f.row = 14; f.col = 17; f.width = 20;
        f.prompt = "Enter your computer type";
        form.fields.push_back(f);
    }

    /* --- Password --- */
    {
        ScreenField f;
        f.name = "password";
        f.widget = TextField{.max_chars = 20, .masked = true};
        f.row = 15; f.col = 17; f.width = 20;
        f.required = true;
        f.prompt = "Enter a password (3-20 chars):";
        f.validate = [](const std::string& v) {
            return v.size() >= 3;
        };
        form.fields.push_back(f);
    }

    /* --- Submit/Cancel --- */
    form.on_submit = [sp](Terminal& term, SFContext& /*ctx*/, const FormResult& r) {
        /* Screen already cleared by FormExit::Clear */
        term.setAttr(0x0B);
        term.puts("=== New User Registration Complete ===");
        term.newline();
        term.newline();

        const char* labels[] = {
            "Name", "Real Name", "Sex", "Birthdate", "Phone",
            "Street", "City", "Comment", "Computer", "Password"
        };
        const char* keys[] = {
            "name", "realname", "sex", "birthdate", "phone",
            "street", "city", "comment", "comptype", "password"
        };
        for (int i = 0; i < 10; i++) {
            term.setAttr(0x03);
            term.printf("%-12s", labels[i]);
            term.setAttr(0x0F);
            auto it = r.values.find(keys[i]);
            if (it != r.values.end())
                term.puts(it->second.c_str());
            term.newline();
        }

        term.newline();
        term.setAttr(0x0E);
        term.puts("Press Q to disconnect.");
        term.newline();

        /* Non-blocking: push a dismiss navigator instead of blocking on getKey() */
        Navigator done;
        done.actions = {{'Q', "Quit", [](Session& s) { ui_quit(s); }}};
        ui_push(*sp, done);
    };

    form.on_cancel = [sp](Terminal& term, SFContext& /*ctx*/) {
        /* Screen already cleared by FormExit::Clear */
        term.setAttr(0x0C);
        term.puts("Registration cancelled.");
        term.newline();
        term.newline();
        term.setAttr(0x0E);
        term.puts("Press Q to disconnect.");
        term.newline();

        Navigator done;
        done.actions = {{'Q', "Quit", [](Session& s) { ui_quit(s); }}};
        ui_push(*sp, done);
    };

    return form;
}

/* ================================================================== */
/*  main                                                               */
/* ================================================================== */

int main(int argc, char *argv[])
{
    int port = 0;
    g_datadir = ".";

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'P')
            port = std::atoi(&argv[i][2]);
        else if (argv[i][0] == '-' && argv[i][1] == 'd')
            g_datadir = &argv[i][2];
    }

    if (port == 0) {
        std::fprintf(stderr, "Usage: formtest -P<port> [-d<datadir>]\n");
        std::fprintf(stderr, "  -P<port>     TCP port to listen on\n");
        std::fprintf(stderr, "  -d<dir>      Data directory (contains afiles/)\n");
        std::fprintf(stderr, "\nExample: formtest -P2024 -dbuild\n");
        return 1;
    }

    UIConfig config;
    config.listen_port = port;
    config.banner = "formtest";
    config.on_connect = [](Session& s) -> ActiveUI {
        return make_newuser_form(&s);
    };

    ui_run(config);
    return 0;
}
