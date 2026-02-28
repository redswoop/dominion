/*
 * cp437.h — CP437 (DOS) to UTF-8 translation for terminal output
 *
 * Maps all 256 CP437 byte values to their UTF-8 string equivalents.
 * Characters 0x20-0x7E map to ASCII. Characters 0x00-0x1F and 0x80-0xFF
 * map to the corresponding Unicode code points for CP437.
 */

#ifndef __CP437_H__
#define __CP437_H__

#include <stdio.h>

static const char *cp437_to_utf8[256] = {
    /* 0x00-0x0F: control chars / special glyphs in CP437 */
    " ",           "\xe2\x98\xba", "\xe2\x98\xbb", "\xe2\x99\xa5",  /* NUL ☺ ☻ ♥ */
    "\xe2\x99\xa6", "\xe2\x99\xa3", "\xe2\x99\xa0", "\xe2\x80\xa2",  /* ♦ ♣ ♠ • */
    "\xe2\x97\x98", "\xe2\x97\x8b", "\xe2\x97\x99", "\xe2\x99\x82",  /* ◘ ○ ◙ ♂ */
    "\xe2\x99\x80", "\xe2\x99\xaa", "\xe2\x99\xab", "\xe2\x98\xbc",  /* ♀ ♪ ♫ ☼ */

    /* 0x10-0x1F */
    "\xe2\x96\xba", "\xe2\x97\x84", "\xe2\x86\x95", "\xe2\x80\xbc",  /* ► ◄ ↕ ‼ */
    "\xc2\xb6",     "\xc2\xa7",     "\xe2\x96\xac", "\xe2\x86\xa8",  /* ¶ § ▬ ↨ */
    "\xe2\x86\x91", "\xe2\x86\x93", "\xe2\x86\x92", "\xe2\x86\x90",  /* ↑ ↓ → ← */
    "\xe2\x88\x9f", "\xe2\x86\x94", "\xe2\x96\xb2", "\xe2\x96\xbc",  /* ∟ ↔ ▲ ▼ */

    /* 0x20-0x7E: standard ASCII (single byte, direct mapping) */
    " ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",
    "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
    "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\", "]", "^", "_",
    "`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
    "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~",

    /* 0x7F: house / delete */
    "\xe2\x8c\x82",  /* ⌂ */

    /* 0x80-0x8F: accented chars */
    "\xc3\x87", "\xc3\xbc", "\xc3\xa9", "\xc3\xa2",  /* Ç ü é â */
    "\xc3\xa4", "\xc3\xa0", "\xc3\xa5", "\xc3\xa7",  /* ä à å ç */
    "\xc3\xaa", "\xc3\xab", "\xc3\xa8", "\xc3\xaf",  /* ê ë è ï */
    "\xc3\xae", "\xc3\xac", "\xc3\x84", "\xc3\x85",  /* î ì Ä Å */

    /* 0x90-0x9F */
    "\xc3\x89", "\xc3\xa6", "\xc3\x86", "\xc3\xb4",  /* É æ Æ ô */
    "\xc3\xb6", "\xc3\xb2", "\xc3\xbb", "\xc3\xb9",  /* ö ò û ù */
    "\xc3\xbf", "\xc3\x96", "\xc3\x9c", "\xc2\xa2",  /* ÿ Ö Ü ¢ */
    "\xc2\xa3", "\xc2\xa5", "\xe2\x82\xa7", "\xc6\x92",  /* £ ¥ ₧ ƒ */

    /* 0xA0-0xAF */
    "\xc3\xa1", "\xc3\xad", "\xc3\xb3", "\xc3\xba",  /* á í ó ú */
    "\xc3\xb1", "\xc3\x91", "\xc2\xaa", "\xc2\xba",  /* ñ Ñ ª º */
    "\xc2\xbf", "\xe2\x8c\x90", "\xc2\xac", "\xc2\xbd",  /* ¿ ⌐ ¬ ½ */
    "\xc2\xbc", "\xc2\xa1", "\xc2\xab", "\xc2\xbb",  /* ¼ ¡ « » */

    /* 0xB0-0xBF: box/shade characters */
    "\xe2\x96\x91", "\xe2\x96\x92", "\xe2\x96\x93", "\xe2\x94\x82",  /* ░ ▒ ▓ │ */
    "\xe2\x94\xa4", "\xe2\x95\xa1", "\xe2\x95\xa2", "\xe2\x95\x96",  /* ┤ ╡ ╢ ╖ */
    "\xe2\x95\x95", "\xe2\x95\xa3", "\xe2\x95\x91", "\xe2\x95\x97",  /* ╕ ╣ ║ ╗ */
    "\xe2\x95\x9d", "\xe2\x95\x9c", "\xe2\x95\x9b", "\xe2\x94\x90",  /* ╝ ╜ ╛ ┐ */

    /* 0xC0-0xCF */
    "\xe2\x94\x94", "\xe2\x94\xb4", "\xe2\x94\xac", "\xe2\x94\x9c",  /* └ ┴ ┬ ├ */
    "\xe2\x94\x80", "\xe2\x94\xbc", "\xe2\x95\x9e", "\xe2\x95\x9f",  /* ─ ┼ ╞ ╟ */
    "\xe2\x95\x9a", "\xe2\x95\x94", "\xe2\x95\xa9", "\xe2\x95\xa6",  /* ╚ ╔ ╩ ╦ */
    "\xe2\x95\xa0", "\xe2\x95\x90", "\xe2\x95\xac", "\xe2\x95\xa7",  /* ╠ ═ ╬ ╧ */

    /* 0xD0-0xDF */
    "\xe2\x95\xa8", "\xe2\x95\xa4", "\xe2\x95\xa5", "\xe2\x95\x99",  /* ╨ ╤ ╥ ╙ */
    "\xe2\x95\x98", "\xe2\x95\x92", "\xe2\x95\x93", "\xe2\x95\xab",  /* ╘ ╒ ╓ ╫ */
    "\xe2\x95\xaa", "\xe2\x94\x98", "\xe2\x94\x8c", "\xe2\x96\x88",  /* ╪ ┘ ┌ █ */
    "\xe2\x96\x84", "\xe2\x96\x8c", "\xe2\x96\x90", "\xe2\x96\x80",  /* ▄ ▌ ▐ ▀ */

    /* 0xE0-0xEF: Greek/math */
    "\xce\xb1", "\xc3\x9f", "\xce\x93", "\xcf\x80",  /* α ß Γ π */
    "\xce\xa3", "\xcf\x83", "\xc2\xb5", "\xcf\x84",  /* Σ σ µ τ */
    "\xce\xa6", "\xce\x98", "\xce\xa9", "\xce\xb4",  /* Φ Θ Ω δ */
    "\xe2\x88\x9e", "\xcf\x86", "\xce\xb5", "\xe2\x88\xa9",  /* ∞ φ ε ∩ */

    /* 0xF0-0xFF */
    "\xe2\x89\xa1", "\xc2\xb1", "\xe2\x89\xa5", "\xe2\x89\xa4",  /* ≡ ± ≥ ≤ */
    "\xe2\x8c\xa0", "\xe2\x8c\xa1", "\xc3\xb7", "\xe2\x89\x88",  /* ⌠ ⌡ ÷ ≈ */
    "\xc2\xb0", "\xe2\x88\x99", "\xc2\xb7", "\xe2\x88\x9a",  /* ° ∙ · √ */
    "\xe2\x81\xbf", "\xc2\xb2", "\xe2\x96\xa0", "\xc2\xa0",  /* ⁿ ² ■ NBSP */
};

/* Write a CP437 byte to stdout as UTF-8 */
static inline void put_cp437(unsigned char ch) __attribute__((unused));
static inline void put_cp437(unsigned char ch)
{
    fputs(cp437_to_utf8[ch], stdout);
}

#endif /* __CP437_H__ */
