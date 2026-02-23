/*
 * ui.h — Declarative UI types and event loop
 *
 * LEAF COMPONENT: depends on terminal.h and nothing else.
 * No BBS headers, no vardec.h, no vars.h, no globals.
 *
 * Core types: Session, Navigator, Form, NavAction, FormField.
 * Event loop: poll-based multi-session dispatcher.
 *
 * Build: linked by tools/uitest.cpp (zero BBS dependencies)
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

/* --- Form --- */

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
    bool completed;               /* true=submit, false=abort */
};

struct Form {
    std::string id;
    std::function<void(Session&)> on_enter;   /* called when form becomes active */
    std::vector<FormField> fields;
    std::function<void(Session&, const FormResult&)> on_submit;
    std::function<void(Session&)> on_abort;
};

/* --- Session --- */

using ActiveUI = std::variant<Navigator, Form>;

struct Session {
    int id;
    int fd;                       /* TCP socket */
    Terminal term;                 /* each session owns a Terminal instance */

    /* UI state */
    ActiveUI current_ui;
    std::vector<ActiveUI> ui_stack;

    /* Form input state */
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
    SessionInitFn on_connect;     /* builds initial navigator for new session */
};

/* Main entry point — runs the event loop, returns on shutdown */
void ui_run(const UIConfig& config);

/* Navigation helpers (for use in action handlers) */
void ui_push(Session& s, ActiveUI ui);
void ui_pop(Session& s);
void ui_quit(Session& s);

#endif /* UI_H_ */
