/*
 * formtest.cpp — Standalone test for ScreenForm: screen + sequential + branching
 *
 * Exercises all ScreenForm modes: fullscreen positioned fields (screen),
 * prompt-by-prompt inline input (sequential), and conditional branching.
 *
 * ZERO BBS DEPENDENCIES — links only ui.o + screen_form.o + terminal.o.
 *
 * Build:  make build/formtest
 * Run:    build/formtest -dbuild
 *         telnet localhost 2024
 *
 *         build/formtest -dbuild seq      (direct launch sequential demo)
 *         build/formtest -dbuild branch   (direct launch branching demo)
 */

#include "ui.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static std::string g_datadir;  /* path to afiles/ directory */

/* ================================================================== */
/*  Demo 1: Screen Form (new user registration)                        */
/* ================================================================== */

static ScreenForm make_screen_demo(Session* sp)
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
            if (v[0] < 'A') return false;
            if (v.back() == ' ') return false;
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
        term.puts("Press Q to return.");
        term.newline();

        Navigator done;
        done.actions = {{'Q', "Return", [](Session& s) { ui_pop(s); ui_pop(s); }}};
        ui_push(*sp, done);
    };

    form.on_cancel = [sp](Terminal& term, SFContext& /*ctx*/) {
        term.setAttr(0x0C);
        term.puts("Registration cancelled.");
        term.newline();
        term.newline();
        term.setAttr(0x0E);
        term.puts("Press Q to return.");
        term.newline();

        Navigator done;
        done.actions = {{'Q', "Return", [](Session& s) { ui_pop(s); ui_pop(s); }}};
        ui_push(*sp, done);
    };

    return form;
}

/* ================================================================== */
/*  Demo 2: Sequential Form (all widget types)                         */
/* ================================================================== */

static ScreenForm make_sequential_demo(Session* sp)
{
    ScreenForm form;
    form.id = "seq_demo";
    form.mode = FormMode::Sequential;
    form.exit = FormExit::None;

    /* TextField */
    {
        ScreenField f;
        f.name = "name";
        f.widget = TextField{.max_chars = 30};
        f.width = 30;
        f.required = true;
        f.prompt = "Your name:";
        form.fields.push_back(f);
    }

    /* SelectField */
    {
        ScreenField f;
        f.name = "color";
        f.widget = SelectField{
            .options = {{'R', "Red"}, {'G', "Green"}, {'B', "Blue"}},
        };
        f.width = 10;
        f.prompt = "Favorite color:";
        form.fields.push_back(f);
    }

    /* DateField */
    {
        ScreenField f;
        f.name = "birthdate";
        f.widget = DateField{.format = DateFormat::MonthDayFullYear};
        f.width = 12;
        f.required = true;
        f.prompt = "Birthdate (MM/DD/YYYY):";
        form.fields.push_back(f);
    }

    /* PhoneField */
    {
        ScreenField f;
        f.name = "phone";
        f.widget = PhoneField{};
        f.width = 14;
        f.required = true;
        f.prompt = "Phone (XXX-XXX-XXXX):";
        f.validate = [](const std::string& digits) {
            return digits.size() == 10;
        };
        form.fields.push_back(f);
    }

    /* TextField masked (password) */
    {
        ScreenField f;
        f.name = "password";
        f.widget = TextField{.max_chars = 20, .masked = true};
        f.width = 20;
        f.required = true;
        f.prompt = "Password (3+ chars):";
        f.validate = [](const std::string& v) {
            return v.size() >= 3;
        };
        form.fields.push_back(f);
    }

    form.on_submit = [sp](Terminal& term, SFContext& /*ctx*/, const FormResult& r) {
        term.newline();
        term.setAttr(0x0B);
        term.puts("=== Sequential Form Results ===");
        term.newline();
        term.newline();

        const char* labels[] = {"Name", "Color", "Birthdate", "Phone", "Password"};
        const char* keys[]   = {"name", "color", "birthdate", "phone", "password"};
        for (int i = 0; i < 5; i++) {
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
        term.puts("Press Q to return.");
        term.newline();

        Navigator done;
        done.actions = {{'Q', "Return", [](Session& s) { ui_pop(s); ui_pop(s); }}};
        ui_push(*sp, done);
    };

    form.on_cancel = [sp](Terminal& term, SFContext& /*ctx*/) {
        term.newline();
        term.setAttr(0x0C);
        term.puts("Cancelled.");
        term.newline();
        term.newline();
        term.setAttr(0x0E);
        term.puts("Press Q to return.");
        term.newline();

        Navigator done;
        done.actions = {{'Q', "Return", [](Session& s) { ui_pop(s); ui_pop(s); }}};
        ui_push(*sp, done);
    };

    return form;
}

/* ================================================================== */
/*  Demo 3: Conditional Branching (file search)                        */
/* ================================================================== */

static ScreenForm make_branching_demo(Session* sp)
{
    ScreenForm form;
    form.id = "branch_demo";
    form.mode = FormMode::Sequential;
    form.exit = FormExit::None;

    /* Search type selector */
    {
        ScreenField f;
        f.name = "search_type";
        f.widget = SelectField{
            .options = {{'U', "Uploader"}, {'D', "Date"}, {'K', "Keyword"}},
        };
        f.width = 15;
        f.required = true;
        f.prompt = "Search by:";
        form.fields.push_back(f);
    }

    /* Uploader name — only when search_type == U */
    {
        ScreenField f;
        f.name = "uploader";
        f.widget = TextField{.max_chars = 30};
        f.width = 30;
        f.required = true;
        f.prompt = "Uploader name:";
        f.when = [](const FormResult& r) {
            auto it = r.values.find("search_type");
            return it != r.values.end() && it->second == "U";
        };
        form.fields.push_back(f);
    }

    /* Since date — only when search_type == D */
    {
        ScreenField f;
        f.name = "since_date";
        f.widget = DateField{.format = DateFormat::MonthDayFullYear};
        f.width = 12;
        f.required = true;
        f.prompt = "Files since (MM/DD/YYYY):";
        f.when = [](const FormResult& r) {
            auto it = r.values.find("search_type");
            return it != r.values.end() && it->second == "D";
        };
        form.fields.push_back(f);
    }

    /* Keyword — only when search_type == K */
    {
        ScreenField f;
        f.name = "keyword";
        f.widget = TextField{.max_chars = 40};
        f.width = 40;
        f.required = true;
        f.prompt = "Search keyword:";
        f.when = [](const FormResult& r) {
            auto it = r.values.find("search_type");
            return it != r.values.end() && it->second == "K";
        };
        form.fields.push_back(f);
    }

    form.on_submit = [sp](Terminal& term, SFContext& /*ctx*/, const FormResult& r) {
        term.newline();
        term.setAttr(0x0B);
        term.puts("=== File Search Parameters ===");
        term.newline();
        term.newline();

        auto st = r.values.find("search_type");
        if (st != r.values.end()) {
            term.setAttr(0x03);
            term.puts("Search type: ");
            term.setAttr(0x0F);
            if (st->second == "U") {
                term.puts("Uploader");
                term.newline();
                term.setAttr(0x03);
                term.puts("Uploader:    ");
                term.setAttr(0x0F);
                auto v = r.values.find("uploader");
                if (v != r.values.end()) term.puts(v->second.c_str());
            } else if (st->second == "D") {
                term.puts("Date");
                term.newline();
                term.setAttr(0x03);
                term.puts("Since:       ");
                term.setAttr(0x0F);
                auto v = r.values.find("since_date");
                if (v != r.values.end()) term.puts(v->second.c_str());
            } else if (st->second == "K") {
                term.puts("Keyword");
                term.newline();
                term.setAttr(0x03);
                term.puts("Keyword:     ");
                term.setAttr(0x0F);
                auto v = r.values.find("keyword");
                if (v != r.values.end()) term.puts(v->second.c_str());
            }
            term.newline();
        }

        term.newline();
        term.setAttr(0x0E);
        term.puts("Press Q to return.");
        term.newline();

        Navigator done;
        done.actions = {{'Q', "Return", [](Session& s) { ui_pop(s); ui_pop(s); }}};
        ui_push(*sp, done);
    };

    form.on_cancel = [sp](Terminal& term, SFContext& /*ctx*/) {
        term.newline();
        term.setAttr(0x0C);
        term.puts("Search cancelled.");
        term.newline();
        term.newline();
        term.setAttr(0x0E);
        term.puts("Press Q to return.");
        term.newline();

        Navigator done;
        done.actions = {{'Q', "Return", [](Session& s) { ui_pop(s); ui_pop(s); }}};
        ui_push(*sp, done);
    };

    return form;
}

/* ================================================================== */
/*  Main menu                                                          */
/* ================================================================== */

static Navigator make_main_menu(Session* sp)
{
    Navigator nav;
    nav.id = "main";
    nav.on_enter = [](Session& s) {
        s.term.clearScreen();
        s.term.gotoXY(0, 0);
        s.term.setAttr(0x0B);
        s.term.puts("== ScreenForm Test ==");
        s.term.newline();
        s.term.newline();
        s.term.setAttr(0x0E);
        s.term.puts("[1] ");
        s.term.setAttr(0x07);
        s.term.puts("Screen Form (new user registration)");
        s.term.newline();
        s.term.setAttr(0x0E);
        s.term.puts("[2] ");
        s.term.setAttr(0x07);
        s.term.puts("Sequential Form (all widget types)");
        s.term.newline();
        s.term.setAttr(0x0E);
        s.term.puts("[3] ");
        s.term.setAttr(0x07);
        s.term.puts("Conditional Branching (file search)");
        s.term.newline();
        s.term.setAttr(0x0E);
        s.term.puts("[Q] ");
        s.term.setAttr(0x07);
        s.term.puts("Quit");
        s.term.newline();
        s.term.newline();
        s.term.setAttr(0x03);
        s.term.puts("Choice: ");
    };
    nav.actions = {
        {'1', "Screen", [sp](Session& s) {
            ui_push(s, make_screen_demo(sp));
        }},
        {'2', "Sequential", [sp](Session& s) {
            ui_push(s, make_sequential_demo(sp));
        }},
        {'3', "Branching", [sp](Session& s) {
            ui_push(s, make_branching_demo(sp));
        }},
        {'Q', "Quit", [](Session& s) { ui_quit(s); }},
    };
    return nav;
}

/* ================================================================== */
/*  main                                                               */
/* ================================================================== */

int main(int argc, char *argv[])
{
    int port = 2024;
    g_datadir = ".";
    const char* test_name = nullptr;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'P')
            port = std::atoi(&argv[i][2]);
        else if (argv[i][0] == '-' && argv[i][1] == 'd')
            g_datadir = &argv[i][2];
        else if (argv[i][0] != '-')
            test_name = argv[i];
    }

    UIConfig config;
    config.listen_port = port;
    config.banner = "formtest";

    if (test_name && std::strcmp(test_name, "screen") == 0) {
        config.on_connect = [](Session& s) -> ActiveUI {
            return make_screen_demo(&s);
        };
    } else if (test_name && std::strcmp(test_name, "seq") == 0) {
        config.on_connect = [](Session& s) -> ActiveUI {
            return make_sequential_demo(&s);
        };
    } else if (test_name && std::strcmp(test_name, "branch") == 0) {
        config.on_connect = [](Session& s) -> ActiveUI {
            return make_branching_demo(&s);
        };
    } else {
        /* Interactive menu */
        config.on_connect = [](Session& s) -> ActiveUI {
            return make_main_menu(&s);
        };
    }

    ui_run(config);
    return 0;
}
