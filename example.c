
#include "jay.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
    char *S = "  [ { \"hello\": 3 }, { \"baz\": false }, \"blah\\u20AC\" ]";
    jay_t _j, *j = &_j;

    jay_init(j, S);

    jay_save(j);
    jay_print_value(j);
    jay_restore(j);

    jay_enter_array(j);
    jay_get_array_child(j, 0);
    jay_save(j);
    jay_enter_object(j);
    jay_find_object_child(j, "hello");
    jay_print_value(j);
    jay_restore(j);
    jay_skip(j);
    jay_next_value(j);
    jay_skip(j);
    jay_next_value(j);
    jay_skip(j);
    jay_leave_array(j);

    return 0;
}
