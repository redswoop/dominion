/*
 * ansi_attr.c — Pure ANSI SGR escape sequence generation (Layer 3)
 *
 * Extracted from com.c.  No BBS globals — only depends on platform.h
 * for itoa().  Produces ESC[...m sequences that transition from
 * current_attr to target_attr.
 */

#include "platform.h"

static void addto(char *s, int i)
{
    char temp[20];

    if (s[0])
        strcat(s, ";");
    else
        strcpy(s, "\x1B[");
    itoa(i, temp, 10);
    strcat(s, temp);
}

void makeansi(unsigned char attr, char *s, unsigned char current_attr)
{
    unsigned char catr;
    char *temp = "04261537";

    catr = current_attr;
    s[0] = 0;

    if (attr != catr) {
        if ((catr & 0x88) ^ (attr & 0x88)) {
            addto(s, 0);
            addto(s, 30 + temp[attr & 0x07] - '0');
            addto(s, 40 + temp[(attr & 0x70) >> 4] - '0');
            catr = attr & 0x77;
        }
        if ((catr & 0x07) != (attr & 0x07))
            addto(s, 30 + temp[attr & 0x07] - '0');
        /* Always include explicit background when emitting color changes.
         * Terminals with non-black default backgrounds show through when
         * background is omitted because it "hasn't changed" from black. */
        if (s[0] || (catr & 0x70) != (attr & 0x70))
            addto(s, 40 + temp[(attr & 0x70) >> 4] - '0');
        if ((catr & 0x08) ^ (attr & 0x08))
            addto(s, 1);
        if ((catr & 0x80) ^ (attr & 0x80))
            addto(s, 5);
    }
    if (s[0])
        strcat(s, "m");
}
