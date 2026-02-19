#include <stdlib.h>
#include <string.h>


/* Return a line from a message */

char * write_line (char **text,unsigned int linelen,char ctla) {

    static char line[133];
    register unsigned int x = 0;
	char *p;
	char *pp;


    if(!*text) return "";
	if(!**text) return *text;
    p = *text;
    pp = line;
    *pp = 0;
    while(++x < (linelen + 1)) {

ReSwitch:

        switch(*p) {
            case '\r':      *pp = 0;
                            p++;
                            goto GotCR;

			case '\0':      goto StopIt;

            case '\n':
            case '\x8d':    p++;
                            goto ReSwitch;

			case '\01':     if(!ctla) {

                                int x = 0;

                                if(p == *text || (*(p - 1) == '\r')) {
                                    while(*p != '\r' && *p &&  x < 128) {
                                        x++;
                                        p++;
                                    }
                                    if(*p == '\r') p++;
									goto ReSwitch;
								}
							}

            default:        *pp = *p;
							pp++;
							p++;
                            *pp = 0;
                            break;
        }
    }

StopIt:

    if(*p == ' ') {
        *pp = 0;
        while(*p == ' ') p++;
	}
    else if(x == (linelen + 1)) {
		if(strchr(line,' ')) {
            while(p > *text && *pp != ' ') {
                *pp = 0;
				pp--;
				p--;
			}
            if(p == *text) {
                strncpy(line,*text,linelen + 1);
                line[linelen + 1] = 0;
                p += (linelen + 1);
			}
			else p++;
		}
	}

GotCR:

    while(*pp == ' ' && pp > line) {    /* Rstrip returned string */
        *pp = 0;
		--pp;
	}

    *text = p;
	return line;
}
