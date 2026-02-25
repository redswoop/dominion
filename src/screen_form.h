/*
 * screen_form.h — Fullscreen positioned form engine (BBS integration layer)
 *
 * Engine types and API extracted from ui.h/ui.cpp for direct BBS use.
 * Operates on Terminal& + SFContext& — no BBS globals, no Session singleton.
 *
 * Types are compatible with but separate from ui.h's ScreenForm types.
 * Deduplication is future work.
 *
 * Key differences from ui.h:
 *   - Callbacks take (Terminal&, SFContext&, ...) not (Session&, ...)
 *   - SelectField stores key char in FormResult (not label)
 *   - SFContext carries hangup/echo pointers for BBS integration
 *   - sf_run() is the entry point (no event loop, no multi-session)
 */

#ifndef SCREEN_FORM_H_
#define SCREEN_FORM_H_

#include "terminal.h"
#include <string>
#include <vector>
#include <functional>
#include <variant>
#include <map>

/* --- Logical Keys --- */

enum class Key {
    None = 0,
    Char,
    Enter,
    Tab,
    ShiftTab,
    Up,
    Down,
    Left,
    Right,
    Backspace,
    Escape,
    Home,
    End,
};

struct KeyEvent {
    Key key = Key::None;
    char ch = 0;
};

/* --- Form exit behavior --- */

enum class FormExit {
    Clear,    /* clear screen before calling on_submit/on_cancel */
    Scroll,   /* let content scroll off */
    None,     /* do nothing */
};

/* --- Form result --- */

struct FormResult {
    std::map<std::string, std::string> values;
    bool completed = false;
};

/* --- Date format --- */

enum class DateFormat {
    MonthDayFullYear,  /* MM/DD/YYYY — 8 digits */
    MonthDayYear,      /* MM/DD/YY   — 6 digits */
    MonthYear,         /* MM/YYYY    — 6 digits */
};

/* --- Select display --- */

enum class SelectDisplay {
    Inline,   /* choices shown inline in command line prompt */
    Popup,    /* choices rendered at popup_row/popup_col on focus */
};

/* --- Widget configs --- */

struct TextField {
    int max_chars = 0;    /* 0 = field width */
    bool masked = false;  /* show '*' for password fields */
};

struct SelectField {
    std::vector<std::pair<char, std::string>> options;  /* {key, label} */
    SelectDisplay display = SelectDisplay::Inline;
    int popup_row = 0, popup_col = 0;
};

struct DateField {
    DateFormat format = DateFormat::MonthDayFullYear;
};

struct PhoneField {};

using WidgetConfig = std::variant<TextField, SelectField, DateField, PhoneField>;

/* --- Screen field --- */

struct ScreenField {
    std::string name;
    int row = 0, col = 0;   /* 0-based screen position */
    int width = 20;          /* display width */
    bool required = false;
    std::string prompt;      /* instruction text shown on command line */
    std::function<bool(const std::string&)> validate;
    WidgetConfig widget;
};

/* Forward declaration for callbacks */
struct SFContext;

/* --- Screen form --- */

struct ScreenForm {
    std::string id;
    std::string background_file;      /* .ans file path */
    FormExit exit = FormExit::Clear;  /* default: clear before callbacks */
    int cmd_row = 0, cmd_col = 0;     /* command line position (0-based) */
    int cmd_width = 60;               /* command line clear width */
    int twoline_field = 0;            /* field to jump to on Tab from cmd line */
    std::vector<ScreenField> fields;
    std::function<void(Terminal&, SFContext&, const FormResult&)> on_submit;
    std::function<void(Terminal&, SFContext&)>                    on_cancel;
};

/* --- Run-time context --- */

struct SFContext {
    int *hangup = nullptr;           /* &io.hangup — sf_run exits if *hangup set */
    int *echo   = nullptr;           /* &io.echo   — 0 for masked fields, 1 otherwise */
    int  current_field_index = 0;
    bool cancelled = false;
    std::string input_buffer;
    FormResult  form_state;
};

/* --- Public API --- */

/* Run the form synchronously.  Returns true if submitted, false if cancelled. */
bool     sf_run(Terminal& term, SFContext& ctx, ScreenForm& form);

/* Translate a raw byte (+ lookahead for ESC sequences) to a logical key */
KeyEvent sf_translate_key(Terminal& term, unsigned char ch);

#endif /* SCREEN_FORM_H_ */
