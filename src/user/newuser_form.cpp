/*
 * newuser_form.cpp — BBS new user registration ScreenForm definition
 *
 * Port of tools/formtest.cpp's make_newuser_form() for real BBS use.
 * Field positions match the newans.ans ANSI art layout (0-based coords).
 *
 * Differences from formtest:
 *   - background_file uses sys.cfg.gfilesdir + "newans.ans"
 *   - DateFormat::MonthDayYear (MM/DD/YY, 2-digit year)
 *   - Computer type options built from getComputerType()/numComputerTypes()
 *   - Name validate calls check_name()
 *   - Phone validate checks sysconfig_free_phone
 *   - on_submit calls newuser_form_apply()
 *   - on_cancel sets *ctx.hangup = 1
 */

#include "user/newuser_form.h"
#include "tui/screen_form.h"
#include "user/newuser.h"
#include "misccmd.h"
#include "timest.h"
#include "session.h"
#include "system.h"
#include "platform.h"

#include <cstring>
#include <cctype>
#include <cstdlib>
#include <string>

/* ================================================================== */
/*  Computer type popup builder                                        */
/* ================================================================== */

static SelectField make_comptype_options()
{
    SelectField sel;
    sel.display   = SelectDisplay::Popup;
    sel.popup_row = 7;
    sel.popup_col = 56;

    int n = numComputerTypes();
    for (int i = 0; i < n && i < 9; i++) {
        char key = (char)('1' + i);
        sel.options.push_back({key, getComputerType(i)});
    }
    return sel;
}

/* ================================================================== */
/*  Form definition                                                    */
/* ================================================================== */

/*
 * Field positions match the newans.ans layout.
 * The original newuser.cpp uses go(row_1based, col_1based) which maps
 * to gotoXY(col-1, row-1):  goin(7,18) → gotoXY(17, 6).
 * Command line ("gotop") is at goin(2,4) → gotoXY(3, 1).
 */

ScreenForm make_newuser_form(const std::string& datadir)
{
    ScreenForm form;
    form.id              = "newuser";
    form.background_file = datadir + "newans.ans";
    form.cmd_row         = 1;
    form.cmd_col         = 3;
    form.cmd_width       = 60;

    /* --- Name (row 7, col 18 → gotoXY(17, 6)) --- */
    {
        ScreenField f;
        f.name     = "name";
        f.widget   = TextField{.max_chars = 30};
        f.row      = 6; f.col = 17; f.width = 30;
        f.required = true;
        f.prompt   = "Enter your handle or real name:";
        f.validate = [](const std::string& v) -> bool {
            /* Uppercase before check_name — matches original input_name() flow.
             * Side effect: sets io.hangup on trashcan hit (acceptable — sf_run
             * will exit on next iteration when hangup is detected). */
            std::string upper = v;
            for (auto& c : upper) c = (char)std::toupper((unsigned char)c);
            return check_name(upper.c_str()) != 0;
        };
        form.fields.push_back(f);
    }

    /* --- Real Name (row 8, col 18 → gotoXY(17, 7)) --- */
    {
        ScreenField f;
        f.name     = "realname";
        f.widget   = TextField{.max_chars = 20};
        f.row      = 7; f.col = 17; f.width = 20;
        f.required = true;
        f.prompt   = "Enter your real name, or = if same as alias:";
        form.fields.push_back(f);
    }

    /* --- Sex (row 9, col 18 → gotoXY(17, 8)) --- */
    {
        ScreenField f;
        f.name   = "sex";
        f.widget = SelectField{
            .options = {{'M', "Male"}, {'F', "Female"}, {'Y', "Yes"}, {'L', "Lots"}},
        };
        f.row    = 8; f.col = 17; f.width = 10;
        f.prompt = "Sex: (M)ale (F)emale (Y)es (L)ots";
        form.fields.push_back(f);
    }

    /* --- Birthdate MM/DD/YY (row 10, col 18 → gotoXY(17, 9)) --- */
    {
        ScreenField f;
        f.name     = "birthdate";
        f.widget   = DateField{.format = DateFormat::MonthDayYear};
        f.row      = 9; f.col = 17; f.width = 10;
        f.required = true;
        f.prompt   = "Enter birthdate (MM/DD/YY):";
        form.fields.push_back(f);
    }

    /* --- Phone (row 11, col 18 → gotoXY(17, 10)) --- */
    {
        ScreenField f;
        f.name     = "phone";
        f.widget   = PhoneField{};
        f.row      = 10; f.col = 17; f.width = 14;
        f.required = true;
        f.prompt   = "Enter voice phone (XXX-XXX-XXXX):";
        f.validate = [](const std::string& digits) -> bool {
            auto& sys = System::instance();
            if (sys.cfg.sysconfig & sysconfig_free_phone)
                return true;        /* free-phone mode: any input accepted */
            return (int)digits.size() == 10;
        };
        form.fields.push_back(f);
    }

    /* --- Street (row 12, col 18 → gotoXY(17, 11)) --- */
    {
        ScreenField f;
        f.name     = "street";
        f.widget   = TextField{.max_chars = 35};
        f.row      = 11; f.col = 17; f.width = 35;
        f.required = true;
        f.prompt   = "Enter your street address:";
        form.fields.push_back(f);
    }

    /* --- City (row 13, col 18 → gotoXY(17, 12)) --- */
    {
        ScreenField f;
        f.name     = "city";
        f.widget   = TextField{.max_chars = 35};
        f.row      = 12; f.col = 17; f.width = 35;
        f.required = true;
        f.prompt   = "Enter your city and state:";
        form.fields.push_back(f);
    }

    /* --- Comment (row 14, col 18 → gotoXY(17, 13)) --- */
    {
        ScreenField f;
        f.name     = "comment";
        f.widget   = TextField{.max_chars = 35};
        f.row      = 13; f.col = 17; f.width = 35;
        f.required = false;
        f.prompt   = "Enter a comment (optional):";
        form.fields.push_back(f);
    }

    /* --- Computer type (row 15, col 18 → gotoXY(17, 14)) --- */
    {
        ScreenField f;
        f.name   = "comptype";
        f.widget = make_comptype_options();
        f.row    = 14; f.col = 17; f.width = 20;
        f.prompt = "Enter your computer type";
        form.fields.push_back(f);
    }

    /* --- Password (row 16, col 18 → gotoXY(17, 15)) --- */
    {
        ScreenField f;
        f.name     = "password";
        f.widget   = TextField{.max_chars = 20, .masked = true};
        f.row      = 15; f.col = 17; f.width = 20;
        f.required = true;
        f.prompt   = "Enter a password (3-20 chars):";
        f.validate = [](const std::string& v) -> bool {
            return (int)v.size() >= 3;
        };
        form.fields.push_back(f);
    }

    /* --- Callbacks --- */

    form.on_submit = [](Terminal& /*term*/, SFContext& /*ctx*/, const FormResult& r) {
        /* Screen already cleared by FormExit::Clear before this is called */
        newuser_form_apply(r);
    };

    form.on_cancel = [](Terminal& /*term*/, SFContext& ctx) {
        /* Cancel = hang up (new user who bails is gone) */
        if (ctx.hangup) *ctx.hangup = 1;
    };

    return form;
}

/* ================================================================== */
/*  Apply form result to sess.user                                     */
/* ================================================================== */

void newuser_form_apply(const FormResult& r)
{
    auto& sess = Session::instance();

    auto get = [&](const char* key) -> std::string {
        auto it = r.values.find(key);
        return (it != r.values.end()) ? it->second : std::string();
    };

    /* Name — uppercase (match original input_name behaviour) */
    {
        std::string name = get("name");
        char buf[31];
        strncpy(buf, name.c_str(), 30);
        buf[30] = '\0';
        strupr(buf);
        sess.user.set_name(buf);
    }

    /* Real name — "=" means copy from alias */
    {
        std::string rn = get("realname");
        if (rn == "=")
            sess.user.set_realname(sess.user.name());
        else
            sess.user.set_realname(rn.c_str());
    }

    /* Sex — stored as key char 'M'/'F'/'Y'/'L' */
    {
        std::string sex = get("sex");
        if (!sex.empty())
            sess.user.set_sex(sex[0]);
    }

    /* Birthdate — stored as "MM/DD/YY" (2-digit year, +1900) */
    {
        std::string bd = get("birthdate");
        if (!bd.empty()) {
            int m = std::atoi(bd.c_str());
            int d = (int)bd.size() >= 5 ? std::atoi(bd.c_str() + 3) : 0;
            int y = (int)bd.size() >= 8 ? std::atoi(bd.c_str() + 6) + 1900 : 1900;
            sess.user.set_birth_month((unsigned short)m);
            sess.user.set_birth_day((unsigned short)d);
            sess.user.set_birth_year((unsigned short)(y - 1900));
            sess.user.set_age(years_old(
                sess.user.birth_month(),
                sess.user.birth_day(),
                sess.user.birth_year()));
        }
    }

    /* Phone — stored formatted (XXX-XXX-XXXX) */
    sess.user.set_phone(get("phone").c_str());

    /* Address */
    sess.user.set_street(get("street").c_str());
    sess.user.set_city(get("city").c_str());
    sess.user.set_comment(get("comment").c_str());

    /* Computer type — stored as key char '1'..'9', convert to 0-based index */
    {
        std::string ct = get("comptype");
        if (!ct.empty())
            sess.user.set_comp_type(ct[0] - '1');
    }

    /* Password */
    sess.user.set_password(get("password").c_str());

    /* Post-form defaults (set after fields so we always get clean values) */
    sess.user.set_helplevel(2);
    sess.user.set_lastconf(1);
    sess.user.set_lastsub(0);
    sess.user.set_lastdir(0);
    sess.user.set_flisttype(1);
}
