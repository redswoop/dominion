/*
 * screen_form.cpp — Fullscreen positioned form engine
 *
 * Adapted from ui.cpp's ScreenForm section.  Operates on Terminal& +
 * SFContext& instead of Session&.  No BBS globals, no Session singleton.
 *
 * Differences from ui.cpp:
 *   - SelectField stores the key char in FormResult (not the label).
 *     Simplifies consumer code: r.values["sex"] == "M" etc.
 *   - Echo pointer: masked TextField entry sets *ctx.echo=0; leave restores.
 *   - sf_run() is the synchronous entry point (no multi-session event loop).
 *   - sf_translate_key() replaces translate_key() (no Session dependency).
 */

#include "screen_form.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <unistd.h>

/* ================================================================== */
/*  Key translation                                                    */
/* ================================================================== */

KeyEvent sf_translate_key(Terminal& term, unsigned char ch)
{
    /* Local console extended key: ncurses returns 0 as prefix, then a
     * DOS scancode on the next call (the pendingScancode_ protocol in
     * Terminal::localGetKeyNB).  Map scancodes to logical keys. */
    if (ch == 0) {
        if (term.keyReady()) {
            unsigned char sc = term.getKeyNB();
            switch (sc) {
            case 72: return {Key::Up};
            case 80: return {Key::Down};
            case 75: return {Key::Left};
            case 77: return {Key::Right};
            case 71: return {Key::Home};
            case 79: return {Key::End};
            case 15: return {Key::ShiftTab};
            default: return {Key::None};
            }
        }
        return {Key::None};
    }

    /* ESC — check for escape sequence from remote TCP.
     *
     * Only check the remote source for sequence continuation.  When
     * ncurses is active (local console), it handles ESC sequences
     * internally and delivers arrow keys as KEY_UP etc. via the
     * scancode protocol above.  A raw 27 from local is always a
     * standalone ESC.  Using the combined keyReady()/getKeyNB() here
     * would cross-contaminate sources: remote data would be consumed
     * as part of a local ESC, eating the ESC and corrupting remote. */
    if (ch == 27) {
        usleep(20000);  /* 20ms for sequence bytes to arrive */
        if (term.remoteConnected() && term.remoteDataReady()) {
            unsigned char ch2 = term.remoteGetKey();
            if (ch2 == '[') {
                unsigned char ch3 = 0;
                for (int i = 0; i < 5 && !ch3; i++) {
                    usleep(5000);
                    if (term.remoteDataReady())
                        ch3 = term.remoteGetKey();
                }
                switch (ch3) {
                case 'A': return {Key::Up};
                case 'B': return {Key::Down};
                case 'C': return {Key::Right};
                case 'D': return {Key::Left};
                case 'Z': return {Key::ShiftTab};
                case 'H': return {Key::Home};
                case 'F': return {Key::End};
                default:  return {Key::None};
                }
            }
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
/*  Date segment model (identical to ui.cpp)                          */
/* ================================================================== */

struct DateSeg { int width; int min_val; int max_val; };

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

static int date_segment_offset(DateFormat fmt, int seg)
{
    DateSeg segs[3];
    date_segments(fmt, segs);
    int offset = 0;
    for (int i = 0; i < seg; i++) offset += segs[i].width;
    return offset;
}

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

static std::string format_phone(const std::string& digits)
{
    std::string out;
    for (int i = 0; i < (int)digits.size() && i < 10; i++) {
        if (i == 3 || i == 6) out += '-';
        out += digits[i];
    }
    return out;
}

static bool date_seg_valid(const DateSeg& seg, const std::string& digits)
{
    if ((int)digits.size() != seg.width) return false;
    int val = std::atoi(digits.c_str());
    return val >= seg.min_val && val <= seg.max_val;
}

static bool date_needs_autopad(const DateSeg& seg, char first_digit)
{
    if (seg.width != 2) return false;
    int d = first_digit - '0';
    return (d * 10 > seg.max_val);
}

/* ================================================================== */
/*  Mode detection and conditional visibility                          */
/* ================================================================== */

static bool sf_is_sequential(ScreenForm& form)
{
    if (form.mode == FormMode::Sequential) return true;
    if (form.mode == FormMode::Screen) return false;
    /* Auto: sequential if no background file */
    return form.background_file.empty();
}

static bool sf_field_visible(ScreenField& f, const FormResult& state)
{
    return !f.when || f.when(state);
}

/* ================================================================== */
/*  Screen rendering helpers                                           */
/* ================================================================== */

static void sf_show_message(Terminal& term, ScreenForm& form, const char* msg,
                             unsigned char attr = 0x07)
{
    term.gotoXY(form.cmd_col, form.cmd_row);
    term.setAttr(attr);
    for (int i = 0; i < form.cmd_width; i++)
        term.putch(' ');
    if (msg && msg[0]) {
        term.gotoXY(form.cmd_col, form.cmd_row);
        term.puts(msg);
    }
}

static void sf_render_field(Terminal& term, ScreenField& f, const std::string& value)
{
    term.gotoXY(f.col, f.row);
    term.setAttr(0x0F);

    for (int i = 0; i < f.width; i++)
        term.putch(' ');
    term.gotoXY(f.col, f.row);

    std::string display;
    if (auto* df = std::get_if<DateField>(&f.widget)) {
        display = format_date(value, df->format);
    } else if (std::holds_alternative<PhoneField>(f.widget)) {
        display = format_phone(value);
    } else if (auto* sel = std::get_if<SelectField>(&f.widget)) {
        /* value is key char — look up label for display */
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
        term.putch((unsigned char)display[i]);
}

static void sf_position_cursor(Terminal& term, ScreenField& f, const std::string& buf)
{
    int offset = 0;
    if (auto* df = std::get_if<DateField>(&f.widget))
        offset = (int)format_date(buf, df->format).size();
    else if (std::holds_alternative<PhoneField>(f.widget))
        offset = (int)format_phone(buf).size();
    else
        offset = (int)buf.size();
    term.gotoXY(f.col + offset, f.row);
}

static void sf_show_popup(Terminal& term, ScreenField& f)
{
    auto* sel = std::get_if<SelectField>(&f.widget);
    if (!sel || sel->display != SelectDisplay::Popup) return;

    for (int i = 0; i < (int)sel->options.size(); i++) {
        term.gotoXY(sel->popup_col, sel->popup_row + i);
        term.setAttr(0x0E);
        term.printf("%c", sel->options[i].first);
        term.setAttr(0x03);
        term.puts("> ");
        term.setAttr(0x07);
        term.puts(sel->options[i].second.c_str());
    }
}

static void sf_clear_popup(Terminal& term, ScreenField& f)
{
    auto* sel = std::get_if<SelectField>(&f.widget);
    if (!sel || sel->display != SelectDisplay::Popup) return;

    for (int i = 0; i < (int)sel->options.size(); i++) {
        term.gotoXY(sel->popup_col, sel->popup_row + i);
        term.setAttr(0x07);
        int clear_width = 3;
        for (auto& opt : sel->options)
            if ((int)opt.second.size() + 3 > clear_width)
                clear_width = (int)opt.second.size() + 3;
        for (int j = 0; j < clear_width; j++)
            term.putch(' ');
    }
}

/* ================================================================== */
/*  Field focus and leave                                              */
/* ================================================================== */

static void sf_focus_field(Terminal& term, SFContext& ctx, ScreenForm& form)
{
    if (ctx.current_field_index < 0)
        ctx.current_field_index = 0;
    if (ctx.current_field_index >= (int)form.fields.size())
        ctx.current_field_index = (int)form.fields.size() - 1;

    ScreenField& f = form.fields[ctx.current_field_index];

    /* Set echo mode: masked fields suppress BBS echo */
    auto* tf_check = std::get_if<TextField>(&f.widget);
    if (ctx.echo)
        *ctx.echo = (tf_check && tf_check->masked) ? 0 : 1;

    /* Restore edit buffer from stored value */
    auto it = ctx.form_state.values.find(f.name);
    if (it != ctx.form_state.values.end() && !it->second.empty()) {
        if (std::holds_alternative<DateField>(f.widget) ||
            std::holds_alternative<PhoneField>(f.widget)) {
            /* Strip non-digits — stored value has separators */
            ctx.input_buffer.clear();
            for (char c : it->second)
                if (c >= '0' && c <= '9') ctx.input_buffer += c;
        } else if (std::holds_alternative<SelectField>(f.widget)) {
            /* Stored value IS the key char */
            ctx.input_buffer = it->second;
        } else {
            ctx.input_buffer = it->second;
        }
    } else {
        ctx.input_buffer.clear();
    }

    sf_show_message(term, form, f.prompt.c_str(), 0x0E);
    sf_show_popup(term, f);
    term.setAttr(0x0F);
    sf_render_field(term, f, ctx.input_buffer);
    sf_position_cursor(term, f, ctx.input_buffer);
}

static void sf_leave_field(Terminal& term, SFContext& ctx, ScreenForm& form, int direction)
{
    ScreenField& f = form.fields[ctx.current_field_index];

    /* Restore echo before leaving */
    if (ctx.echo) *ctx.echo = 1;

    sf_clear_popup(term, f);

    /* Store value — formatted for Date/Phone, key char for Select, raw for Text */
    if (auto* df = std::get_if<DateField>(&f.widget)) {
        ctx.form_state.values[f.name] = ctx.input_buffer.empty()
            ? ctx.input_buffer
            : format_date(ctx.input_buffer, df->format);
    } else if (std::holds_alternative<PhoneField>(f.widget)) {
        ctx.form_state.values[f.name] = ctx.input_buffer.empty()
            ? ctx.input_buffer
            : format_phone(ctx.input_buffer);
    } else if (std::holds_alternative<SelectField>(f.widget)) {
        /* Store key char directly — simpler for consumer code */
        ctx.form_state.values[f.name] = ctx.input_buffer;
    } else {
        ctx.form_state.values[f.name] = ctx.input_buffer;
    }

    /* Move to next/prev field, skipping fields where when() is false */
    int next = ctx.current_field_index + direction;
    while (next >= 0 && next < (int)form.fields.size() &&
           !sf_field_visible(form.fields[next], ctx.form_state))
        next += direction;

    if (next < 0) next = 0;
    if (next >= (int)form.fields.size()) {
        /* Past last field — enter command line mode */
        ctx.current_field_index = (int)form.fields.size();
        ctx.input_buffer.clear();
        if (ctx.echo) *ctx.echo = 1;
        sf_show_message(term, form, "Q to save, 1-9 to edit, ESC to cancel", 0x0E);
        term.gotoXY(form.cmd_col, form.cmd_row);
        return;
    }

    ctx.current_field_index = next;
    sf_focus_field(term, ctx, form);
}

/* ================================================================== */
/*  Validation                                                         */
/* ================================================================== */

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

static int sf_validate_all(ScreenForm& form, const FormResult& r, const char*& errmsg)
{
    for (int i = 0; i < (int)form.fields.size(); i++) {
        ScreenField& f = form.fields[i];
        if (!sf_field_visible(f, r)) continue;
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

            /* External validator — pass raw value */
            std::string raw_val = it->second;
            if (std::holds_alternative<DateField>(f.widget) ||
                std::holds_alternative<PhoneField>(f.widget)) {
                /* Strip separators — pass digits only */
                raw_val.clear();
                for (char c : it->second)
                    if (c >= '0' && c <= '9') raw_val += c;
            } else if (std::holds_alternative<SelectField>(f.widget)) {
                /* Stored value IS the key char — pass directly */
                raw_val = it->second;
            }
            if (f.validate && !f.validate(raw_val)) {
                errmsg = "Invalid input. Please try again.";
                return i;
            }
        }
    }
    return -1;
}

/* ================================================================== */
/*  Sequential mode helpers                                            */
/* ================================================================== */

/* Find the next visible field at or after `start` (direction +1) or
 * at or before `start` (direction -1).  Returns -1 if none. */
static int sf_next_visible(ScreenForm& form, const FormResult& state, int start, int dir)
{
    int i = start;
    while (i >= 0 && i < (int)form.fields.size()) {
        if (sf_field_visible(form.fields[i], state)) return i;
        i += dir;
    }
    return -1;
}

/* Format a display string for a field value (shared between screen/seq) */
static std::string sf_format_display(ScreenField& f, const std::string& buf)
{
    if (auto* df = std::get_if<DateField>(&f.widget))
        return format_date(buf, df->format);
    if (std::holds_alternative<PhoneField>(f.widget))
        return format_phone(buf);
    if (auto* sel = std::get_if<SelectField>(&f.widget)) {
        for (auto& opt : sel->options)
            if (buf.size() == 1 && std::toupper(buf[0]) == opt.first)
                return opt.second;
        return buf;
    }
    auto* tf = std::get_if<TextField>(&f.widget);
    if (tf && tf->masked)
        return std::string(buf.size(), '*');
    return buf;
}

static void sf_seq_render_value(Terminal& term, SFContext& ctx,
                                ScreenField& f, const std::string& buf)
{
    term.gotoXY(ctx.inline_start_x, ctx.inline_start_y);
    term.setAttr(0x0F);
    for (int i = 0; i < f.width; i++) term.putch(' ');
    term.gotoXY(ctx.inline_start_x, ctx.inline_start_y);
    std::string display = sf_format_display(f, buf);
    int limit = f.width;
    for (int i = 0; i < (int)display.size() && i < limit; i++)
        term.putch((unsigned char)display[i]);
}

static void sf_seq_position_cursor(Terminal& term, SFContext& ctx,
                                   ScreenField& f, const std::string& buf)
{
    std::string display = sf_format_display(f, buf);
    term.gotoXY(ctx.inline_start_x + (int)display.size(), ctx.inline_start_y);
}

/* Forward declarations for mutual recursion */
static void sf_seq_focus_field(Terminal& term, SFContext& ctx, ScreenForm& form);
static void form_exit_action(Terminal& term, FormExit exit);

/* Sequential forms auto-downgrade Clear→Scroll (clearing makes no sense
 * without a background).  This means the default FormExit::Clear just works
 * for both modes — screen mode clears, sequential mode scrolls. */
static FormExit sf_seq_exit(ScreenForm& form)
{
    return (form.exit == FormExit::Clear) ? FormExit::Scroll : form.exit;
}

static void sf_seq_leave_field(Terminal& term, SFContext& ctx, ScreenForm& form)
{
    ScreenField& f = form.fields[ctx.current_field_index];

    /* Restore echo before leaving */
    if (ctx.echo) *ctx.echo = 1;

    /* Store value */
    if (auto* df = std::get_if<DateField>(&f.widget)) {
        ctx.form_state.values[f.name] = ctx.input_buffer.empty()
            ? ctx.input_buffer
            : format_date(ctx.input_buffer, df->format);
    } else if (std::holds_alternative<PhoneField>(f.widget)) {
        ctx.form_state.values[f.name] = ctx.input_buffer.empty()
            ? ctx.input_buffer
            : format_phone(ctx.input_buffer);
    } else {
        ctx.form_state.values[f.name] = ctx.input_buffer;
    }

    /* Per-field immediate validation */
    auto it = ctx.form_state.values.find(f.name);
    bool empty = (it == ctx.form_state.values.end() || it->second.empty());

    if (f.required && empty) {
        term.newline();
        term.setAttr(0x0C);
        term.puts("  This field is required.");
        term.newline();
        /* Re-prompt same field */
        sf_seq_focus_field(term, ctx, form);
        return;
    }

    if (!empty) {
        /* Built-in date validation */
        if (auto* df = std::get_if<DateField>(&f.widget)) {
            std::string digits;
            for (char c : it->second)
                if (c >= '0' && c <= '9') digits += c;
            if (!date_validate(df->format, digits)) {
                term.newline();
                term.setAttr(0x0C);
                term.puts("  Invalid date.");
                term.newline();
                ctx.form_state.values[f.name].clear();
                sf_seq_focus_field(term, ctx, form);
                return;
            }
        }

        /* External validator */
        std::string raw_val = it->second;
        if (std::holds_alternative<DateField>(f.widget) ||
            std::holds_alternative<PhoneField>(f.widget)) {
            raw_val.clear();
            for (char c : it->second)
                if (c >= '0' && c <= '9') raw_val += c;
        }
        if (f.validate && !f.validate(raw_val)) {
            term.newline();
            term.setAttr(0x0C);
            term.puts("  Invalid input. Please try again.");
            term.newline();
            ctx.form_state.values[f.name].clear();
            sf_seq_focus_field(term, ctx, form);
            return;
        }
    }

    /* Advance to next visible field */
    term.newline();
    int next = sf_next_visible(form, ctx.form_state,
                               ctx.current_field_index + 1, +1);
    if (next < 0) {
        /* Past last field — auto-submit */
        const char* errmsg = nullptr;
        int fail = sf_validate_all(form, ctx.form_state, errmsg);
        if (fail >= 0) {
            term.setAttr(0x0C);
            term.puts("  ");
            term.puts(errmsg);
            term.newline();
            ctx.current_field_index = fail;
            sf_seq_focus_field(term, ctx, form);
            return;
        }
        ctx.form_state.completed = true;
        form_exit_action(term, sf_seq_exit(form));
        if (form.on_submit)
            form.on_submit(term, ctx, ctx.form_state);
        return;
    }

    ctx.current_field_index = next;
    sf_seq_focus_field(term, ctx, form);
}

static void sf_seq_focus_field(Terminal& term, SFContext& ctx, ScreenForm& form)
{
    /* Skip to next visible field */
    int idx = sf_next_visible(form, ctx.form_state,
                              ctx.current_field_index, +1);
    if (idx < 0) {
        /* No visible fields remaining — auto-submit */
        ctx.form_state.completed = true;
        form_exit_action(term, sf_seq_exit(form));
        if (form.on_submit)
            form.on_submit(term, ctx, ctx.form_state);
        return;
    }
    ctx.current_field_index = idx;

    ScreenField& f = form.fields[idx];

    /* Set echo mode */
    auto* tf_check = std::get_if<TextField>(&f.widget);
    if (ctx.echo)
        *ctx.echo = (tf_check && tf_check->masked) ? 0 : 1;

    /* Restore edit buffer from stored value */
    auto it = ctx.form_state.values.find(f.name);
    if (it != ctx.form_state.values.end() && !it->second.empty()) {
        if (std::holds_alternative<DateField>(f.widget) ||
            std::holds_alternative<PhoneField>(f.widget)) {
            ctx.input_buffer.clear();
            for (char c : it->second)
                if (c >= '0' && c <= '9') ctx.input_buffer += c;
        } else {
            ctx.input_buffer = it->second;
        }
    } else {
        ctx.input_buffer.clear();
    }

    bool plain = (ctx.render == SFRender::Plain);

    /* Print prompt */
    term.setAttr(form.prompt_attr);
    if (!f.prompt.empty()) {
        term.puts(f.prompt.c_str());
        term.putch(' ');
    }

    /* For select fields, show options after prompt */
    if (auto* sel = std::get_if<SelectField>(&f.widget)) {
        if (sel->display == SelectDisplay::Popup) {
            /* List choices vertically */
            term.newline();
            for (int i = 0; i < (int)sel->options.size(); i++) {
                term.setAttr(form.prompt_attr);
                term.puts("  ");
                term.putch(sel->options[i].first);
                term.setAttr(0x03);
                term.puts(") ");
                term.setAttr(0x07);
                term.puts(sel->options[i].second.c_str());
                term.newline();
            }
            term.setAttr(form.prompt_attr);
            term.puts("Choice: ");
        } else {
            /* Inline: compact (M/F/Y/L) format */
            term.setAttr(0x03);
            term.putch('(');
            for (int i = 0; i < (int)sel->options.size(); i++) {
                if (i > 0) term.putch('/');
                term.setAttr(form.prompt_attr);
                term.putch(sel->options[i].first);
                term.setAttr(0x03);
            }
            term.puts(") ");
        }
    }

    /* Record cursor position for field input */
    if (!plain) {
        ctx.inline_start_x = term.cursorX();
        ctx.inline_start_y = term.cursorY();
    }

    /* Render existing value if re-entering field */
    term.setAttr(form.input_attr);
    if (!ctx.input_buffer.empty()) {
        if (plain) {
            std::string display = sf_format_display(f, ctx.input_buffer);
            term.puts(display.c_str());
        } else {
            sf_seq_render_value(term, ctx, f, ctx.input_buffer);
            sf_seq_position_cursor(term, ctx, f, ctx.input_buffer);
        }
    }
}

static void sf_seq_init(Terminal& term, SFContext& ctx, ScreenForm& form)
{
    ctx.form_state    = FormResult();
    ctx.current_field_index = 0;
    ctx.input_buffer.clear();
    ctx.cancelled     = false;

    /* Clean slate — sequential forms can't redraw previous prompts,
       so re-entry means starting over from a known-good state. */
    if (ctx.render == SFRender::Plain) {
        term.newline();
        term.newline();
    } else {
        term.clearScreen();
        term.gotoXY(0, 0);
    }
    if (form.on_enter) form.on_enter(term);

    sf_seq_focus_field(term, ctx, form);
}

static void sf_seq_dispatch(Terminal& term, SFContext& ctx,
                            ScreenForm& form, KeyEvent ev)
{
    if (ctx.current_field_index >= (int)form.fields.size()) return;

    ScreenField& f = form.fields[ctx.current_field_index];
    bool plain = (ctx.render == SFRender::Plain);

    /* --- Escape: cancel in all widget types --- */
    if (ev.key == Key::Escape) {
        ctx.form_state.completed = false;
        term.newline();
        form_exit_action(term, sf_seq_exit(form));
        if (form.on_cancel) form.on_cancel(term, ctx);
        ctx.cancelled = true;
        return;
    }

    /* --- Text --- */
    if (auto* tf = std::get_if<TextField>(&f.widget)) {
        int max = tf->max_chars > 0 ? tf->max_chars : f.width;
        switch (ev.key) {
        case Key::Char:
            if ((int)ctx.input_buffer.size() < max) {
                ctx.input_buffer += ev.ch;
                if (tf->masked)
                    term.putch('*');
                else
                    term.putch((unsigned char)ev.ch);
            }
            break;
        case Key::Backspace:
            if (!ctx.input_buffer.empty()) {
                ctx.input_buffer.pop_back();
                if (plain) {
                    term.backspace();
                } else {
                    sf_seq_render_value(term, ctx, f, ctx.input_buffer);
                    sf_seq_position_cursor(term, ctx, f, ctx.input_buffer);
                }
            }
            break;
        case Key::Enter:
            sf_seq_leave_field(term, ctx, form);
            break;
        default: break;
        }
    }

    /* --- Select --- */
    else if (auto* sel = std::get_if<SelectField>(&f.widget)) {
        if (ev.key == Key::Char) {
            char upper = (char)std::toupper(ev.ch);
            for (auto& opt : sel->options) {
                if (opt.first == upper) {
                    ctx.input_buffer = std::string(1, upper);
                    if (plain)
                        term.putch(upper);
                    else
                        sf_seq_render_value(term, ctx, f, ctx.input_buffer);
                    sf_seq_leave_field(term, ctx, form);
                    return;
                }
            }
        }
    }

    /* --- Date --- */
    else if (auto* df = std::get_if<DateField>(&f.widget)) {
        if (ev.key == Key::Backspace) {
            if (!ctx.input_buffer.empty()) {
                ctx.input_buffer.pop_back();
                if (plain) {
                    term.backspace();
                } else {
                    sf_seq_render_value(term, ctx, f, ctx.input_buffer);
                    sf_seq_position_cursor(term, ctx, f, ctx.input_buffer);
                }
            }
        } else if (ev.key == Key::Enter) {
            sf_seq_leave_field(term, ctx, form);
        } else if (ev.key == Key::Char && ev.ch >= '0' && ev.ch <= '9') {
            int total = date_total_digits(df->format);
            if ((int)ctx.input_buffer.size() < total) {
                DateSeg segs[3];
                date_segments(df->format, segs);
                int cur_seg = date_segment_at(df->format, (int)ctx.input_buffer.size());
                std::string seg_digits = date_seg_digits(df->format, ctx.input_buffer, cur_seg);

                if (seg_digits.empty() && date_needs_autopad(segs[cur_seg], ev.ch)) {
                    ctx.input_buffer += '0';
                    ctx.input_buffer += ev.ch;
                } else {
                    ctx.input_buffer += ev.ch;
                }

                if (plain) {
                    /* Echo raw digits — no separators */
                    if (seg_digits.empty() && date_needs_autopad(segs[cur_seg], ev.ch))
                        term.putch('0');
                    term.putch((unsigned char)ev.ch);
                } else {
                    sf_seq_render_value(term, ctx, f, ctx.input_buffer);
                    sf_seq_position_cursor(term, ctx, f, ctx.input_buffer);
                }

                if ((int)ctx.input_buffer.size() >= total)
                    sf_seq_leave_field(term, ctx, form);
            }
        }
    }

    /* --- Phone --- */
    else if (std::holds_alternative<PhoneField>(f.widget)) {
        if (ev.key == Key::Backspace) {
            if (!ctx.input_buffer.empty()) {
                ctx.input_buffer.pop_back();
                if (plain) {
                    term.backspace();
                } else {
                    sf_seq_render_value(term, ctx, f, ctx.input_buffer);
                    sf_seq_position_cursor(term, ctx, f, ctx.input_buffer);
                }
            }
        } else if (ev.key == Key::Enter) {
            sf_seq_leave_field(term, ctx, form);
        } else if (ev.key == Key::Char && ev.ch >= '0' && ev.ch <= '9') {
            if ((int)ctx.input_buffer.size() < 10) {
                ctx.input_buffer += ev.ch;
                if (plain) {
                    term.putch((unsigned char)ev.ch);
                } else {
                    sf_seq_render_value(term, ctx, f, ctx.input_buffer);
                    sf_seq_position_cursor(term, ctx, f, ctx.input_buffer);
                }

                if ((int)ctx.input_buffer.size() == 10)
                    sf_seq_leave_field(term, ctx, form);
            }
        }
    }
}

/* ================================================================== */
/*  Dispatch                                                           */
/* ================================================================== */

static void form_exit_action(Terminal& term, FormExit exit)
{
    switch (exit) {
    case FormExit::Clear:   term.clearScreen(); break;
    case FormExit::Scroll:  term.newline(); break;
    case FormExit::None:    break;
    }
}

void sf_dispatch(Terminal& term, SFContext& ctx,
                 ScreenForm& form, KeyEvent ev)
{
    if (ctx.render != SFRender::Fullscreen) {
        sf_seq_dispatch(term, ctx, form, ev);
        return;
    }

    /* Command line mode: past last field */
    if (ctx.current_field_index >= (int)form.fields.size()) {
        if (ev.key == Key::Escape) {
            ctx.form_state.completed = false;
            form_exit_action(term, form.exit);
            if (form.on_cancel) form.on_cancel(term, ctx);
            ctx.cancelled = true;
            return;
        }
        if (ev.key == Key::ShiftTab || ev.key == Key::Up) {
            ctx.current_field_index = (int)form.fields.size() - 1;
            sf_focus_field(term, ctx, form);
            return;
        }
        if (ev.key == Key::Tab || ev.key == Key::Down) {
            int target = form.twoline_field;
            if (target < 0 || target >= (int)form.fields.size())
                target = 0;
            ctx.current_field_index = target;
            sf_focus_field(term, ctx, form);
            return;
        }
        if (ev.key == Key::Char) {
            char upper = (char)std::toupper(ev.ch);
            if (upper == 'Q') {
                const char* errmsg = nullptr;
                int fail = sf_validate_all(form, ctx.form_state, errmsg);
                if (fail >= 0) {
                    ctx.current_field_index = fail;
                    sf_focus_field(term, ctx, form);
                    sf_show_message(term, form, errmsg, 0x0C);
                    sf_position_cursor(term, form.fields[fail], ctx.input_buffer);
                    return;
                }
                ctx.form_state.completed = true;
                form_exit_action(term, form.exit);
                if (form.on_submit)
                    form.on_submit(term, ctx, ctx.form_state);
                return;
            }
            if (upper >= '1' && upper <= '9') {
                int idx = upper - '1';
                if (idx < (int)form.fields.size()) {
                    ctx.current_field_index = idx;
                    sf_focus_field(term, ctx, form);
                }
            }
        }
        return;
    }

    ScreenField& f = form.fields[ctx.current_field_index];

    /* --- Text --- */
    if (auto* tf = std::get_if<TextField>(&f.widget)) {
        int max = tf->max_chars > 0 ? tf->max_chars : f.width;
        switch (ev.key) {
        case Key::Char:
            if ((int)ctx.input_buffer.size() < max) {
                ctx.input_buffer += ev.ch;
                if (tf->masked)
                    term.putch('*');
                else
                    term.putch((unsigned char)ev.ch);
            }
            break;
        case Key::Backspace:
            if (!ctx.input_buffer.empty()) {
                ctx.input_buffer.pop_back();
                sf_render_field(term, f, ctx.input_buffer);
                sf_position_cursor(term, f, ctx.input_buffer);
            }
            break;
        case Key::Enter: case Key::Tab: case Key::Down:
            sf_leave_field(term, ctx, form, +1);
            break;
        case Key::ShiftTab: case Key::Up:
            sf_leave_field(term, ctx, form, -1);
            break;
        case Key::Escape:
            form_exit_action(term, form.exit);
            if (form.on_cancel) form.on_cancel(term, ctx);
            ctx.cancelled = true;
            break;
        default: break;
        }
    }

    /* --- Select --- */
    else if (auto* sel = std::get_if<SelectField>(&f.widget)) {
        if (ev.key == Key::Escape) {
            form_exit_action(term, form.exit);
            if (form.on_cancel) form.on_cancel(term, ctx);
            ctx.cancelled = true;
        } else if (ev.key == Key::Tab || ev.key == Key::Enter || ev.key == Key::Down) {
            sf_leave_field(term, ctx, form, +1);
        } else if (ev.key == Key::ShiftTab || ev.key == Key::Up) {
            sf_leave_field(term, ctx, form, -1);
        } else if (ev.key == Key::Char) {
            char upper = (char)std::toupper(ev.ch);
            for (auto& opt : sel->options) {
                if (opt.first == upper) {
                    ctx.input_buffer = std::string(1, upper);
                    sf_render_field(term, f, ctx.input_buffer);
                    sf_leave_field(term, ctx, form, +1);
                    return;
                }
            }
        }
    }

    /* --- Date --- */
    else if (auto* df = std::get_if<DateField>(&f.widget)) {
        if (ev.key == Key::Escape) {
            form_exit_action(term, form.exit);
            if (form.on_cancel) form.on_cancel(term, ctx);
            ctx.cancelled = true;
        } else if (ev.key == Key::ShiftTab || ev.key == Key::Up) {
            sf_leave_field(term, ctx, form, -1);
        } else if (ev.key == Key::Enter || ev.key == Key::Tab || ev.key == Key::Down) {
            sf_leave_field(term, ctx, form, +1);
        } else if (ev.key == Key::Backspace) {
            if (!ctx.input_buffer.empty()) {
                ctx.input_buffer.pop_back();
                sf_render_field(term, f, ctx.input_buffer);
                sf_position_cursor(term, f, ctx.input_buffer);
            }
        } else if (ev.key == Key::Char && ev.ch >= '0' && ev.ch <= '9') {
            int total = date_total_digits(df->format);
            if ((int)ctx.input_buffer.size() < total) {
                DateSeg segs[3];
                date_segments(df->format, segs);
                int cur_seg = date_segment_at(df->format, (int)ctx.input_buffer.size());
                std::string seg_digits = date_seg_digits(df->format, ctx.input_buffer, cur_seg);

                if (seg_digits.empty() && date_needs_autopad(segs[cur_seg], ev.ch)) {
                    ctx.input_buffer += '0';
                    ctx.input_buffer += ev.ch;
                } else {
                    ctx.input_buffer += ev.ch;
                }

                sf_render_field(term, f, ctx.input_buffer);
                sf_position_cursor(term, f, ctx.input_buffer);

                if ((int)ctx.input_buffer.size() >= total)
                    sf_leave_field(term, ctx, form, +1);
            }
        }
    }

    /* --- Phone --- */
    else if (std::holds_alternative<PhoneField>(f.widget)) {
        if (ev.key == Key::Escape) {
            form_exit_action(term, form.exit);
            if (form.on_cancel) form.on_cancel(term, ctx);
            ctx.cancelled = true;
        } else if (ev.key == Key::ShiftTab || ev.key == Key::Up) {
            sf_leave_field(term, ctx, form, -1);
        } else if (ev.key == Key::Enter || ev.key == Key::Tab || ev.key == Key::Down) {
            sf_leave_field(term, ctx, form, +1);
        } else if (ev.key == Key::Backspace) {
            if (!ctx.input_buffer.empty()) {
                ctx.input_buffer.pop_back();
                sf_render_field(term, f, ctx.input_buffer);
                sf_position_cursor(term, f, ctx.input_buffer);
            }
        } else if (ev.key == Key::Char && ev.ch >= '0' && ev.ch <= '9') {
            if ((int)ctx.input_buffer.size() < 10) {
                ctx.input_buffer += ev.ch;
                sf_render_field(term, f, ctx.input_buffer);
                sf_position_cursor(term, f, ctx.input_buffer);
                if ((int)ctx.input_buffer.size() == 10)
                    sf_leave_field(term, ctx, form, +1);
            }
        }
    }
}

/* ================================================================== */
/*  Init and run                                                       */
/* ================================================================== */

void sf_init(Terminal& term, SFContext& ctx, ScreenForm& form)
{
    /* Resolve: what the form wants vs what the session supports.
     * Enum order: Fullscreen(0) < Sequential(1) < Plain(2).
     * Higher = more degraded.  effective = max(ideal, max_render). */
    SFRender ideal = sf_is_sequential(form)
        ? SFRender::Sequential : SFRender::Fullscreen;
    ctx.render = (ideal >= ctx.max_render) ? ideal : ctx.max_render;

    if (ctx.render != SFRender::Fullscreen) {
        sf_seq_init(term, ctx, form);
        return;
    }

    ctx.form_state    = FormResult();
    ctx.current_field_index = 0;
    ctx.input_buffer.clear();
    ctx.cancelled     = false;

    term.clearScreen();
    if (!form.background_file.empty())
        term.sendAnsiFile(form.background_file);
    if (form.on_enter) form.on_enter(term);

    sf_focus_field(term, ctx, form);
}

bool sf_run(Terminal& term, SFContext& ctx, ScreenForm& form)
{
    sf_init(term, ctx, form);

    /* Track whether remote was connected at form start.  If it
     * disconnects mid-form, we should abort even if local console
     * is still active (BBS use case: remote user hangs up). */
    bool had_remote = term.remoteConnected();

    while (ctx.hangup == nullptr || !*ctx.hangup) {
        /* Detect disconnect */
        if (!term.localActive() && !term.remoteConnected()) {
            if (ctx.hangup) *ctx.hangup = 1;
            break;
        }
        /* Remote dropped mid-session — abort the form */
        if (had_remote && !term.remoteConnected()) {
            if (ctx.hangup) *ctx.hangup = 1;
            break;
        }

        if (!term.keyReady()) {
            usleep(10000);  /* 10ms poll interval */
            continue;
        }
        unsigned char ch = term.getKeyNB();

        KeyEvent ev = sf_translate_key(term, ch);
        if (ev.key != Key::None)
            sf_dispatch(term, ctx, form, ev);

        if (ctx.form_state.completed || ctx.cancelled)
            break;
    }

    return ctx.form_state.completed;
}

std::string sf_prompt(Terminal& term, SFContext& ctx, ScreenField& field)
{
    ScreenForm form;
    form.mode = FormMode::Sequential;
    form.exit = FormExit::None;
    form.fields.push_back(field);

    sf_run(term, ctx, form);

    auto it = form.fields[0].name.empty()
        ? ctx.form_state.values.begin()
        : ctx.form_state.values.find(form.fields[0].name);
    if (it != ctx.form_state.values.end())
        return it->second;
    return "";
}
