/*
 * session.c — Per-session state global instance + initialization (Phase B)
 */

#include "session.h"
#include <string.h>

/* Undefine the compatibility macro so we can define the real variable */
#undef io

session_t session;

void session_init(session_t *s)
{
    /* Zero everything first */
    memset(s, 0, sizeof(session_t));

    /* Initialize the I/O subsystem */
    io_init(&s->io);

    /* Session defaults matching frequent_init() */
    s->usernum = 0;
    s->actsl = 0;
    s->umaxsubs = 0;
    s->umaxdirs = 0;

    s->cursub = 0;
    s->curdir = 0;
    s->curlsub = -1;
    s->curldir = 0;
    s->curconf = 0;
    s->confmode = 0;

    s->mdepth = 0;
    s->maxcmd = 0;

    s->timeon = 0.0;
    s->extratimecall = 0.0;
    s->last_time = 0.0;
    s->xtime = 0.0;
    s->time_event = 0.0;
    s->hanguptime1 = 0;
    s->nscandate = 0;
    s->timelastchar1 = 0;

    s->checkit = 0;
    s->okmacro = 1;
    s->okskey = 1;
    s->mailcheck = 0;
    s->smwcheck = 0;
    s->useron = 0;
    s->backdoor = 0;
    s->live_user = 1;
    s->doinghelp = 0;
    s->in_extern = 0;
    s->input_extern = 0;
    s->use_workspace = 0;
    s->express = 0;
    s->expressabort = 0;
    s->msgr = 1;
    s->msgreadlogon = 0;
    s->fwaiting = 0;
    s->fsenttoday = 0;
    s->topdata = 0;
    s->sysop_alert = 0;
    s->ltime = 0;
    s->do_event = 0;
    s->already_on = 0;
    s->arcling = 0;
    s->bchanged = 0;
    s->dlf = 0;
    s->edlf = 0;
    s->numf = 0;
    s->num_listed = 0;
    s->gat_section = 0;
    s->global_xx = 0;
    s->wfc = 0;
    s->ARC_NUMBER = -1;
    s->MAX_BATCH = 0;

    s->batchdir = 0;
    s->batchsize = 0.0;
    s->batchtime = 0.0;
    s->numbatch = 0;
    s->numbatchdl = 0;
    s->batchpoints = 0;

    s->bquote = 0;
    s->equote = 0;
    s->quoting = 0;
    s->quote = 0;

    s->chatsoundon = 1;
    s->blueinput = 0;
    s->readinvoting = 0;
    s->com_speed = 0;
    s->modem_speed = 0;
}
