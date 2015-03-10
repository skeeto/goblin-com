#include <inttypes.h>
#include <stdlib.h>

/* all of this derived from http://en.wikipedia.org/wiki/UTF-8 */

/* given the leading byte of a utf8 character, return its total number of
 * bytes (1-4). does not check validity of any trailing bytes. */
static inline
size_t utf8_charlen(uint8_t c)
{
    if (c < 0x80) return 1;         /* 0xxxxxxx */
    if ((c & 0xe0)==0xc0) return 2; /* 110xxxxx */
    if ((c & 0xf0)==0xe0) return 3; /* 1110xxxx */
    if ((c & 0xf8)==0xf0 && (c <= 0xf4)) return 4; /* 11110xxx */
    else return 0; /* invalid UTF8 */
}

/* this one returns the utf8 len AND validates the full 1-4 bytes */
/* TODO add additional checks to reject "overlong forms", (e.g.
 * representing a three byte UTF8 character in 4 bytes, etc) by
 * having all zeros in the high bit positions. */
static inline
size_t utf8_valid(const uint8_t *c)
{
    size_t clen = utf8_charlen(*c);
    switch(clen) { /* use fall through */
    case 4: if ((c[3] & 0xc0) != 0x80) return 0;
    case 3: if ((c[2] & 0xc0) != 0x80) return 0;
    case 2: if ((c[1] & 0xc0) != 0x80) return 0;
    case 1: return clen;  /* no trailing bytes to validate */
    case 0: return 0;     /* invalid utf8 */
    }
    return clen; /* don't complain, gcc */
}

/* convert UTF8 to UTF32 */
static inline uint32_t
utf8_to_32(const uint8_t *c)
{
    switch(utf8_valid(c)) {
    case 0: return 0;  /* invalid utf8 */
    case 1: return *c; /* no work, just promote size */
    case 2: return ((c[0] & 0x1f) << 6) | (c[1] & 0x3f);
    case 3: return ((c[0] & 0x0f) << 12) | ((c[1] & 0x3f) << 6) | (c[2] & 0x3f);
    case 4: return ((c[0] & 0x07) << 18) | ((c[1] & 0x3f) << 12) | ((c[2] & 0x3f) << 6) | (c[3] & 0x3f);
    }
    return 0; /* no complaints gcc */
}

/* convert UTF32 to UTF8. return the number of utf8 bytes (0-4) written to c */
static inline size_t
utf32_to_8(uint32_t utf32, uint8_t *utf8)
{
    if (utf32 < 0x80) {
        *utf8 = (uint8_t)utf32;
        return 1;
    }
    if (utf32 < 0x800) {
        utf8[0] = 0xc0 | ((utf32 & 0x07c0) >> 6);
        utf8[1] = 0x80 |  (utf32 & 0x003f);
        return 2;
    }
    if (utf32 < 0x10000) {
        utf8[0] = 0xe0 | ((utf32 & 0xf000) >> 12);
        utf8[1] = 0x80 | ((utf32 & 0x0fc0) >>  6);
        utf8[2] = 0x80 |  (utf32 & 0x003f);
        return 3;
    }
    if (utf32 < 0x110000) {
        utf8[0] = 0xf0 | ((utf32 & 0x1c0000) >> 18);
        utf8[1] = 0x80 | ((utf32 & 0x03f000) >> 12);
        utf8[2] = 0x80 | ((utf32 & 0x000fc0) >>  6);
        utf8[3] = 0x80 |  (utf32 & 0x00003f);
        return 4;
    }
    return 0; /* invalid utf32 */
}

/* Note: this merely counts codepoints, which is not the same as
 * visual length when rendered. */
static inline size_t
utf8_strlen(const uint8_t *utf8)
{
    size_t length = 0;
    for (; *utf8; length++, utf8 += utf8_charlen(*utf8));
    return length;
}
