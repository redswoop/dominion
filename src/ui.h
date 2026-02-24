/*
 * ui.h — Declarative UI types and event loop
 *
 * LEAF COMPONENT: depends on terminal.h and nothing else.
 * No BBS headers, no vardec.h, no vars.h, no globals.
 *
 * Core types: Session, Navigator, Form, ScreenForm, NavAction, FormField.
 * Event loop: poll-based multi-session dispatcher.
 *
 * Build: linked by tools/uitest.cpp and tools/formtest.cpp (zero BBS deps)
 */

#ifndef UI_H_
#define UI_H_

#include "terminal.h"
#include <string>
#include <vector>
#include <functional>
#include <variant>
#include <map>

struct Session;  /* forward */

/* --- Logical Keys --- */

enum class Key {
    None = 0,
    Char,           /* printable character — value in KeyEvent.ch */
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

/* Translate raw byte (+ lookahead for ESC sequences) to logical key */
KeyEvent translate_key(Session& s, unsigned char first_byte);

/* --- Interaction Primitives --- */

enum class InputKind {
    TextInput,   /* gather a string */
    Choice,      /* single keypress from a set */
    Confirm,     /* yes/no */
};

/* --- Navigator --- */

struct NavAction {
    char key;
    std::string label;
    std::function<void(Session&)> handler;
};

struct Navigator {
    std::string id;
    std::function<void(Session&)> on_enter;  /* called when navigator becomes active */
    std::vector<NavAction> actions;
};

/* --- Form exit behavior --- */

enum class FormExit {
    Clear,      /* reset attrs + clear screen + home cursor (fullscreen default) */
    Scroll,     /* let content scroll off — inline/sequential forms */
    None,       /* do nothing — consumer manages screen transition */
};

/* --- Sequential Form --- */

struct FormField {
    std::string name;
    InputKind kind;
    std::string prompt;
    int max_len = 80;
    bool required = true;
    std::string allowed_keys;     /* for Choice kind */
    bool default_yes = true;      /* for Confirm kind */
};

struct FormResult {
    std::map<std::string, std::string> values;
    bool completed = false;       /* true=submit, false=abort */
};

struct Form {
    std::string id;
    FormExit exit = FormExit::None;  /* sequential forms: usually None or Scroll */
    std::function<void(Session&)> on_enter;   /* called when form becomes active */
    std::vector<FormField> fields;
    std::function<void(Session&, const FormResult&)> on_submit;
    std::function<void(Session&)> on_abort;
};

/* --- Screen Form (Fullscreen Positioned) --- */

/* Which date components to collect.  Order is locale-determined (v1: MDY). */
enum class DateFormat {
    MonthDayFullYear,  /* MM/DD/YYYY — 3 segments, 8 digits */
    MonthDayYear,      /* MM/DD/YY   — 3 segments, 6 digits */
    MonthYear,         /* MM/YYYY    — 2 segments, 6 digits */
};

/* How a Select field renders its choices */
enum class SelectDisplay {
    Inline,     /* choices shown in command line prompt (e.g. Sex: M/F/Y/L) */
    Popup,      /* choices rendered in a list at popup_row/popup_col on focus */
};

/* Per-widget-type configuration.  Each widget carries only its own properties. */

struct TextField {
    int max_chars = 0;       /* 0 = same as field width */
    bool masked = false;     /* password: show '*' */
};

struct SelectField {
    std::vector<std::pair<char, std::string>> options;  /* {key, label} */
    SelectDisplay display = SelectDisplay::Inline;
    int popup_row = 0, popup_col = 0;  /* Popup: where to render choice list */
};

struct DateField {
    DateFormat format = DateFormat::MonthDayFullYear;
};

struct PhoneField {};

using WidgetConfig = std::variant<TextField, SelectField, DateField, PhoneField>;

struct ScreenField {
    std::string name;
    int row = 0, col = 0;            /* 0-based screen position */
    int width = 20;                   /* display width (clear area) */
    bool required = false;
    std::string prompt;               /* instruction text for command line */
    std::function<bool(const std::string&)> validate;
    WidgetConfig widget;              /* TextField by default */
};

struct ScreenForm {
    std::string id;
    std::string background_file;      /* .ans file path */
    FormExit exit = FormExit::Clear;  /* fullscreen default: clear screen on exit */
    int cmd_row = 0, cmd_col = 0;     /* command line position (0-based) */
    int cmd_width = 60;               /* command line clear width */
    int twoline_field = 0;            /* field index focused on Down/Tab from command line */
    std::vector<ScreenField> fields;
    std::function<void(Session&, const FormResult&)> on_submit;
    std::function<void(Session&)> on_cancel;
};

/* --- Session --- */

using ActiveUI = std::variant<Navigator, Form, ScreenForm>;

struct Session {
    int id;
    int fd;                       /* TCP socket */
    Terminal term;                 /* each session owns a Terminal instance */

    /* UI state */
    ActiveUI current_ui;
    std::vector<ActiveUI> ui_stack;

    /* Form input state (shared by Form and ScreenForm) */
    FormResult form_state;
    int current_field_index = 0;
    std::string input_buffer;

    /* Connection state */
    bool active = true;

    /* Push a new UI, saving current one */
    void push_ui(ActiveUI ui);
    /* Pop back to previous UI */
    void pop_ui();
};

/* --- Event Loop API --- */

/* App provides a factory that builds the initial UI for new connections */
using SessionInitFn = std::function<ActiveUI(Session&)>;

struct UIConfig {
    int listen_port = 0;          /* 0 = local only */
    const char *banner = nullptr; /* server name for console banner (e.g. "formtest") */
    SessionInitFn on_connect;     /* builds initial navigator for new session */
};

/* Main entry point — runs the event loop, returns on shutdown */
void ui_run(const UIConfig& config);

/* Navigation helpers (for use in action handlers) */
void ui_push(Session& s, ActiveUI ui);
void ui_pop(Session& s);
void ui_quit(Session& s);

#endif /* UI_H_ */
