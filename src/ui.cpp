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
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
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
/*  Key translation                                                    */
/* ================================================================== */

KeyEvent translate_key(Session& s, unsigned char ch)
{
    if (ch == 27) {
        /* ESC received — check for escape sequence.
         * Arrow keys etc. send ESC [ X in rapid succession.
         * A lone ESC press has no following bytes. */
        usleep(20000);  /* 20ms for sequence bytes to arrive */
        if (s.term.remoteDataReady()) {
            unsigned char ch2 = s.term.remoteGetKey();
            if (ch2 == '[') {
                /* CSI sequence — read the final byte */
                unsigned char ch3 = 0;
                /* Wait briefly for the sequence byte */
                for (int i = 0; i < 5 && !ch3; i++) {
                    usleep(5000);
                    if (s.term.remoteDataReady())
                        ch3 = s.term.remoteGetKey();
                }
                switch (ch3) {
                case 'A': return {Key::Up};
                case 'B': return {Key::Down};
                case 'C': return {Key::Right};
                case 'D': return {Key::Left};
                case 'Z': return {Key::ShiftTab};
                case 'H': return {Key::Home};
                case 'F': return {Key::End};
                default:  return {Key::None};  /* unknown sequence — eat it */
                }
            }
            /* ESC + something other than [ — ignore the combo */
            return {Key::None};
        }
        return {Key::Escape};
    }
    if (ch == '\r' || ch == '\n') return {Key::Enter};
    if (ch == '\t')               return {Key::Tab};
    if (ch == 8 || ch == 127)     return {Key::Backspace};
    if (ch >= 32 && ch < 127)     return {Key::Char, (char)ch};
    return {Key::None};
}

/* ================================================================== */
/*  ANSI file sender                                                   */
/* ================================================================== */

void send_ansi_file(Terminal& t, const std::string& path)
{
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) return;

    struct stat st;
    if (fstat(fd, &st) < 0) { close(fd); return; }

    /* Read entire file and send byte by byte through remotePutch
     * for CP437→UTF-8 conversion of art characters.
     *
     * CGA true-color fixup: After each SGR sequence, we track the
     * resulting CGA attribute and inject true-color fg+bg overrides.
     * This fixes two issues:
     *   1. \033[0m resets bg to terminal default (not necessarily black)
     *   2. \033[1;30m (intense black = dark grey) renders wrong on most
     *      terminals that map bold+black to something other than CGA grey.
     * Same approach as Terminal::setAttr() but applied to raw ANSI art.
     * See terminal-rendering.md. */
    size_t sz = (size_t)st.st_size;
    std::vector<unsigned char> buf(sz);
    ssize_t n = read(fd, buf.data(), sz);
    close(fd);
    if (n <= 0) return;

    /* ANSI-to-CGA color map (same as clrlst in terminal.cpp executeAnsi):
     * ANSI index 0-7 → CGA color number */
    static const int ansi2cga[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

    /* State machine: parse SGR sequences, track CGA attr, inject true color */
    enum { NORMAL, GOT_ESC, CSI_PARAMS } state = NORMAL;
    std::vector<int> params;
    int cur_param = 0;
    bool have_digit = false;
    unsigned char cga_attr = 0x07;  /* current CGA attribute — starts as light grey on black */

    for (ssize_t i = 0; i < n; i++) {
        unsigned char c = buf[i];

        switch (state) {
        case NORMAL:
            t.remotePutch(c);
            if (c == 0x1B) state = GOT_ESC;
            break;

        case GOT_ESC:
            t.remotePutch(c);
            if (c == '[') {
                state = CSI_PARAMS;
                params.clear();
                cur_param = 0;
                have_digit = false;
            } else {
                state = NORMAL;
            }
            break;

        case CSI_PARAMS:
            t.remotePutch(c);
            if (c >= '0' && c <= '9') {
                cur_param = cur_param * 10 + (c - '0');
                have_digit = true;
            } else if (c == ';') {
                params.push_back(have_digit ? cur_param : 0);
                cur_param = 0;
                have_digit = false;
            } else {
                /* Final byte — sequence complete */
                params.push_back(have_digit ? cur_param : 0);
                if (c == 'm') {
                    /* SGR: apply params to CGA attr (same logic as
                     * Terminal::executeAnsi case 'm') */
                    if (params.empty() || (params.size() == 1 && params[0] == 0))
                        params = {0};  /* \033[m = \033[0m */
                    for (int p : params) {
                        if (p == 0)       cga_attr = 0x07;
                        else if (p == 1)  cga_attr |= 0x08;
                        else if (p == 5)  cga_attr |= 0x80;
                        else if (p == 7) {
                            int base = cga_attr & 0x77;
                            cga_attr = (cga_attr & 0x88) | (base << 4) | (base >> 4);
                        }
                        else if (p == 8)  cga_attr = 0x00;
                        else if (p >= 30 && p <= 37)
                            cga_attr = (cga_attr & 0xf8) | ansi2cga[p - 30];
                        else if (p >= 40 && p <= 47)
                            cga_attr = (cga_attr & 0x8f) | (ansi2cga[p - 40] << 4);
                    }
                    t.injectTrueColor(cga_attr);
                }
                state = NORMAL;
            }
            break;
        }
    }
}

/* ================================================================== */
/*  Navigation helpers                                                 */
/* ================================================================== */

static void begin_field(Session& s, Form& form);

/* Forward declarations for ScreenForm (defined below) */
static void sf_init(Session& s, ScreenForm& form);

static void fire_on_enter(Session& s)
{
    if (auto* sf = std::get_if<ScreenForm>(&s.current_ui)) {
        sf_init(s, *sf);
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
/*  Screen Form (fullscreen positioned) handling                       */
/* ================================================================== */

/* ================================================================== */
/*  Date segment model                                                 */
/* ================================================================== */

struct DateSeg { int width; int min_val; int max_val; };

/* Segment definitions per DateFormat (v1: MDY locale) */
static int date_segments(DateFormat fmt, DateSeg segs[3])
{
    switch (fmt) {
    case DateFormat::MonthDayFullYear:
        segs[0] = {2, 1, 12}; segs[1] = {2, 1, 31}; segs[2] = {4, 1900, 2099};
        return 3;
    case DateFormat::MonthDayYear:
        segs[0] = {2, 1, 12}; segs[1] = {2, 1, 31}; segs[2] = {2, 0, 99};
        return 3;
    case DateFormat::MonthYear:
        segs[0] = {2, 1, 12}; segs[1] = {4, 1900, 2099};
        return 2;
    }
    return 0;
}

static int date_total_digits(DateFormat fmt)
{
    DateSeg segs[3];
    int n = date_segments(fmt, segs);
    int total = 0;
    for (int i = 0; i < n; i++) total += segs[i].width;
    return total;
}

/* Which segment does digit position `pos` fall in? */
static int date_segment_at(DateFormat fmt, int pos)
{
    DateSeg segs[3];
    int n = date_segments(fmt, segs);
    int offset = 0;
    for (int i = 0; i < n; i++) {
        if (pos < offset + segs[i].width) return i;
        offset += segs[i].width;
    }
    return n - 1;
}

/* Starting digit offset of segment `seg` */
static int date_segment_offset(DateFormat fmt, int seg)
{
    DateSeg segs[3];
    date_segments(fmt, segs);
    int offset = 0;
    for (int i = 0; i < seg; i++) offset += segs[i].width;
    return offset;
}

/* Extract the digits for segment `seg` from the raw buffer */
static std::string date_seg_digits(DateFormat fmt, const std::string& buf, int seg)
{
    int off = date_segment_offset(fmt, seg);
    DateSeg segs[3];
    date_segments(fmt, segs);
    int end = off + segs[seg].width;
    if (off >= (int)buf.size()) return "";
    if (end > (int)buf.size()) end = (int)buf.size();
    return buf.substr(off, end - off);
}

/* Format raw digits → display string with separators */
static std::string format_date(const std::string& digits, DateFormat fmt)
{
    DateSeg segs[3];
    int n = date_segments(fmt, segs);
    std::string out;
    int pos = 0;
    for (int i = 0; i < n && pos < (int)digits.size(); i++) {
        if (i > 0) out += '/';
        for (int j = 0; j < segs[i].width && pos < (int)digits.size(); j++)
            out += digits[pos++];
    }
    return out;
}

/* Check if a completed segment value is in range */
static bool date_seg_valid(const DateSeg& seg, const std::string& digits)
{
    if ((int)digits.size() != seg.width) return false;
    int val = std::atoi(digits.c_str());
    return val >= seg.min_val && val <= seg.max_val;
}

/* Leading digit optimization: can this first digit possibly start a valid
 * value for this segment?  If not, auto-pad with '0' and advance. */
static bool date_needs_autopad(const DateSeg& seg, char first_digit)
{
    if (seg.width != 2) return false;  /* only for 2-digit segments */
    int d = first_digit - '0';
    /* If the first digit times 10 already exceeds max, the only valid
     * interpretation is 0N — auto-pad with leading zero. */
    return (d * 10 > seg.max_val);
}

static std::string format_phone(const std::string& digits)
{
    /* digits = up to 10 raw digits → XXX-XXX-XXXX */
    std::string out;
    for (int i = 0; i < (int)digits.size() && i < 10; i++) {
        if (i == 3 || i == 6) out += '-';
        out += digits[i];
    }
    return out;
}

/* Clear the command line area and show a message */
static void sf_show_message(Session& s, ScreenForm& form, const char* msg,
                            unsigned char attr = 0x07)
{
    s.term.gotoXY(form.cmd_col, form.cmd_row);
    s.term.setAttr(attr);
    /* Clear the command line area */
    for (int i = 0; i < form.cmd_width; i++)
        s.term.putch(' ');
    /* Write message */
    if (msg && msg[0]) {
        s.term.gotoXY(form.cmd_col, form.cmd_row);
        s.term.puts(msg);
    }
}

/* Render a field's display value at its screen position */
static void sf_render_field(Session& s, ScreenField& f, const std::string& value)
{
    s.term.gotoXY(f.col, f.row);
    s.term.setAttr(0x0F);  /* bright white for field values */

    /* Clear the field area */
    for (int i = 0; i < f.width; i++)
        s.term.putch(' ');
    s.term.gotoXY(f.col, f.row);

    /* Display formatted value */
    std::string display;
    if (auto* df = std::get_if<DateField>(&f.widget)) {
        display = format_date(value, df->format);
    } else if (std::holds_alternative<PhoneField>(f.widget)) {
        display = format_phone(value);
    } else if (auto* sel = std::get_if<SelectField>(&f.widget)) {
        /* Show the label for the selected key */
        for (auto& opt : sel->options) {
            if (value.size() == 1 && std::toupper(value[0]) == opt.first) {
                display = opt.second;
                break;
            }
        }
        if (display.empty()) display = value;
    } else {
        auto* tf = std::get_if<TextField>(&f.widget);
        if (tf && tf->masked) {
            display = std::string(value.size(), '*');
        } else {
            display = value;
        }
    }

    int limit = f.width;
    for (int i = 0; i < (int)display.size() && i < limit; i++)
        s.term.putch((unsigned char)display[i]);
}

/* Position cursor at a field's input position (end of current value) */
static void sf_position_cursor(Session& s, ScreenField& f, const std::string& buf)
{
    int offset = 0;
    if (auto* df = std::get_if<DateField>(&f.widget))
        offset = (int)format_date(buf, df->format).size();
    else if (std::holds_alternative<PhoneField>(f.widget))
        offset = (int)format_phone(buf).size();
    else
        offset = (int)buf.size();
    s.term.gotoXY(f.col + offset, f.row);
}

/* Render a Select field's popup choice list */
static void sf_show_popup(Session& s, ScreenField& f)
{
    auto* sel = std::get_if<SelectField>(&f.widget);
    if (!sel || sel->display != SelectDisplay::Popup) return;

    for (int i = 0; i < (int)sel->options.size(); i++) {
        s.term.gotoXY(sel->popup_col, sel->popup_row + i);
        s.term.setAttr(0x0E);  /* yellow for key */
        s.term.printf("%c", sel->options[i].first);
        s.term.setAttr(0x03);  /* cyan for separator */
        s.term.puts("> ");
        s.term.setAttr(0x07);  /* light grey for label */
        s.term.puts(sel->options[i].second.c_str());
    }
}

/* Clear a Select field's popup choice list */
static void sf_clear_popup(Session& s, ScreenField& f)
{
    auto* sel = std::get_if<SelectField>(&f.widget);
    if (!sel || sel->display != SelectDisplay::Popup) return;

    for (int i = 0; i < (int)sel->options.size(); i++) {
        s.term.gotoXY(sel->popup_col, sel->popup_row + i);
        s.term.setAttr(0x07);
        /* Clear enough for "N> " + longest label */
        int clear_width = 3;
        for (auto& opt : sel->options)
            if ((int)opt.second.size() + 3 > clear_width)
                clear_width = (int)opt.second.size() + 3;
        for (int j = 0; j < clear_width; j++)
            s.term.putch(' ');
    }
}

/* Focus a field: show its prompt, render value, position cursor */
static void sf_focus_field(Session& s, ScreenForm& form)
{
    if (s.current_field_index < 0)
        s.current_field_index = 0;
    if (s.current_field_index >= (int)form.fields.size())
        s.current_field_index = (int)form.fields.size() - 1;

    ScreenField& f = form.fields[s.current_field_index];

    /* Load existing value into edit buffer.  Widgets that store formatted
     * values need to convert back to their raw editing form. */
    auto it = s.form_state.values.find(f.name);
    if (it != s.form_state.values.end() && !it->second.empty()) {
        if (std::holds_alternative<DateField>(f.widget) ||
            std::holds_alternative<PhoneField>(f.widget)) {
            /* Strip non-digits — stored value has separators */
            s.input_buffer.clear();
            for (char c : it->second)
                if (c >= '0' && c <= '9') s.input_buffer += c;
        } else if (auto* sel = std::get_if<SelectField>(&f.widget)) {
            /* Stored value is the label — find the key */
            s.input_buffer.clear();
            for (auto& opt : sel->options) {
                if (opt.second == it->second) {
                    s.input_buffer = std::string(1, opt.first);
                    break;
                }
            }
        } else {
            s.input_buffer = it->second;
        }
    } else {
        s.input_buffer.clear();
    }

    /* Show instruction in command line */
    sf_show_message(s, form, f.prompt.c_str(), 0x0E);

    /* Show popup choice list if applicable */
    sf_show_popup(s, f);

    /* Highlight the field and position cursor */
    s.term.setAttr(0x0F);
    sf_render_field(s, f, s.input_buffer);
    sf_position_cursor(s, f, s.input_buffer);
}

/* Leave the current field — store value, move focus.
 * Navigation is never blocked.  Validation happens at submit time. */
static void sf_leave_field(Session& s, ScreenForm& form, int direction)
{
    ScreenField& f = form.fields[s.current_field_index];

    /* Clear popup if leaving a popup Select */
    sf_clear_popup(s, f);

    /* Store value — widgets that own a display format store the
     * formatted form so the consumer never sees raw internals. */
    if (auto* df = std::get_if<DateField>(&f.widget)) {
        if (!s.input_buffer.empty())
            s.form_state.values[f.name] = format_date(s.input_buffer, df->format);
        else
            s.form_state.values[f.name] = s.input_buffer;
    } else if (std::holds_alternative<PhoneField>(f.widget)) {
        if (!s.input_buffer.empty())
            s.form_state.values[f.name] = format_phone(s.input_buffer);
        else
            s.form_state.values[f.name] = s.input_buffer;
    } else if (auto* sel = std::get_if<SelectField>(&f.widget)) {
        if (!s.input_buffer.empty()) {
            for (auto& opt : sel->options) {
                if (s.input_buffer.size() == 1 && opt.first == s.input_buffer[0]) {
                    s.form_state.values[f.name] = opt.second;
                    break;
                }
            }
        } else {
            s.form_state.values[f.name] = s.input_buffer;
        }
    } else {
        s.form_state.values[f.name] = s.input_buffer;
    }

    /* Move to next/prev field */
    int next = s.current_field_index + direction;
    if (next < 0) next = 0;
    if (next >= (int)form.fields.size()) {
        /* Past the last field — enter command line mode */
        s.current_field_index = (int)form.fields.size();
        s.input_buffer.clear();
        sf_show_message(s, form, "Q to save, 1-9 to edit, ESC to cancel", 0x0E);
        s.term.gotoXY(form.cmd_col, form.cmd_row);
        return;
    }

    s.current_field_index = next;
    sf_focus_field(s, form);
}

/* Built-in date validation: check each segment is in range */
static bool date_validate(DateFormat fmt, const std::string& digits)
{
    DateSeg segs[3];
    int n = date_segments(fmt, segs);
    int total = 0;
    for (int i = 0; i < n; i++) total += segs[i].width;
    if ((int)digits.size() != total) return false;
    for (int i = 0; i < n; i++) {
        std::string sd = date_seg_digits(fmt, digits, i);
        if (!date_seg_valid(segs[i], sd)) return false;
    }
    return true;
}

/* Validate all fields at submit time.  Returns index of first failing
 * field, or -1 if all pass.  Sets errmsg to a description. */
static int sf_validate_all(ScreenForm& form, const FormResult& r, const char*& errmsg)
{
    for (int i = 0; i < (int)form.fields.size(); i++) {
        ScreenField& f = form.fields[i];
        auto it = r.values.find(f.name);
        bool empty = (it == r.values.end() || it->second.empty());

        if (f.required && empty) {
            errmsg = "This field is required.";
            return i;
        }
        if (!empty) {
            /* Built-in widget validation */
            if (auto* df = std::get_if<DateField>(&f.widget)) {
                std::string digits;
                for (char c : it->second)
                    if (c >= '0' && c <= '9') digits += c;
                if (!date_validate(df->format, digits)) {
                    errmsg = "Invalid date.";
                    return i;
                }
            }
            /* External validator */
            if (f.validate && !f.validate(it->second)) {
                errmsg = "Invalid input. Please try again.";
                return i;
            }
        }
    }
    return -1;
}

/* Initialize a screen form: render background and all fields */
static void sf_init(Session& s, ScreenForm& form)
{
    s.form_state = FormResult();
    s.current_field_index = 0;
    s.input_buffer.clear();

    /* Clear screen and send background art */
    s.term.clearScreen();
    if (!form.background_file.empty())
        send_ansi_file(s.term, form.background_file);

    /* Focus the first field */
    sf_focus_field(s, form);
}

/* Dispatch a key event to the focused field */
static void dispatch_screen_form_input(Session& s, ScreenForm& form, KeyEvent ev)
{
    /* Command line mode: past the last field, waiting for commands */
    if (s.current_field_index >= (int)form.fields.size()) {
        if (ev.key == Key::Escape) {
            s.form_state.completed = false;
            if (form.on_cancel) form.on_cancel(s);
            return;
        }
        if (ev.key == Key::ShiftTab || ev.key == Key::Up) {
            /* Back to last field */
            s.current_field_index = (int)form.fields.size() - 1;
            sf_focus_field(s, form);
            return;
        }
        if (ev.key == Key::Tab || ev.key == Key::Down) {
            /* Twoline focus: jump to designated field */
            int target = form.twoline_field;
            if (target < 0 || target >= (int)form.fields.size())
                target = 0;
            s.current_field_index = target;
            sf_focus_field(s, form);
            return;
        }
        if (ev.key == Key::Char) {
            char upper = (char)std::toupper(ev.ch);
            if (upper == 'Q') {
                /* Validate all fields before submitting */
                const char* errmsg = nullptr;
                int fail = sf_validate_all(form, s.form_state, errmsg);
                if (fail >= 0) {
                    /* Jump to the failing field */
                    s.current_field_index = fail;
                    sf_focus_field(s, form);
                    sf_show_message(s, form, errmsg, 0x0C);
                    sf_position_cursor(s, form.fields[fail], s.input_buffer);
                    return;
                }
                s.form_state.completed = true;
                if (form.on_submit)
                    form.on_submit(s, s.form_state);
                return;
            }
            /* Digit → jump to that field (1-based) */
            if (upper >= '1' && upper <= '9') {
                int idx = upper - '1';
                if (idx < (int)form.fields.size()) {
                    s.current_field_index = idx;
                    sf_focus_field(s, form);
                }
            }
        }
        return;
    }

    ScreenField& f = form.fields[s.current_field_index];

    /* --- Text --- */
    if (auto* tf = std::get_if<TextField>(&f.widget)) {
        int max = tf->max_chars > 0 ? tf->max_chars : f.width;
        switch (ev.key) {
        case Key::Char:
            if ((int)s.input_buffer.size() < max) {
                s.input_buffer += ev.ch;
                if (tf->masked)
                    s.term.putch('*');
                else
                    s.term.putch((unsigned char)ev.ch);
            }
            break;
        case Key::Backspace:
            if (!s.input_buffer.empty()) {
                s.input_buffer.pop_back();
                sf_render_field(s, f, s.input_buffer);
                sf_position_cursor(s, f, s.input_buffer);
            }
            break;
        case Key::Enter: case Key::Tab: case Key::Down:
            sf_leave_field(s, form, +1);
            break;
        case Key::ShiftTab: case Key::Up:
            sf_leave_field(s, form, -1);
            break;
        case Key::Escape:
            s.form_state.completed = false;
            if (form.on_cancel) form.on_cancel(s);
            break;
        default: break;
        }
    }

    /* --- Select --- */
    else if (auto* sel = std::get_if<SelectField>(&f.widget)) {
        if (ev.key == Key::Escape) {
            s.form_state.completed = false;
            if (form.on_cancel) form.on_cancel(s);
        } else if (ev.key == Key::Tab || ev.key == Key::Enter || ev.key == Key::Down) {
            sf_leave_field(s, form, +1);
        } else if (ev.key == Key::ShiftTab || ev.key == Key::Up) {
            sf_leave_field(s, form, -1);
        } else if (ev.key == Key::Char) {
            char upper = (char)std::toupper(ev.ch);
            for (auto& opt : sel->options) {
                if (opt.first == upper) {
                    s.input_buffer = std::string(1, upper);
                    sf_render_field(s, f, s.input_buffer);
                    /* advanceOnSelection */
                    sf_leave_field(s, form, +1);
                    return;
                }
            }
        }
    }

    /* --- Date --- */
    else if (auto* df = std::get_if<DateField>(&f.widget)) {
        /* Segment-based date entry.  input_buffer holds raw digits.
         * Segments (month, day, year) are at fixed offsets determined
         * by the DateFormat.  Auto-advance between segments on fill,
         * with leading-digit auto-pad for 2-digit segments. */
        if (ev.key == Key::Escape) {
            s.form_state.completed = false;
            if (form.on_cancel) form.on_cancel(s);
        } else if (ev.key == Key::ShiftTab || ev.key == Key::Up) {
            sf_leave_field(s, form, -1);
        } else if (ev.key == Key::Enter || ev.key == Key::Tab || ev.key == Key::Down) {
            sf_leave_field(s, form, +1);
        } else if (ev.key == Key::Backspace) {
            if (!s.input_buffer.empty()) {
                s.input_buffer.pop_back();
                sf_render_field(s, f, s.input_buffer);
                sf_position_cursor(s, f, s.input_buffer);
            }
        } else if (ev.key == Key::Char && ev.ch >= '0' && ev.ch <= '9') {
            int total = date_total_digits(df->format);
            if ((int)s.input_buffer.size() < total) {
                DateSeg segs[3];
                date_segments(df->format, segs);
                int cur_seg = date_segment_at(df->format, (int)s.input_buffer.size());
                std::string seg_digits = date_seg_digits(df->format, s.input_buffer, cur_seg);

                /* Leading digit auto-pad: if this first digit can only mean 0N,
                 * insert the leading zero automatically */
                if (seg_digits.empty() && date_needs_autopad(segs[cur_seg], ev.ch)) {
                    s.input_buffer += '0';
                    s.input_buffer += ev.ch;
                } else {
                    s.input_buffer += ev.ch;
                }

                sf_render_field(s, f, s.input_buffer);
                sf_position_cursor(s, f, s.input_buffer);

                /* Check if all digits are filled → advance out of field */
                if ((int)s.input_buffer.size() >= total)
                    sf_leave_field(s, form, +1);
            }
        }
    }

    /* --- Phone --- */
    else if (std::holds_alternative<PhoneField>(f.widget)) {
        /* Store raw digits only, display formatted */
        if (ev.key == Key::Escape) {
            s.form_state.completed = false;
            if (form.on_cancel) form.on_cancel(s);
        } else if (ev.key == Key::ShiftTab || ev.key == Key::Up) {
            sf_leave_field(s, form, -1);
        } else if (ev.key == Key::Enter || ev.key == Key::Tab || ev.key == Key::Down) {
            sf_leave_field(s, form, +1);
        } else if (ev.key == Key::Backspace) {
            if (!s.input_buffer.empty()) {
                s.input_buffer.pop_back();
                sf_render_field(s, f, s.input_buffer);
                sf_position_cursor(s, f, s.input_buffer);
            }
        } else if (ev.key == Key::Char && ev.ch >= '0' && ev.ch <= '9') {
            if ((int)s.input_buffer.size() < 10) {
                s.input_buffer += ev.ch;
                sf_render_field(s, f, s.input_buffer);
                sf_position_cursor(s, f, s.input_buffer);
                /* Auto-advance when complete */
                if ((int)s.input_buffer.size() == 10)
                    sf_leave_field(s, form, +1);
            }
        }
    }
}

/* ================================================================== */
/*  Input dispatch                                                     */
/* ================================================================== */

static void dispatch_input(Session& s, unsigned char ch)
{
    if (auto* sf = std::get_if<ScreenForm>(&s.current_ui)) {
        KeyEvent ev = translate_key(s, ch);
        if (ev.key != Key::None)
            dispatch_screen_form_input(s, *sf, ev);
    } else if (auto* nav = std::get_if<Navigator>(&s.current_ui)) {
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
