/*
 * user.h — User value type + flag constants
 *
 * Replaces vardec_user.h. Contains:
 *   - userrec raw struct (binary-compatible with disk/JSON)
 *   - smalrec index entry (transitional — will be removed)
 *   - User class wrapping userrec with typed accessors
 *   - All user flag constants (inact, sysstatus, exempt, restrict)
 *
 * Layer 2: depends only on vardec_types.h
 */

#ifndef _USER_H_
#define _USER_H_

#include "vardec_types.h"

#ifdef __cplusplus
#include <string>
#include <cstring>

struct cJSON;   /* forward declaration for to_json/from_json */
#endif

/*============================================================
 * smalrec — name-to-number index entry
 * Transitional type — will be replaced by UserDB internal map.
 *============================================================*/
typedef struct {
    char            name[31];
    unsigned short  number;
} smalrec;

/*============================================================
 * userrec — raw user record (binary-compatible with disk)
 *
 * This struct maps directly to JSON serialization in json_io.cpp.
 * Field sizes and order matter. Do not rearrange.
 *============================================================*/
typedef struct {
    char    name[31],
            realname[21],
            callsign[7],
            phone[21],
            dphone[21],
            pw[21],
            laston[9],
            firston[9],
            note[41],
            comment[41],
            street[41],
            city[41],
            macros[4][MAX_PATH_LEN],
            sex;

    unsigned char
            age,
            inact,
            comp_type,
            defprot,
            defed,
            flisttype,
            mlisttype,
            helplevel,
            lastsub,
            lastdir,
            lastconf,
            screenchars,
            screenlines,
            sl,
            dsl,
            exempt,
            colors[20],
            votes[20],
            illegal,
            waiting,
            subop,
            ontoday;

    unsigned short
            forwardusr,
            msgpost,
            emailsent,
            feedbacksent,
            posttoday,
            etoday,
            ar,
            dar,
            restrict,
            month,
            day,
            year;

    int
            fpts;

    unsigned short
            uploaded,
            downloaded,
            logons,
            fsenttoday1,
            emailnet,
            postnet;

    unsigned long
            msgread,
            uk,
            dk,
            daten,
            sysstatus,
            lastrate,
            nuv,
            timebank;

    float
            timeontoday,
            extratime,
            timeon,
            pcr,
            ratio,
            pos_account,
            neg_account;

    char
            res[29];
    long
            resl[29];
    int
            resi[29];

    float
            resf[29];

    long
            qscn[200],
            nscn[200];
} userrec;


/*============================================================
 * User flag constants
 *============================================================*/

/* userrec.inact */
#define inact_deleted 0x01
#define inact_inactive 0x02
#define inact_lockedout 0x04

/* userrec.exempt */
#define exempt_ratio 0x01
#define exempt_time 0x02
#define exempt_userlist 0x04
#define exempt_post 0x08

/* userrec.restrict */
#define restrict_logon 0x0001
#define restrict_chat 0x0002
#define restrict_validate 0x0004
#define restrict_automessage 0x0008
#define restrict_anony 0x0010
#define restrict_post 0x0020
#define restrict_email 0x0040
#define restrict_vote 0x0080
#define restrict_auto_msg_delete 0x0100
#define restrict_net 0x0200
#define restrict_upload 0x0400
#define restrict_rumours 0x0800
#define restrict_timebank 0x1000
#define restrict_bbslist 0x2000
#define restrict_userlist 0x4000

#define restrict_string "LCMA*PEVKN!RTBU "

/* userrec.sysstatus */
#define sysstatus_ansi 0x0001
#define sysstatus_color 0x0002
#define sysstatus_fullline 0x0004
#define sysstatus_pause_on_page 0x0008
#define sysstatus_smw 0x0020
#define sysstatus_full_screen 0x0040
#define sysstatus_nscan_file_system 0x0080
#define sysstatus_regular 0x0100
#define sysstatus_clr_scrn 0x0200

#ifdef __cplusplus
/*============================================================
 * class User — Lakos value type
 *
 * All access through typed accessors. No public struct.
 * The userrec struct is an implementation detail only.
 *============================================================*/
class User {
public:
    /** Zero-initialized user record. */
    User() { memset(&data_, 0, sizeof(data_)); }

    /*--- Display formatting (implemented in user.cpp) ---*/

    /** Proper-cased name (was pnam). "SYSOP USER" -> "Sysop User" */
    std::string display_name() const;

    /** Proper-cased name + " #num" (was nam). */
    std::string display_name(int usernum) const;

    /*--- Identity accessors ---*/

    /** User's handle/alias. Max 30 chars. Stored uppercase in index. */
    const char* name() const { return data_.name; }
    void set_name(const char* v) { std::strncpy(data_.name, v, 30); data_.name[30] = 0; }

    /** Real name. Max 20 chars. */
    const char* realname() const { return data_.realname; }
    void set_realname(const char* v) { std::strncpy(data_.realname, v, 20); data_.realname[20] = 0; }

    /** Amateur radio callsign. Max 6 chars. */
    const char* callsign() const { return data_.callsign; }
    void set_callsign(const char* v) { std::strncpy(data_.callsign, v, 6); data_.callsign[6] = 0; }

    /** Voice phone number. Max 20 chars (typically "555-555-5555"). */
    const char* phone() const { return data_.phone; }
    void set_phone(const char* v) { std::strncpy(data_.phone, v, 20); data_.phone[20] = 0; }

    /** Data phone number. Max 20 chars. */
    const char* dataphone() const { return data_.dphone; }
    void set_dataphone(const char* v) { std::strncpy(data_.dphone, v, 20); data_.dphone[20] = 0; }

    /** Password. Max 20 chars. */
    const char* password() const { return data_.pw; }
    void set_password(const char* v) { std::strncpy(data_.pw, v, 20); data_.pw[20] = 0; }

    /** Last logon date string. Max 8 chars (e.g. "02/23/26"). */
    const char* laston() const { return data_.laston; }
    void set_laston(const char* v) { std::strncpy(data_.laston, v, 8); data_.laston[8] = 0; }

    /** First logon date string. Max 8 chars. */
    const char* firston() const { return data_.firston; }
    void set_firston(const char* v) { std::strncpy(data_.firston, v, 8); data_.firston[8] = 0; }

    /** Sysop note. Max 40 chars. */
    const char* note() const { return data_.note; }
    void set_note(const char* v) { std::strncpy(data_.note, v, 40); data_.note[40] = 0; }

    /** User comment / tagline. Max 40 chars. */
    const char* comment() const { return data_.comment; }
    void set_comment(const char* v) { std::strncpy(data_.comment, v, 40); data_.comment[40] = 0; }

    /** Street address. Max 40 chars. */
    const char* street() const { return data_.street; }
    void set_street(const char* v) { std::strncpy(data_.street, v, 40); data_.street[40] = 0; }

    /** City/state. Max 40 chars. */
    const char* city() const { return data_.city; }
    void set_city(const char* v) { std::strncpy(data_.city, v, 40); data_.city[40] = 0; }

    /** Gender character ('M', 'F', etc.). */
    char sex() const { return data_.sex; }
    void set_sex(char v) { data_.sex = v; }

    /*--- Keyboard macros (4 slots, MAX_PATH_LEN each) ---*/

    const char* macro(int slot) const { return data_.macros[slot]; }
    char* macro_mut(int slot) { return data_.macros[slot]; }

    /*--- Access levels ---*/

    /** Security level (0-255). Determines command access via slrec table. */
    unsigned char sl() const { return data_.sl; }
    void set_sl(unsigned char v) { data_.sl = v; }

    /** Download/upload security level. Controls file section access. */
    unsigned char dsl() const { return data_.dsl; }
    void set_dsl(unsigned char v) { data_.dsl = v; }

    /** AR flags (16 bits, A-P). Controls subboard access. */
    unsigned short ar() const { return data_.ar; }
    void set_ar(unsigned short v) { data_.ar = v; }

    /** DAR flags (16 bits, A-P). Controls directory access. */
    unsigned short dar() const { return data_.dar; }
    void set_dar(unsigned short v) { data_.dar = v; }

    /** Restriction flags. See restrict_* defines. */
    unsigned short restrict_flags() const { return data_.restrict; }
    void set_restrict(unsigned short v) { data_.restrict = v; }

    /** Exemption flags. See exempt_* defines. */
    unsigned char exempt() const { return data_.exempt; }
    void set_exempt(unsigned char v) { data_.exempt = v; }

    /*--- Activity flags ---*/

    /** Inactivity/deletion flags. See inact_* defines. */
    unsigned char inact() const { return data_.inact; }
    void set_inact(unsigned char v) { data_.inact = v; }

    /** System status flags. See sysstatus_* defines. */
    unsigned long sysstatus() const { return data_.sysstatus; }
    void set_sysstatus(unsigned long v) { data_.sysstatus = v; }

    /*--- Numeric fields ---*/

    /** User's age in years. Recalculated on logon from birthdate. */
    unsigned char age() const { return data_.age; }
    void set_age(unsigned char v) { data_.age = v; }

    /** Computer type index. */
    unsigned char comp_type() const { return data_.comp_type; }
    void set_comp_type(unsigned char v) { data_.comp_type = v; }

    /** Default transfer protocol index. */
    unsigned char defprot() const { return data_.defprot; }
    void set_defprot(unsigned char v) { data_.defprot = v; }

    /** Default message editor (0=line, 1=full-screen). */
    unsigned char defed() const { return data_.defed; }
    void set_defed(unsigned char v) { data_.defed = v; }

    /** File list format type. */
    unsigned char flisttype() const { return data_.flisttype; }
    void set_flisttype(unsigned char v) { data_.flisttype = v; }

    /** Message list format type. */
    unsigned char mlisttype() const { return data_.mlisttype; }
    void set_mlisttype(unsigned char v) { data_.mlisttype = v; }

    /** Help level (0=expert, 1=regular, 2=novice). */
    unsigned char helplevel() const { return data_.helplevel; }
    void set_helplevel(unsigned char v) { data_.helplevel = v; }

    /** Last subboard index (saved on logoff). */
    unsigned char lastsub() const { return data_.lastsub; }
    void set_lastsub(unsigned char v) { data_.lastsub = v; }

    /** Last directory index (saved on logoff). */
    unsigned char lastdir() const { return data_.lastdir; }
    void set_lastdir(unsigned char v) { data_.lastdir = v; }

    /** Last conference index (saved on logoff). */
    unsigned char lastconf() const { return data_.lastconf; }
    void set_lastconf(unsigned char v) { data_.lastconf = v; }

    /** Terminal width (default 80). */
    unsigned char screenchars() const { return data_.screenchars; }
    void set_screenchars(unsigned char v) { data_.screenchars = v; }

    /** Terminal height (default 25). */
    unsigned char screenlines() const { return data_.screenlines; }
    void set_screenlines(unsigned char v) { data_.screenlines = v; }

    /** Illegal logon attempts. Reset on successful logon. */
    unsigned char illegal() const { return data_.illegal; }
    void set_illegal(unsigned char v) { data_.illegal = v; }

    /** Number of waiting messages. */
    unsigned char waiting() const { return data_.waiting; }
    void set_waiting(unsigned char v) { data_.waiting = v; }

    /** Subop board number (255 = none). */
    unsigned char subop() const { return data_.subop; }
    void set_subop(unsigned char v) { data_.subop = v; }

    /** Times logged on today. */
    unsigned char ontoday() const { return data_.ontoday; }
    void set_ontoday(unsigned char v) { data_.ontoday = v; }

    /** User number to forward mail to (0=none, 255=system). */
    unsigned short forwardusr() const { return data_.forwardusr; }
    void set_forwardusr(unsigned short v) { data_.forwardusr = v; }

    /** Total message posts. */
    unsigned short msgpost() const { return data_.msgpost; }
    void set_msgpost(unsigned short v) { data_.msgpost = v; }

    /** Total email sent. */
    unsigned short emailsent() const { return data_.emailsent; }
    void set_emailsent(unsigned short v) { data_.emailsent = v; }

    /** Total feedback sent. */
    unsigned short feedbacksent() const { return data_.feedbacksent; }
    void set_feedbacksent(unsigned short v) { data_.feedbacksent = v; }

    /** Posts today. Reset on first logon of day. */
    unsigned short posttoday() const { return data_.posttoday; }
    void set_posttoday(unsigned short v) { data_.posttoday = v; }

    /** Email sent today. */
    unsigned short etoday() const { return data_.etoday; }
    void set_etoday(unsigned short v) { data_.etoday = v; }

    /** Birth month (1-12). */
    unsigned short birth_month() const { return data_.month; }
    void set_birth_month(unsigned short v) { data_.month = v; }

    /** Birth day (1-31). */
    unsigned short birth_day() const { return data_.day; }
    void set_birth_day(unsigned short v) { data_.day = v; }

    /** Birth year (2-digit or 4-digit). */
    unsigned short birth_year() const { return data_.year; }
    void set_birth_year(unsigned short v) { data_.year = v; }

    /** File points balance. */
    int fpts() const { return data_.fpts; }
    void set_fpts(int v) { data_.fpts = v; }

    /** Files uploaded count. */
    unsigned short uploaded() const { return data_.uploaded; }
    void set_uploaded(unsigned short v) { data_.uploaded = v; }

    /** Files downloaded count. */
    unsigned short downloaded() const { return data_.downloaded; }
    void set_downloaded(unsigned short v) { data_.downloaded = v; }

    /** Total logons. */
    unsigned short logons() const { return data_.logons; }
    void set_logons(unsigned short v) { data_.logons = v; }

    /** Files sent today (batch). */
    unsigned short fsenttoday1() const { return data_.fsenttoday1; }
    void set_fsenttoday1(unsigned short v) { data_.fsenttoday1 = v; }

    /** Email sent to net. */
    unsigned short emailnet() const { return data_.emailnet; }
    void set_emailnet(unsigned short v) { data_.emailnet = v; }

    /** Posts to net. */
    unsigned short postnet() const { return data_.postnet; }
    void set_postnet(unsigned short v) { data_.postnet = v; }

    /** Total messages read. */
    unsigned long msgread() const { return data_.msgread; }
    void set_msgread(unsigned long v) { data_.msgread = v; }

    /** Upload kilobytes. */
    unsigned long uk() const { return data_.uk; }
    void set_uk(unsigned long v) { data_.uk = v; }

    /** Download kilobytes. */
    unsigned long dk() const { return data_.dk; }
    void set_dk(unsigned long v) { data_.dk = v; }

    /** Last logon timestamp (DOS date format). */
    unsigned long daten() const { return data_.daten; }
    void set_daten(unsigned long v) { data_.daten = v; }

    /** Last connect rate. */
    unsigned long lastrate() const { return data_.lastrate; }
    void set_lastrate(unsigned long v) { data_.lastrate = v; }

    /** New user voting status (-1=pending). */
    unsigned long nuv_status() const { return data_.nuv; }
    void set_nuv_status(unsigned long v) { data_.nuv = v; }

    /** Time bank balance (minutes). */
    unsigned long timebank() const { return data_.timebank; }
    void set_timebank(unsigned long v) { data_.timebank = v; }

    /*--- Float fields ---*/

    /** Time online today (seconds). */
    float timeontoday() const { return data_.timeontoday; }
    void set_timeontoday(float v) { data_.timeontoday = v; }

    /** Extra time adjustment (seconds). */
    float extratime() const { return data_.extratime; }
    void set_extratime(float v) { data_.extratime = v; }

    /** Total time online (seconds). */
    float total_timeon() const { return data_.timeon; }
    void set_total_timeon(float v) { data_.timeon = v; }

    /** Post/call ratio requirement. */
    float pcr() const { return data_.pcr; }
    void set_pcr(float v) { data_.pcr = v; }

    /** UL/DL ratio requirement. */
    float ul_dl_ratio() const { return data_.ratio; }
    void set_ul_dl_ratio(float v) { data_.ratio = v; }

    /** Positive account balance. */
    float pos_account() const { return data_.pos_account; }
    void set_pos_account(float v) { data_.pos_account = v; }

    /** Negative account balance. */
    float neg_account() const { return data_.neg_account; }
    void set_neg_account(float v) { data_.neg_account = v; }

    /*--- Array accessors (const + mutable) ---*/

    /** Color scheme (20 slots). */
    const unsigned char* colors() const { return data_.colors; }
    unsigned char* colors_mut() { return data_.colors; }

    /** Voting record (20 slots). */
    const unsigned char* votes() const { return data_.votes; }
    unsigned char* votes_mut() { return data_.votes; }

    /** Message scan pointers (200 subs). */
    const long* qscn() const { return data_.qscn; }
    long* qscn_mut() { return data_.qscn; }

    /** File new-scan flags (200 dirs). */
    const long* nscn() const { return data_.nscn; }
    long* nscn_mut() { return data_.nscn; }

    /*--- Reserved arrays (opaque storage, preserved across serialization) ---*/

    const char* res() const { return data_.res; }
    char* res_mut() { return data_.res; }

    const long* resl() const { return data_.resl; }
    long* resl_mut() { return data_.resl; }

    const int* resi() const { return data_.resi; }
    int* resi_mut() { return data_.resi; }

    const float* resf() const { return data_.resf; }
    float* resf_mut() { return data_.resf; }

    /*--- JSON serialization (implemented in user.cpp) ---*/

    cJSON* to_json() const;
    static User from_json(cJSON* j);

    /*--- Status queries ---*/

    bool is_deleted() const { return (data_.inact & inact_deleted) != 0; }
    bool is_inactive() const { return (data_.inact & inact_inactive) != 0; }
    bool is_locked_out() const { return (data_.inact & inact_lockedout) != 0; }

    bool has_ansi() const { return (data_.sysstatus & sysstatus_ansi) != 0; }
    bool has_color() const { return (data_.sysstatus & sysstatus_color) != 0; }
    bool has_fullscreen() const { return (data_.sysstatus & sysstatus_full_screen) != 0; }
    bool has_hotkeys() const { return (data_.sysstatus & sysstatus_fullline) == 0; }

    /*--- Flag manipulation ---*/

    void mark_deleted() { data_.inact |= inact_deleted; }
    void mark_active() { data_.inact &= ~inact_deleted; }

    void set_sysstatus_flag(unsigned long flag, bool on) {
        if (on) data_.sysstatus |= flag;
        else data_.sysstatus &= ~flag;
    }

    void set_restrict_flag(unsigned short flag, bool on) {
        if (on) data_.restrict |= flag;
        else data_.restrict &= ~flag;
    }

    void set_exempt_flag(unsigned char flag, bool on) {
        if (on) data_.exempt |= flag;
        else data_.exempt &= ~flag;
    }

private:
    userrec data_;
};
#endif /* __cplusplus */

#endif /* _USER_H_ */
