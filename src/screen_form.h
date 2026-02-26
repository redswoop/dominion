/*
 * screen_form.h — Positioned + sequential form engine
 *
 * Canonical ScreenForm library used by both the BBS and standalone tools.
 * Operates on Terminal& + SFContext& — no BBS globals, no Session singleton.
 *
 * Two form modes:
 *   - Screen mode: ANSI background art with positioned fields (newuser reg)
 *   - Sequential mode: prompt-by-prompt inline input (no background needed)
 *
 * Two usage patterns:
 *   - sf_run()  — synchronous: init + dispatch loop (BBS newuser.cpp)
 *   - sf_init() + sf_dispatch() — event-driven: caller owns the loop (ui.cpp)
 *
 * Single-field convenience:
 *   - sf_prompt() — runs a 1-field sequential form inline. Drop-in replacement
 *     for input(), onek(), inputdate(), etc.
 *
 * Callbacks take (Terminal&, SFContext&, ...).  Callers that need additional
 * context (e.g. ui.h Session) capture it in the lambda closure.
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
    std::function<bool(const FormResult&)> when;  /* conditional visibility */
    WidgetConfig widget;
};

/* Forward declaration for callbacks */
struct SFContext;

/* --- Form mode --- */

enum class FormMode {
    Auto,        /* sequential if no background_file, else screen */
    Screen,      /* fullscreen positioned fields */
    Sequential,  /* prompt-by-prompt inline */
};

/* --- Screen form --- */

struct ScreenForm {
    std::string id;
    std::string background_file;      /* .ans file path */
    FormExit exit = FormExit::Clear;  /* default: clear before callbacks */
    int cmd_row = 0, cmd_col = 0;     /* command line position (0-based) */
    int cmd_width = 60;               /* command line clear width */
    int twoline_field = 0;            /* field to jump to on Tab from cmd line */
    FormMode mode = FormMode::Auto;
    unsigned char prompt_attr = 0x0E; /* yellow prompt text (sequential) */
    unsigned char input_attr  = 0x0F; /* bright white input (sequential) */
    std::function<void(Terminal&)> on_enter;  /* called after clear, before first field */
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
    bool sequential = false;         /* set by sf_run/sf_init based on mode */
    int  inline_start_x = 0;        /* cursor X where field input begins (seq) */
    int  inline_start_y = 0;        /* cursor Y where field input begins (seq) */
};

/* --- Public API --- */

/* Run the form synchronously.  Returns true if submitted, false if cancelled. */
bool     sf_run(Terminal& term, SFContext& ctx, ScreenForm& form);

/* Translate a raw byte (+ lookahead for ESC sequences) to a logical key */
KeyEvent sf_translate_key(Terminal& term, unsigned char ch);

/* Initialize form: clear screen, send background, focus first field */
void     sf_init(Terminal& term, SFContext& ctx, ScreenForm& form);

/* Dispatch a single key event to the focused field or command line */
void     sf_dispatch(Terminal& term, SFContext& ctx, ScreenForm& form, KeyEvent ev);

/* Prompt for a single field value inline.  Runs a 1-field sequential form.
 * Returns the collected value (empty string if cancelled/hangup). */
std::string sf_prompt(Terminal& term, SFContext& ctx, ScreenField& field);

#endif /* SCREEN_FORM_H_ */
