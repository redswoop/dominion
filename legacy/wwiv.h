/* DATA FOR EVERY USER */
typedef struct {

    char
      name[31],       /* user's name/handle */
      realname[21],   /* user's real name */
      callsign[7],    /* user's amateur callsign */
      phone[13],      /* user's phone number */
      dataphone[13],  /* user's data phone */
      street[31],     /* street address */
      city[31],       /* city */
      state[3],       /* state code [MO, CA, etc] */
      country[4],     /* country [USA, CAN, FRA, etc] */
      zipcode[11],    /* zipcode [#####-####] */
      pw[9],          /* user's password */
      laston[9],      /* last date on */
      firston[9],     /* first date on */
      note[61],       /* sysop's note about user */
      macros[3][81],  /* macro keys */
      sex;            /* user's sex */

    char
      res_char[78];   /* bytes for more strings */

    unsigned char
      age,            /* user's age */
      inact,          /* if deleted or inactive */
      comp_type,      /* computer type */
      defprot,        /* deflt transfer protocol */
      defed,          /* default editor */
      screenchars,    /* screen width */
      screenlines,    /* screen height */
      num_extended,   /* extended description lines */
      optional_val,   /* optional lines in msgs */
      sl,             /* security level */
      dsl,            /* transfer security level */
      exempt,         /* exempt from ratios, etc */
      colors[10],     /* user's colors */
      bwcolors[10],   /* user's b&w colors */
      votes[20],      /* user's votes */
      illegal,        /* illegal logons */
      waiting,        /* number mail waiting */
      ontoday,        /* num times on today */
      month,          /* birth month */
      day,            /* birth day */
      year,           /* birth year */
      language;       /* language to use */


    char
      res_byte[50];   /* reserved for byte values */

    unsigned short
      homeuser,       /* user number where user can be found */
      homesys,        /* system where user can be found */
      forwardusr,     /* mail forwarded to this user number */
      forwardsys,     /* mail forwarded to this system number */
      net_num,        /* net num for forwarding */
      msgpost,        /* number messages posted */
      emailsent,      /* number of email sent */
      feedbacksent,   /* number of f-back sent */
      fsenttoday1,    /* feedbacks today */
      posttoday,      /* number posts today */
      etoday,         /* number emails today */
      ar,             /* board access */
      dar,            /* directory access */
      restrict,       /* restrictions on account */
      ass_pts,        /* bad things the user did */
      uploaded,       /* number files uploaded */
      downloaded,     /* number files downloaded */
      lastrate,       /* last baud rate on */
      logons,         /* total number of logons */
      emailnet,       /* email sent via net */
      postnet,        /* posts sent thru net */
      deletedposts,   /* how many posts deleted */
      chainsrun,      /* how many "chains" run */
      gfilesread,     /* how many gfiles read */
      banktime,       /* how many mins in timebank */
      homenet;        /* home net number */

   char
     res_short[48];   /* reserved for short values */

    unsigned long
      msgread,        /* total num msgs read */
      uk,             /* number of k uploaded */
      dk,             /* number of k downloaded */
      daten,          /* numerical time last on */
      sysstatus,      /* status/defaults */
      wwiv_regnum,    /* user's WWIV reg number */
      filepoints;     /* points to spend for files */

    char
      res_long[56];   /* reserved for long values */

    float
      timeontoday,    /* time on today */
      extratime,      /* time left today */
      timeon,         /* total time on system */
      pos_account,    /* $ credit */
      neg_account,    /* $ debit */
      gold;           /* game money */

    char
      res_float[32];  /* reserved for real values */

    char
      res_gp[100];    /* reserved for whatever */

} wuserrec;
