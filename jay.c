
/* I license this work under the public domain. */

#include "jay.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Write code-point cp in CESU-8 form into p */
static int cesu8_write(uint8_t *p, uint16_t cp) {
    if      (cp <= 0x007f) { *p++ = cp; return 1; }
    else if (cp <= 0x07ff) { *p++ = (0xc0 | (cp >>  6)); *p++ = (0x80 | (cp & 0x3f)); return 2; }
    else                   { *p++ = (0xe0 | (cp >> 12)); *p++ = (0x80 | ((cp >> 6) & 0x3f)); *p++ = (0x80 | (cp & 0x3f)); return 3; }
}

/* Parse a hex digit */
static uint8_t dhexd(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a';
    if (c >= 'A' && c <= 'F') return c - 'A';
    assert(false);
}

/* Parse a four-digit hex sequence */
static uint16_t dhex(char *p)
{
    return (dhexd(p[0]) << 12) | (dhexd(p[1]) << 8) | (dhexd(p[2]) << 4) | dhexd(p[3]);
}

/* Scanner */

static inline void jays_init(jay_s *j, char *S) { j->S = S; }

static inline char  pk    (jay_s *j, int L) { return j->S[L]; }
static inline char  hd    (jay_s *j) { return j->S[0]; }
static inline void  adv   (jay_s *j) { ++j->S; }
static inline void  advn  (jay_s *j, int n) { j->S += n; }
static inline void  sync  (jay_s *j) { while (isblank(hd(j))) adv(j); }
static inline bool  match (jay_s *j, char *S) { return strcmp(j->S, S); }
static inline bool  breq  (jay_s *j, char c) {
    sync(j);
    if (hd(j) == c) {
        adv(j);
        return true;
    } else {
#if JAY_DEBUG
        assert(false);
#endif /* JAY_DEBUG */
        return false;
    }
}

/* jay_type, with the addition of the chars }]:, is our collection of tokens */
static enum jay_type tok(jay_s *j)
{
    sync(j);

    switch (hd(j)) {
    case '\0':
        return JAY_END;
    case '"':
    case '\'':
        return JAY_STRING;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
        return JAY_NUMBER;
    case '[':
    case '{':
    case ']':
    case '}':
    case ':':
    case ',':
        return (enum jay_type) hd(j);
    default:
        break;
    }

    if (match(j, "false")) return JAY_FALSE;
    if (match(j, "true"))  return JAY_TRUE;
    if (match(j, "null"))  return JAY_NULL;

    return JAY_ERROR;
}

static void chomp_string(jay_s *j)
{
    sync(j);

    char delim = hd(j);
    while (hd(j) != '\0') {
        adv(j);

        if (hd(j) == delim) {
            adv(j);
            break;
        }

        if (hd(j) == '\\')
            adv(j);
    }
}

static char *get_string(jay_s *j, char *V, int Vl)
{
    sync(j);

    char delim = hd(j);

    int i = 0;
    for (i = 0; i < Vl - 1;) {
        adv(j);

        if (hd(j) == delim) {
            adv(j);
            break;
        }

        if (hd(j) == '\\') {
            switch (pk(j, 1)) {
            case '"':  V[i++] = '"';  break;
            case '\'': V[i++] = '\''; break;
            case '\\': V[i++] = '\\'; break;
            case 'b':  V[i++] = '\b'; break;
            case 'f':  V[i++] = '\f'; break;
            case 'n':  V[i++] = '\n'; break;
            case 'r':  V[i++] = '\r'; break;
            case 't':  V[i++] = '\t'; break;
            case 'u': {
                uint32_t cp = dhex(j->S+2);
                i += cesu8_write(&V[i], cp);
                break;
            }
            }
            adv(j);
        } else {
            V[i++] = hd(j);
        }
    }
    V[i] = '\0';
    return V;
}

static double chomp_number(jay_s *j)
{
    sync(j);
    /* XXX: We should at least verify the number ourselves
     * so we don't parse hexadecimals and such. */
    return strtod(j->S, &j->S);
}

static void jreq(jay_t *j, char c) { if (!breq(&j->s, c)) j->e = true; }

void jay_init(jay_t *j, char *S) { memset(j, 0, sizeof(*j)); jays_init(&j->s, S); }

enum jay_type jay_get_type(jay_t *j) { if (j->e) return JAY_ERROR; return tok(&j->s); }
bool jay_has_error(jay_t *j) { return jay_get_type(j) == JAY_ERROR; }

char *jay_save(jay_t *j) { j->r.s[j->r.n++] = j->s; return j->s.S; }
char *jay_restore(jay_t *j) { j->s = j->r.s[--j->r.n]; return j->r.s[j->r.n+1].S; }

double jay_get_number(jay_t *j) { return chomp_number(&j->s); }

char *jay_get_string(jay_t *j, char *buf, int len) { return get_string(&j->s, buf, len); }

int jay_string_len(jay_t *j) {
    char *S = jay_save(j);
    chomp_string(&j->s);
    char *S2 = jay_restore(j);
    /* For the convenience of the user, also include the trailing NUL. */
    return S2 - S + 1;
}

void jay_enter_object(jay_t *j)   { jreq(j, '{'); }
void jay_enter_array(jay_t *j)    { jreq(j, '['); }
void jay_leave_object(jay_t *j)   { jreq(j, '}'); }
void jay_leave_array(jay_t *j)    { jreq(j, ']'); }

/* XXX: I need a better name for this function that doesn't include a
 * homophone. */
void jay_read_key(jay_t *j)       { jreq(j, ':'); }

bool jay_has_next_value(jay_t *j) { return jay_get_type(j) == ','; }
void jay_next_value(jay_t *j)     { jreq(j, ','); }

void jay_skip(jay_t *j);

void jay_get_array_child(jay_t *j, int idx) {
    while (idx--) {
        jay_skip(j);
        jreq(j, ',');
    }
}
bool jay_find_object_child(jay_t *j, char *key) {
    char kbuf[JAY_STATIC_BUFFER_LENGTH];
    assert(strlen(key) < sizeof(kbuf));

    while (true) {
        get_string(&j->s, kbuf, sizeof(kbuf));
        jay_read_key(j);
        if (strncmp(key, kbuf, sizeof(kbuf)) == 0)
            return true;
        jay_skip(j);
        if (!jay_has_next_value(j))
            break;
        jay_next_value(j);
    }
    return false;
}

static void skip_string(jay_t *j)  { chomp_string(&j->s); }
static void skip_number(jay_t *j)  { chomp_number(&j->s); }

static void skip_array(jay_t *j) {
    jay_enter_array(j);
    while (true) {
        jay_skip(j);
        if (jay_has_next_value(j))
            adv(&j->s);
        else
            break;
    }
    adv(&j->s);
}
static void skip_object(jay_t *j) {
    jay_enter_object(j);
    while (true) {
        jay_skip(j);
        jreq(j, ':');
        jay_skip(j);
        if (jay_has_next_value(j))
            adv(&j->s);
        else
            break;
    }
    adv(&j->s);
}

void jay_skip(jay_t *j) {
    switch (jay_get_type(j)) {
    case JAY_FALSE:  advn(&j->s, 5); break;
    case JAY_TRUE:   advn(&j->s, 4); break;
    case JAY_NULL:   advn(&j->s, 4); break;
    case JAY_ARRAY:  skip_array(j); break;
    case JAY_OBJECT: skip_object(j); break;
    case JAY_NUMBER: skip_number(j); break;
    case JAY_STRING: skip_string(j); break;
    default: break;
    }
}

#if JAY_PRINT_VALUE
#include <stdio.h>

static void print_value(jay_t *j);

static void print_array(jay_t *j) {
    jay_enter_array(j); printf("[");
    while (true) {
        print_value(j);
        if (!jay_has_next_value(j)) break;
        jay_next_value(j); printf(", ");
    }
    jay_leave_array(j); printf("]");
}
static void print_object(jay_t *j) {
    jay_enter_object(j); printf("{");
    while (true) {
        print_value(j);
        jay_read_key(j); printf(": ");
        print_value(j);
        if (!jay_has_next_value(j)) break;
        jay_next_value(j); printf(", ");
    }
    jay_leave_object(j); printf("}");
}
static void print_value(jay_t *j) {
    char strbuf[JAY_STATIC_BUFFER_LENGTH];

    switch (jay_get_type(j)) {
    case JAY_FALSE:  printf("false"); jay_skip(j); break;
    case JAY_TRUE:   printf("true");  jay_skip(j); break;
    case JAY_NULL:   printf("null");  jay_skip(j); break;
    case JAY_ARRAY:  print_array(j); break;
    case JAY_OBJECT: print_object(j); break;
    case JAY_NUMBER: printf("%g", jay_get_number(j)); break;
    case JAY_STRING: printf("\"%s\"", jay_get_string(j, strbuf, sizeof(strbuf))); break;
    case JAY_ERROR:  printf("ERROR"); break;
    default:         printf("UNK"); break;
    }
}
void jay_print_value(jay_t *j) {
    print_value(j); printf("\n");
}

#endif /* JAY_PRINT_VALUE */
