
/* I license this work under the public domain. */

#pragma once

#include <stdbool.h>

/* Define to enable extra assertions to know when things go wrong. */
#define JAY_DEBUG 1

/* Define to enable the printf-style functions. Requires stdio.h */
#define JAY_PRINT_VALUE 1

/* Several convenience functions, like jay_print_value and
 * jay_find_object_child, have to decode strings internally.
 * This determines how long their internal static buffers are. */
#define JAY_STATIC_BUFFER_LENGTH 255

enum jay_type {
    JAY_ERROR,
    JAY_END,
    JAY_STRING,
    JAY_NUMBER,
    JAY_FALSE,
    JAY_TRUE,
    JAY_NULL,
    JAY_OBJECT = '{',
    JAY_ARRAY = '[',
};

/* Scanner -- intended to be private. Only included here 
 * so that you can place a jay_t on the stack. */

typedef struct { char *S; } jay_s;

/* Simple high-level parser interface */

/* A jay_t is composed of a scanner, along with a stack of up to ten (by default)
 * restore points, to save your position and come back later with. */

#define JAY_T_STACK_LENGTH 10

typedef struct {
    jay_s s;
    struct { jay_s s[JAY_T_STACK_LENGTH]; int n; } r;
    bool e;
} jay_t;

void jay_init(jay_t *j, char *S);

/* Returns the current pointer into the string. */
static inline char *jay_get_cursor(jay_t *j) { return j->s.S; }

/* jay_get_type is the main dispatch function -- it return what data type
 * the cursor is sitting on right now. */
enum jay_type jay_get_type(jay_t *j);
bool jay_has_error(jay_t *j);

/* These save the current position in the stack, and also return the current
 * cursor *before* these functions took effect. */
char *jay_save(jay_t *j);
char *jay_restore(jay_t *j);

/* Gets and decodes the contents of the currently pointed to string into
 * the given buffer.
 *
 * XXX: A way to detect string truncation. Return the length that would
 * have been without truncation? */
char *jay_get_string(jay_t *j, char *buf, int len);

/* Gets the length of the given string. Note that this is an upper bound
 * designed to be passed into malloc and may not accurately predict the
 * actual length of the string. */
int jay_string_len(jay_t *j);

void jay_enter_object(jay_t *j);
void jay_enter_array(jay_t *j);
void jay_leave_object(jay_t *j);
void jay_leave_array(jay_t *j);

/* To be used after an object key is read. */
void jay_read_key(jay_t *j);

/* To be used after an array or object value is read. */
void jay_next_value(jay_t *j);

/* Whether there's another value in the current array or object. */
bool jay_has_next_value(jay_t *j);

/* After entering an array or object, you can use these to skip to
 * the given indexed object or find a key by name. You probably want
 * to use jay_save before these methods if you're looking up multiple
 * values. */
void jay_get_array_child(jay_t *j, int idx);
bool jay_find_object_child(jay_t *j, char *key);

/* Skips over the current value. This can be used for content-less
 * values like false, true, and null, and also if you just don't care
 * about the current value. */
void jay_skip(jay_t *j);

#if JAY_PRINT_VALUE
/* A convenience function for debugging to help you figure out the
 * current value. */
void jay_print_value(jay_t *j);
#endif /* JAY_PRINT_VALUE */
