/*
 * acs.c — Access Control String evaluator.
 *
 * Pure evaluation logic in acs_check() — reads only from acs_context_t.
 * Bridge function acs_fill_context() populates context from session globals.
 */

#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "bbsutl.h"
#include "session.h"
#include "system.h"
#include "acs.h"
#include "file.h"


extern int fastlogon;


void acs_fill_context(acs_context_t *ctx)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    memset(ctx, 0, sizeof(*ctx));
    ctx->sl          = sess.actsl;
    ctx->dsl         = sess.user.dsl;
    ctx->age         = sess.user.age;
    ctx->exempt      = sess.user.exempt;
    ctx->ar          = sess.user.ar;
    ctx->dar         = sess.user.dar;
    ctx->usernum     = sess.usernum;
    ctx->modem_speed = sess.modem_speed;
    ctx->is_sysop    = sysop2();
    ctx->is_backdoor = sess.backdoor;
    ctx->fastlogon   = fastlogon;
    ctx->can_post    = postr_ok();
    ctx->helplevel   = sess.user.helplevel;
    ctx->conf_flagstr = sys.conf[sess.curconf].flagstr;
}


int acs_check(const char *acs_string, const acs_context_t *ctx)
{
    char s[MAX_PATH_LEN], *p;
    int ok, neg, curok;

    if (ctx->is_backdoor)
        return 1;

    if (!acs_string || !acs_string[0])
        return 1;

    strncpy(s, acs_string, sizeof(s) - 1);
    s[sizeof(s) - 1] = '\0';

    p = strtok(s, "&");
    if (p) memmove(s, p, strlen(p) + 1);
    else s[0] = 0;

    ok = 1;
    while (s[0]) {
        curok = 1;
        neg = 0;
        if (s[0] == '!') {
            memmove(s, s + 1, strlen(s));
            neg = 1;
        }
        switch (toupper(s[0])) {
        case 'A':
            if (!(ctx->ar & (1 << (toupper(s[1]) - 'A'))))
                curok = 0;
            break;
        case 'B':
            if ((ctx->modem_speed / 100) < atoi(s + 1))
                curok = 0;
            break;
        case 'C':
            if (!ctx->can_post)
                curok = 0;
            break;
        case 'D':
            if (ctx->dsl < atoi(s + 1))
                curok = 0;
            break;
        case 'G':
            if (ctx->age < atoi(s + 1))
                curok = 0;
            break;
        case 'I':
            if (!(ctx->dar & (1 << (toupper(s[1]) - 'A'))))
                curok = 0;
            break;
        case 'S':
            if (ctx->sl < atoi(s + 1))
                curok = 0;
            break;
        case 'U':
            if (atoi(s + 1) != ctx->usernum)
                curok = 0;
            break;
        case 'V':
            curok = 0;
            break;
        case '@':
            if (!ctx->conf_flagstr || !strchr(ctx->conf_flagstr, s[1]))
                curok = 0;
            break;
        case '#':
            if (!ctx->is_sysop)
                curok = 0;
            break;
        case 'F':
            if (!ctx->fastlogon)
                curok = 0;
            break;
        case 'H':
            if (atoi(s + 1) != ctx->helplevel)
                curok = 0;
            break;
        case 'E':
            switch (s[1]) {
            case 'R': if (!(ctx->exempt & exempt_ratio))    curok = 0; break;
            case 'T': if (!(ctx->exempt & exempt_time))     curok = 0; break;
            case 'U': if (!(ctx->exempt & exempt_userlist))  curok = 0; break;
            case 'P': if (!(ctx->exempt & exempt_post))     curok = 0; break;
            }
            break;
        }
        if (neg)
            curok = !curok;

        p = strtok(NULL, "&");
        if (p) memmove(s, p, strlen(p) + 1);
        else s[0] = 0;
        if (!curok)
            ok = 0;
    }

    return ok;
}


int slok(char val[31],char menu)
{
    acs_context_t ctx;

    acs_fill_context(&ctx);
    if (menu == 3) menu = 0;
    if (!menu) ctx.can_post = 0;  /* 'C' always denies in execution mode */

    return acs_check(val, &ctx);
}
